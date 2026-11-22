#include "evaluation.hpp"

#include <charconv>
#include <cmath>
#include <deque>
#ifdef _WIN32
#include <filesystem>
#endif
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <string_view>
#include <utility>

namespace cfgy {
namespace {

using detail::Eval;
using detail::Integer;
using detail::pi;
constexpr auto min_int = std::numeric_limits<Integer>::min();
constexpr auto max_int = std::numeric_limits<Integer>::max();

struct Setting;

struct Token {
    std::string kind;
    std::string text;
    std::size_t line;
    std::size_t column;
    Setting* reference = nullptr;
};

using Line = std::vector<Token>;

[[noreturn]] void fail(const std::string& source, const Token& token, const std::string& message) {
    throw ConfigError(source + ":" + std::to_string(token.line) + ":" +
                      std::to_string(token.column) + ": " + message);
}

bool digit(char c) { return c >= '0' && c <= '9'; }
bool letter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }

// All bindings receive UTF-8 strings, including when a file is opened by C.
void validate_utf8(std::string_view line, std::size_t number, const std::string& source) {
    for (std::size_t i = 0; i < line.size();) {
        const auto start = i;
        const auto lead = static_cast<unsigned char>(line[i++]);
        if (lead < 0x80) continue;
        const auto invalid = [&] { fail(source, {"", "", number, start + 1}, "invalid UTF-8"); };
        if (lead < 0xc2 || lead > 0xf4) invalid();
        const std::size_t extra = lead < 0xe0 ? 1 : (lead < 0xf0 ? 2 : 3);
        if (line.size() - i < extra) invalid();
        const auto second = static_cast<unsigned char>(line[i]);
        // Exclude overlong encodings, UTF-16 surrogates and values above U+10FFFF.
        if ((lead == 0xe0 && second < 0xa0) || (lead == 0xed && second >= 0xa0) ||
            (lead == 0xf0 && second < 0x90) || (lead == 0xf4 && second >= 0x90)) invalid();
        for (std::size_t end = i + extra; i < end; ++i) {
            if ((static_cast<unsigned char>(line[i]) & 0xc0) != 0x80) invalid();
        }
    }
}

Line lex_line(const std::string& line, std::size_t number, const std::string& source) {
    Line tokens;
    std::size_t i = 0;
    // std::string supplies a zero terminator at size(), useful for one-byte lookahead.
    while (i < line.size() && line[i] != '#') {
        const auto start = i;
        const char c = line[i++];
        Token token{"", "", number, start + 1};
        if (c == ' ' || c == '\t') {
            if (c == '\t' && tokens.empty()) fail(source, token, "use spaces for indentation");
            continue;
        }
        if (c == '"' || c == '\'') {
            token.kind = "string";
            while (i < line.size() && line[i] != c) {
                char ch = line[i++];
                if (ch == '\\') {
                    switch (line[i]) {
                        case 'n': ch = '\n'; break;
                        case 't': ch = '\t'; break;
                        case 'r': ch = '\r'; break;
                        case '"': case '\'': case '\\': ch = line[i]; break;
                        default: fail(source, {"", "", number, i}, "invalid string escape");
                    }
                    ++i;
                }
                token.text += ch;
            }
            if (i == line.size()) fail(source, token, "unterminated string");
            ++i;
        } else if (letter(c)) {
            while (letter(line[i]) || digit(line[i])) ++i;
            token.kind = "name";
            token.text = line.substr(start, i - start);
        } else if (digit(c) || (c == '.' && digit(line[i]))) {
            while (digit(line[i])) ++i;
            if (c != '.' && line[i] == '.') {
                ++i;
                while (digit(line[i])) ++i;
            }
            if (line[i] == 'e' || line[i] == 'E') {
                auto exponent = i + 1;
                if (line[exponent] == '+' || line[exponent] == '-') ++exponent;
                if (digit(line[exponent])) {
                    i = exponent + 1;
                    while (digit(line[i])) ++i;
                }
            }
            token.kind = "number";
            token.text = line.substr(start, i - start);
        } else if (std::string_view(":[],().+-*/@").find(c) != std::string_view::npos) {
            token.kind = token.text = std::string(1, c);
        } else {
            fail(source, token, "unexpected character");
        }
        tokens.push_back(std::move(token));
    }
    if (!tokens.empty()) tokens.push_back({"end", "", number, i + 1});
    return tokens;
}

std::vector<Line> read_lines(std::istream& input, const std::string& source) {
    std::vector<Line> lines;
    std::string text;
    for (std::size_t n = 1; std::getline(input, text); ++n) {
        if (!text.empty() && text.back() == '\r') text.pop_back();
        validate_utf8(text, n, source);
        Line row = lex_line(text, n, source);
        if (row.empty()) continue;
        // Split leading list markers so "- key: value" uses ordinary block parsing.
        auto first = row.begin();
        while (first->kind == "-" && (first + 1)->kind != "end" &&
               (first + 1)->column > first->column + 1) {
            lines.push_back({*first, {"end", "", n, first->column + 1}});
            ++first;
        }
        row.erase(row.begin(), first);
        lines.push_back(std::move(row));
    }
    if (input.bad()) throw ConfigError(source + ": error reading input");
    return lines;
}

int precedence(const std::string& op) {
    if (op == "+" || op == "-") return 1;
    if (op == "*" || op == "/" || op == "@") return 2;
    return 0;
}

// One graph per document. Mapping fields are indexed by name and inputs retain
// source order for evaluating expressions with random draws.
enum class Kind { expression, mapping, list, name, field };

struct Setting {
    Kind kind;
    const Token& location;
    Setting* scope;
    std::vector<Setting*> inputs;
    std::map<std::string, Setting*> fields;
    Line expression;
    Setting* target;  // Final definition after resolving aliases and dotted paths.
    bool visiting = false;
    std::size_t uses = 0;  // Move the cached value to its last consumer.
    Eval result{nullptr};
    bool evaluated = false;

