#include "evaluation.hpp"

#include <charconv>
#include <cmath>
#include <filesystem>
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
using Scopes = std::vector<const Eval::Map*>;
constexpr auto min_int = std::numeric_limits<Integer>::min();
constexpr auto max_int = std::numeric_limits<Integer>::max();
const double pi = std::acos(-1.0);

struct Token {
    std::string kind;
    std::string text;
    std::size_t line;
    std::size_t column;
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
        } else if (std::string_view(":[],().+-*/").find(c) != std::string_view::npos) {
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
    if (op == "*" || op == "/") return 2;
    return 0;
}

class Expression {
public:
    Expression(const Line& line, std::size_t start, const Scopes& scopes, const std::string& source,
               detail::Random& random)
        : line_(line), i_(start), scopes_(scopes), source_(source), random_(random) {}

    Eval parse() {
        auto value = expression();
        require("end");
        return value;
    }

private:
    const Line& line_;
    std::size_t i_;
    const Scopes& scopes_;
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
            {"torad", [](double x) { return x * (pi / 180.0); }},
            {"todeg", [](double x) { return x * (180.0 / pi); }},
            {"abs", std::fabs}, {"sin", std::sin}, {"cos", std::cos}, {"tan", std::tan},
            {"sqrt", std::sqrt}, {"floor", std::floor}, {"ceil", std::ceil}
        };
        auto args = arguments(")");
        try {
            if (name.text == "matrix" || name.text == "transpose") {
                if (args.size() != 1) fail(source_, name, name.text + " expects one matrix or list of rows");
                auto matrix = detail::as_matrix(std::move(args.front()));
                if (name.text == "transpose") return Eval{detail::transpose(matrix)};
                return Eval{std::move(matrix)};
            }
            if (name.text.compare(0, 4, "Rot.") == 0) return detail::rotation(name.text, args);
            if (name.text.compare(0, 5, "Rand.") == 0) return random_.call(name.text, args);
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

    Eval named(const Token& name) {
        if (accept("(")) return call(name);
        if (line_[i_].kind == "." && line_[i_ + 1].kind == "name" && line_[i_ + 2].kind == "(") {
            Token function = name;
            function.text += "." + line_[i_ + 1].text;
            i_ += 3;
            return call(function);
        }
        static const Eval::Map constants = {
            {"null", Eval{nullptr}}, {"true", Eval{true}}, {"false", Eval{false}}, {"pi", Eval{pi}}
        };
        const Eval* value = nullptr;
        const auto constant = constants.find(name.text);
        if (constant != constants.end()) value = &constant->second;
        for (auto s = scopes_.rbegin(); !value && s != scopes_.rend(); ++s) {
            const auto it = (*s)->find(name.text);
            if (it != (*s)->end()) value = &it->second;
        }
        if (!value) fail(source_, name, "unknown reference '" + name.text + "'");
        while (accept(".")) {
            const auto& key = line_[i_];
            require("name");
            const auto* map = std::get_if<Eval::Map>(&value->data);
            if (!map || !map->count(key.text)) fail(source_, key, "unknown field '" + key.text + "'");
            value = &map->at(key.text);
        }
        return *value;
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
        if (accept("name")) return named(token);
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
        if (detail::is_matrix_operand(left) || detail::is_matrix_operand(right))
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
        if (first.front().column != 1 || !is_entry(first)) {
            fail(source_, first.front(), "expected a top-level mapping");
        }
        return block();
    }

private:
    std::vector<Line> lines_;
    const std::string& source_;
    Scopes scopes_;
    detail::Random random_;
    std::size_t i_ = 0;

    static bool is_entry(const Line& line) {
        return (line[0].kind == "name" || line[0].kind == "string") && line[1].kind == ":";
    }

    static bool is_item(const Line& line) { return line[0].kind == "-" && line[1].kind == "end"; }

    Eval value(const Line& line, std::size_t start) {
        if (line[start].kind != "end") return Expression(line, start, scopes_, source_, random_).parse();
        if (i_ < lines_.size() && lines_[i_][0].column > line[0].column) return block();
        return Eval{nullptr};
    }

    Eval block() {
        const auto& first = lines_[i_];
        const auto column = first[0].column;
        const bool mapping = is_entry(first);
        if (!mapping && !is_item(first)) {
            ++i_;
            return Expression(first, 0, scopes_, source_, random_).parse();
        }
        Eval::Map fields;
        Eval::List items;
        if (mapping) scopes_.push_back(&fields);
        while (i_ < lines_.size() && lines_[i_][0].column >= column) {
            const auto& line = lines_[i_++];
            const auto& key = line[0];
            if (key.column != column) fail(source_, key, "unexpected indentation");
            if (mapping) {
                if (!is_entry(line)) fail(source_, key, "expected a mapping entry");
                if (fields.count(key.text)) fail(source_, key, "duplicate key '" + key.text + "'");
                fields.emplace(key.text, value(line, 2));
            } else {
                if (!is_item(line)) fail(source_, key, "expected a list item");
                items.push_back(value(line, 1));
            }
        }
        if (mapping) {
            scopes_.pop_back();
            return Eval{std::move(fields)};
        }
        return Eval{std::move(items)};
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
    std::ifstream input(std::filesystem::u8path(path), std::ios::binary);
    if (!input) throw ConfigError(path + ": could not open file");
    return detail::materialize(Document(input, path).parse());
}

}  // namespace cfgy
