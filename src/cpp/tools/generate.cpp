#include <cfgy>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string_view>

namespace {

bool usable_name(const std::string& name) {
    constexpr std::string_view letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
    constexpr std::string_view keywords =
        " alignas alignof and and_eq asm auto bitand bitor bool break case catch char char8_t "
        "char16_t char32_t class compl concept const consteval constexpr constinit const_cast "
        "continue co_await co_return co_yield decltype default delete do double dynamic_cast else "
        "enum explicit export extern false float for friend goto if inline int long mutable "
        "namespace new noexcept not not_eq nullptr operator or or_eq private protected public "
        "register reinterpret_cast requires return short signed sizeof static static_assert "
        "static_cast struct switch template this thread_local throw true try typedef typeid "
        "typename union unsigned using virtual void volatile wchar_t while xor xor_eq ";
    if (name.empty() || letters.find(name.front()) == letters.npos) return false;
    if (name.find("__") != name.npos || (name.size() > 1 && name[0] == '_' &&
        name[1] >= 'A' && name[1] <= 'Z')) return false;
    if (keywords.find(" " + name + " ") != keywords.npos) return false;
    return name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789") == name.npos;
}

std::string field_type(const cfgy::Value& value) {
    if (value.is_null()) return "::std::nullptr_t";
    if (std::holds_alternative<bool>(value.data)) return "bool";
    if (std::holds_alternative<std::int64_t>(value.data)) return "::std::int64_t";
    if (std::holds_alternative<double>(value.data)) return "double";
    if (std::holds_alternative<std::string>(value.data)) return "::std::string";
    return "::cfgy::Value::List";
}

// A sample can establish an element type only if it contains matching scalars.
std::string element_type(const cfgy::Value& value) {
    const auto* list = std::get_if<cfgy::Value::List>(&value.data);
    if (!list || list->empty()) return {};
    const auto& first = list->front();
    if (std::holds_alternative<cfgy::Value::List>(first.data) ||
        std::holds_alternative<cfgy::Value::Map>(first.data)) return {};
    const bool uniform = std::all_of(list->begin(), list->end(), [&](const auto& item) {
        return item.data.index() == first.data.index();
    });
    return uniform ? field_type(first) : "";
}

void members(const cfgy::Value::Map& map, const std::string& indent, const std::string& parent_keys,
             bool infer_types, std::ostream& declarations, std::ostream& initializers) {
    for (const auto& [name, value] : map) {
        if (!usable_name(name)) throw cfgy::ConfigError("unusable C++ member name '" + name + "'");
        const auto keys = parent_keys + "\"" + name + "\"";
        if (const auto* child = std::get_if<cfgy::Value::Map>(&value.data)) {
            declarations << indent << "struct {\n";
            initializers << indent << "    {\n";
            members(*child, indent + "    ", keys + ", ", infer_types, declarations, initializers);
            declarations << indent << "} " << name << ";\n";
            initializers << indent << "    },\n";
        } else {
            const auto element = element_type(value);
            const auto scalar = infer_types ? field_type(value) : "::cfgy::Value";
            const auto type = element.empty() ? scalar : "::std::vector<" + element + ">";
            const auto lookup = element.empty() ? "field<" + type : "list_field<" + element;
            declarations << indent << type << " " << name << ";\n";
            initializers << indent << "    detail::" << lookup << ">(root, source, {" << keys << "}),\n";
        }
    }
}

// Build the member tree from application field paths when no sample file exists.
cfgy::Value::Map select_fields(char** first, char** last) {
    cfgy::Value::Map root;
    for (; first != last; ++first) {
        const std::string path = *first;
        auto* map = &root;
        std::size_t start = 0;
        while (true) {
            const auto dot = path.find('.', start);
            const auto name = path.substr(start, dot - start);
            if (!usable_name(name)) throw cfgy::ConfigError("invalid field path '" + path + "'");
            if (dot == path.npos) {
                if (!map->emplace(name, cfgy::Value{nullptr}).second)
                    throw cfgy::ConfigError("overlapping field path '" + path + "'");
                break;
            }
            auto& child = map->try_emplace(name, cfgy::Value{cfgy::Value::Map{}}).first->second;
            map = std::get_if<cfgy::Value::Map>(&child.data);
            if (!map) throw cfgy::ConfigError("overlapping field path '" + path + "'");
            start = dot + 1;
        }
    }
    return root;
}

void generate(const cfgy::Value::Map& fields, const std::string& type, bool infer_types) {
    if (!usable_name(type) || type[0] == '_' || type == "std" || type == "cfgy") {
        throw cfgy::ConfigError("unusable C++ type name '" + type + "'");
    }
    std::ostringstream declarations, initializers;
    members(fields, "    ", "", infer_types, declarations, initializers);
    const bool empty = fields.empty();
    std::cout << "// Generated by cfgy-gen.\n"
              << "#pragma once\n\n#include <cfgy>\n\n"
              << "struct " << type << " {\n" << declarations.str() << "};\n\n"
              << "template<>\ninline ::" << type << " cfgy::decode<::" << type << ">("
              << "const Value&" << (empty ? "" : " root") << ", const std::string&"
              << (empty ? "" : " source") << ") {\n"
              << "    return {\n" << initializers.str() << "    };\n}\n";
}

}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc >= 4 && std::string_view(argv[1]) == "--fields") {
            generate(select_fields(argv + 3, argv + argc), argv[2], false);
        } else if (argc == 3 && std::string_view(argv[1]) != "--fields") {
            generate(cfgy::load(argv[1]).as_map(), argv[2], true);
        } else {
            std::cerr << "Usage: cfgy-gen --fields <CppTypeName> <field.path> ...\n"
                      << "       cfgy-gen <sample.cfgy> <CppTypeName>\n";
            return 1;
        }
    } catch (const cfgy::ConfigError& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return std::cout ? 0 : 1;
}