    Setting(Kind kind, const Token& location, Setting* scope)
        : kind(kind), location(location), scope(scope),
          target(kind == Kind::name || kind == Kind::field ? nullptr : this) {}
};

Eval take_value(Setting& definition) {
    if (--definition.uses == 0) return std::move(definition.result);
    return definition.result;
}

class Expression {
public:
    Expression(const Line& line, const std::string& source, detail::Random& random)
        : line_(line), source_(source), random_(random) {}

    Eval parse() {
        auto value = expression();
        require("end");
        return value;
    }

private:
    const Line& line_;
    std::size_t i_ = 0;
    const std::string& source_;
    detail::Random& random_;

    bool accept(const std::string& kind) {
        if (line_[i_].kind != kind) return false;
        ++i_;
        return true;
    }

    void require(const std::string& kind) {
        if (!accept(kind)) fail(source_, line_[i_], "expected '" + kind + "'");
    }

    Eval finite(double n, const Token& token) const {
        if (!std::isfinite(n)) fail(source_, token, "float out of range");
        return Eval{n};
    }

    Eval number(const Token& token, bool negative = false) const {
        const std::string text = negative ? "-" + token.text : token.text;
        if (text.find_first_of(".eE") == std::string::npos) {
            Integer n;
            const auto result = std::from_chars(text.data(), text.data() + text.size(), n);
            if (result.ec != std::errc{}) fail(source_, token, "integer out of range");
            return Eval{n};
        }
        std::istringstream input(text);
        input.imbue(std::locale::classic());
        double n;
        if (!(input >> n)) fail(source_, token, "float out of range");
        return finite(n, token);
    }

    Eval::List arguments(const std::string& closing) {
        Eval::List args;
        if (!accept(closing)) {
            do {
                args.push_back(expression());
                if (accept(closing)) break;
                require(",");
            } while (true);
        }
        return args;
    }

    Eval call(const Token& name) {
        using Function = double (*)(double);
        static const std::map<std::string, Function> functions = {
            {"torad", detail::to_radians},
            {"todeg", [](double x) { return x * (180.0 / pi); }},
            {"abs", std::fabs}, {"sin", std::sin}, {"cos", std::cos}, {"tan", std::tan},
            {"sqrt", std::sqrt}, {"floor", std::floor}, {"ceil", std::ceil}
        };
        const bool is_random = name.text.compare(0, 5, "Rand.") == 0;
        std::shared_ptr<std::mt19937_64> stream;
        if (accept("[")) {
            if (!is_random || name.text == "Rand.seed")
                fail(source_, name, "only random draws accept a seed selector");
            const auto& start = line_[i_];
            stream = expression().random_stream;
            require("]");
            if (!stream) fail(source_, start, "expected a seed created by Rand.seed");
        }
        require("(");
        auto args = arguments(")");
        try {
            if (name.text == "diag") return Eval{detail::diagonal(args)};
            if (name.text == "matrix" || name.text == "transpose") {
                if (args.size() != 1) fail(source_, name, name.text + " expects one matrix or list of rows");
                auto matrix = detail::as_matrix(std::move(args.front()));
                if (name.text == "transpose") return Eval{detail::transpose(matrix)};
                return Eval{std::move(matrix)};
            }
            if (name.text.compare(0, 4, "Rot.") == 0) return detail::rotation(name.text, args);
            if (is_random) return random_.call(name.text, args, stream.get());
        } catch (const std::domain_error& e) {
            fail(source_, name, e.what());
        }
        const auto fn = functions.find(name.text);
        if (fn == functions.end()) fail(source_, name, "unknown function '" + name.text + "'");
        if (args.empty()) fail(source_, name, name.text + " expects one or more numbers");
        for (auto& arg : args) {
            if (!is_number(arg)) fail(source_, name, name.text + " expects one or more numbers");
            if (name.text == "abs" && std::holds_alternative<Integer>(arg.data)) {
                const auto n = arg.as_int();
                if (n == min_int) fail(source_, name, "integer out of range");
                arg = Eval{n < 0 ? -n : n};
            } else {
                const double result = fn->second(to_double(arg));
                if (std::isnan(result)) fail(source_, name, name.text + " argument outside its domain");
                arg = finite(result, name);
            }
        }
        if (args.size() == 1) return std::move(args.front());
        return Eval{std::move(args)};
    }

    Eval operand() {
        const auto& token = line_[i_];
        if (accept("+") || accept("-")) {
            const bool negative = token.kind == "-";
            const auto& next = line_[i_];
            if (accept("number")) return number(next, negative);
            auto value = operand();
            if (detail::is_matrix_operand(value))
                return matrix_op(token, '*', std::move(value), Eval{negative ? -1.0 : 1.0});
            if (!is_number(value)) fail(source_, token, "unary sign expects a number");
            if (!negative) return value;
            if (auto* n = std::get_if<Integer>(&value.data)) {
                if (*n == min_int) fail(source_, token, "integer out of range");
                return Eval{-*n};
            }
            return Eval{-value.as_float()};
        }
        if (accept("number")) return number(token);
        if (accept("string")) return Eval{token.text};
        if (accept("function")) return call(token);
        if (accept("reference")) return take_value(*token.reference->target);
        if (accept("[")) return Eval{arguments("]")};
        if (accept("(")) {
            auto value = expression();
            require(")");
            return value;
        }
        fail(source_, token, "expected a value");
    }

    Eval integer_op(const Token& op, Integer a, Integer b) const {
        if (op.kind == "+") {
            if (!((b > 0 && a > max_int - b) || (b < 0 && a < min_int - b))) return Eval{a + b};
        } else if (op.kind == "-") {
            if (!((b > 0 && a < min_int + b) || (b < 0 && a > max_int + b))) return Eval{a - b};
        } else {
            // Check by division before multiplying to avoid signed overflow.
            const bool overflow = (a > 0 && (b > max_int / a || b < min_int / a)) ||
                                  (a < 0 && b > 0 && a < min_int / b) ||
                                  (a < 0 && b < 0 && a < max_int / b);
            if (!overflow) return Eval{a * b};
        }
        fail(source_, op, "integer out of range");
    }

    Eval matrix_op(const Token& token, char op, Eval left, Eval right) const {
        try {
            return detail::matrix_operation(op, std::move(left), std::move(right));
        } catch (const std::domain_error& e) {
            fail(source_, token, e.what());
        }
    }

    Eval apply(const Token& op, Eval left, Eval right) const {
        if (op.kind == "+" && std::holds_alternative<Eval::List>(left.data) &&
            std::holds_alternative<Eval::List>(right.data)) {
            auto& list = std::get<Eval::List>(left.data);
            list.insert(list.end(), right.as_list().begin(), right.as_list().end());
            return left;
        }
        if (op.kind == "@" || detail::is_matrix_operand(left) || detail::is_matrix_operand(right))
            return matrix_op(op, op.kind.front(), std::move(left), std::move(right));
        if (!is_number(left) || !is_number(right)) fail(source_, op, "invalid operands for '" + op.kind + "'");
        if (op.kind != "/" && std::holds_alternative<Integer>(left.data) &&
            std::holds_alternative<Integer>(right.data)) {
            return integer_op(op, left.as_int(), right.as_int());
        }
        const double a = to_double(left), b = to_double(right);
        if (op.kind == "+") return finite(a + b, op);
        if (op.kind == "-") return finite(a - b, op);
        if (op.kind == "*") return finite(a * b, op);
        if (b == 0) fail(source_, op, "division by zero");
        return finite(a / b, op);
    }

    Eval expression(int minimum = 1) {
        auto left = operand();
        while (precedence(line_[i_].kind) >= minimum) {
            const auto& op = line_[i_++];
            auto right = expression(precedence(op.kind) + 1);
            left = apply(op, std::move(left), std::move(right));
        }
        return left;
    }
};

class Document {
public:
    Document(std::istream& input, const std::string& source)
        : lines_(read_lines(input, source)), source_(source) {}

    Eval parse() {
        if (lines_.empty()) return Eval{Eval::Map{}};
        const auto& first = lines_.front();
        if (first.front().column != 1 || !is_entry(first))
            fail(source_, first.front(), "expected a top-level mapping");

        auto& root = block(nullptr);
        for (auto& setting : settings_) bind(setting);
        for (auto& setting : settings_) {
            if (setting.target == &setting) {
                for (auto* input : setting.inputs) ++input->target->uses;
            }
        }
        evaluate(root);
        return std::move(root.result);
    }

private:
    std::vector<Line> lines_;
    const std::string& source_;
    std::deque<Setting> settings_;
    detail::Random random_;
    std::size_t i_ = 0;

    static bool is_entry(const Line& line) {
        return (line[0].kind == "name" || line[0].kind == "string") && line[1].kind == ":";
    }

    static bool is_item(const Line& line) { return line[0].kind == "-" && line[1].kind == "end"; }

    // Fold each name or dotted path into one token. A reference is bound after
    // all mappings have been indexed. Function names use a separate namespace.
    Setting& expression(const Line& line, std::size_t start, Setting* scope) {
        Line tokens;
        std::vector<Setting*> inputs;
        for (std::size_t i = start; i < line.size(); ++i) {
            tokens.push_back(line[i]);
            if (line[i].kind != "name") continue;
            auto& token = tokens.back();
            if (line[i + 1].kind == "(") {
                token.kind = "function";
                continue;
            }
            if (line[i + 1].kind == "." && line[i + 2].kind == "name" &&
                (line[i + 3].kind == "(" || line[i + 3].kind == "[")) {
                token.kind = "function";
                token.text += "." + line[i + 2].text;
                i += 2;
                continue;
            }
            auto* ref = &settings_.emplace_back(Kind::name, line[i], scope);
            while (line[i + 1].kind == ".") {
                if (line[i + 2].kind != "name") fail(source_, line[i + 2], "expected 'name'");
                i += 2;
                auto& field = settings_.emplace_back(Kind::field, line[i], scope);
                field.inputs.push_back(ref);
                ref = &field;
            }
            token.kind = "reference";
            token.reference = ref;
            inputs.push_back(ref);
        }
        // A (possibly parenthesized) alias can be followed to a mapping without
        // evaluating all its fields just to read one of them.
        std::size_t first = 0, end = tokens.size() - 1;
        while (end - first >= 2 && tokens[first].kind == "(" && tokens[end - 1].kind == ")") {
            ++first;
            --end;
        }
        if (end - first == 1 && tokens[first].kind == "reference")
            return *tokens[first].reference;
        auto& setting = settings_.emplace_back(Kind::expression, line[start], scope);
        setting.expression = std::move(tokens);
        setting.inputs = std::move(inputs);
        return setting;
    }

    Setting& value(const Line& line, std::size_t start, Setting* scope) {
        if (line[start].kind != "end") return expression(line, start, scope);
        if (i_ < lines_.size() && lines_[i_][0].column > line[0].column) return block(scope);
        return settings_.emplace_back(Kind::expression, line[0], scope);  // Empty entry is null.
    }

    Setting& block(Setting* scope) {
        const auto& first = lines_[i_];
        const auto column = first[0].column;
        const bool mapping = is_entry(first);
        if (!mapping && !is_item(first)) {
            ++i_;
            return expression(first, 0, scope);
        }
        auto& block = settings_.emplace_back(mapping ? Kind::mapping : Kind::list, first[0], scope);
        while (i_ < lines_.size() && lines_[i_][0].column >= column) {
            const auto& line = lines_[i_++];
            const auto& key = line[0];
            if (key.column != column) fail(source_, key, "unexpected indentation");
            if (mapping) {
                if (!is_entry(line)) fail(source_, key, "expected a mapping entry");
                if (block.fields.count(key.text)) fail(source_, key, "duplicate key '" + key.text + "'");
                auto& child = value(line, 2, &block);
                block.fields.emplace(key.text, &child);
                block.inputs.push_back(&child);
            } else {
                if (!is_item(line)) fail(source_, key, "expected a list item");
                block.inputs.push_back(&value(line, 1, scope));
            }
        }
        return block;
    }

    Setting& lookup(Setting& name) {
        static const Eval::Map constants = {
            {"null", Eval{nullptr}}, {"true", Eval{true}}, {"false", Eval{false}}, {"pi", Eval{pi}}
        };
        const auto& key = name.location.text;
        const auto constant = constants.find(key);
        if (constant != constants.end()) {
            name.result = constant->second;
            name.evaluated = true;
            name.target = &name;
            return name;
        }
        for (auto* scope = name.scope; scope; scope = scope->scope) {
            const auto it = scope->fields.find(key);
            if (it != scope->fields.end()) return *it->second;
        }
        fail(source_, name.location, "unknown reference '" + key + "'");
    }

    [[noreturn]] void cycle(const Token& at) const {
        fail(source_, at, "circular reference involving '" + at.text + "'");
    }

    // Resolve paths to definitions, descending through aliases one component at
    // a time. A reference to group.x depends only on x, even within group.
    void bind(Setting& start) {
        if (start.target) return;
        std::vector<Setting*> pending{&start};
        start.visiting = true;
        while (!pending.empty()) {
            auto& ref = *pending.back();
            Setting* next;
            if (ref.kind == Kind::name) {
                next = &lookup(ref);
            } else {
                next = ref.inputs.front();
                if (next->target) {
                    const auto& fields = next->target->fields;
                    const auto it = fields.find(ref.location.text);
                    if (it == fields.end())
                        fail(source_, ref.location, "unknown field '" + ref.location.text + "'");
                    next = it->second;
                }
            }
            if (next->target) {
                ref.target = next->target;
                ref.visiting = false;
                pending.pop_back();
            } else {
                if (next->visiting) cycle(ref.location);
                next->visiting = true;
                pending.push_back(next);
            }
        }
    }

    void evaluate(Setting& root) {
        struct Frame { Setting& setting; std::size_t next_input = 0; };
        std::vector<Frame> pending{{root}};
        root.visiting = true;
        while (!pending.empty()) {
            auto& frame = pending.back();
            auto& setting = frame.setting;
            if (frame.next_input < setting.inputs.size()) {
                auto* input = setting.inputs[frame.next_input++];
                auto& definition = *input->target;
                if (definition.visiting) cycle(input->location);
                if (!definition.evaluated) {
                    definition.visiting = true;
                    pending.push_back({definition});
                }
                continue;
            }
            if (setting.kind == Kind::mapping) {
                Eval::Map fields;
                for (const auto& [key, field] : setting.fields)
                    fields.emplace_hint(fields.end(), key, take_value(*field->target));
                setting.result = Eval{std::move(fields)};
            } else if (setting.kind == Kind::list) {
                Eval::List items;
                items.reserve(setting.inputs.size());
                for (auto* item : setting.inputs) items.push_back(take_value(*item->target));
                setting.result = Eval{std::move(items)};
            } else if (!setting.expression.empty()) {
                setting.result = Expression(setting.expression, source_, random_).parse();
            }
            setting.evaluated = true;
            setting.visiting = false;
            pending.pop_back();
        }
    }
};

}  // namespace

const Value& Value::at(const std::string& dotted_path) const {
    const Value* value = this;
    std::size_t start = 0;
    while (true) {
        const auto dot = dotted_path.find('.', start);
        const auto key = dotted_path.substr(start, dot - start);
        value = &value->as_map().at(key);
        if (dot == std::string::npos) return *value;
        start = dot + 1;
    }
}

std::ostream& operator<<(std::ostream& out, const Value& value) {
    if (value.is_null()) return out << "null";
    if (const auto* b = std::get_if<bool>(&value.data)) return out << (*b ? "true" : "false");
    if (const auto* n = std::get_if<Integer>(&value.data)) return out << *n;
    if (const auto* n = std::get_if<double>(&value.data)) return out << *n;
    if (const auto* text = std::get_if<std::string>(&value.data)) return out << *text;
    if (const auto* list = std::get_if<Value::List>(&value.data)) {
        out << '[';
        for (std::size_t i = 0; i < list->size(); ++i) {
            if (i != 0) out << ", ";
            out << (*list)[i];
        }
        return out << ']';
    }
    out << '{';
    bool first = true;
    for (const auto& [key, field] : value.as_map()) {
        if (!first) out << ", ";
        out << std::quoted(key) << ": " << field;
        first = false;
    }
    return out << '}';
}

Value loads(const std::string& text, const std::string& source) {
    std::istringstream input(text);
    return detail::materialize(Document(input, source).parse());
}

Value load(const std::string& path) {
    if (path.find('\0') != std::string::npos) throw ConfigError("NUL in file path");
#ifdef _WIN32
    std::ifstream input(std::filesystem::u8path(path), std::ios::binary);
#else
    std::ifstream input(path, std::ios::binary);
#endif
    if (!input) throw ConfigError(path + ": could not open file");
    return detail::materialize(Document(input, path).parse());
}

}  // namespace cfgy
