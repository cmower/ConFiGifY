#include "cfgy.h"
#include <cfgy>

#include <algorithm>
#include <memory>
#include <new>
#include <string_view>

struct cfgy_document {
    cfgy::Value value;
    cfgy_value root{};
    std::vector<std::unique_ptr<cfgy_value[]>> arrays;
    std::string error;

    void expose(const cfgy::Value& value, cfgy_value& out) {
        if (value.is_null()) {
            out.kind = CFGY_NULL;
        } else if (const auto* b = std::get_if<bool>(&value.data)) {
            out.kind = CFGY_BOOL;
            out.integer = *b;
        } else if (const auto* i = std::get_if<std::int64_t>(&value.data)) {
            out.kind = CFGY_INT;
            out.integer = *i;
        } else if (const auto* f = std::get_if<double>(&value.data)) {
            out.kind = CFGY_FLOAT;
            out.number = *f;
        } else if (const auto* s = std::get_if<std::string>(&value.data)) {
            out.kind = CFGY_STRING;
            out.string = {s->data(), s->size()};
        } else {
            const auto* list = std::get_if<cfgy::Value::List>(&value.data);
            out.kind = list ? CFGY_LIST : CFGY_MAP;
            out.count = list ? list->size() : value.as_map().size();
            if (!out.count) return;
            auto* children = arrays.emplace_back(std::make_unique<cfgy_value[]>(out.count)).get();
            out.children = children;
            if (list) {
                for (size_t i = 0; i < out.count; ++i) expose((*list)[i], children[i]);
            } else {
                for (const auto& [key, child] : value.as_map()) {
                    children->key = {key.data(), key.size()};
                    expose(child, *children++);
                }
            }
        }
    }
};

namespace {
template<class Parse> cfgy_document* read_document(Parse parse) noexcept {
    try {
        auto doc = std::make_unique<cfgy_document>();
        try {
            doc->value = parse();
            doc->expose(doc->value, doc->root);
        } catch (const std::exception& e) {
            doc->error = e.what();
        }
        return doc.release();
    } catch (...) {
        return nullptr;
    }
}
}

cfgy_document* cfgy_load(const char* path) {
    return read_document([&] {
        if (!path) throw cfgy::ConfigError("null file path");
        return cfgy::load(path);
    });
}

cfgy_document* cfgy_parse(const char* text, size_t length, const char* source) {
    return read_document([&] {
        if (!text && length) throw cfgy::ConfigError("null input");
        return cfgy::loads(std::string(text ? text : "", length), source ? source : "<string>");
    });
}

const char* cfgy_error(const cfgy_document* doc) {
    if (!doc) return "out of memory";
    return doc->error.empty() ? nullptr : doc->error.c_str();
}
const cfgy_value* cfgy_root(const cfgy_document* doc) {
    return cfgy_error(doc) ? nullptr : &doc->root;
}
void cfgy_free(cfgy_document* doc) { delete doc; }

const cfgy_value* cfgy_get(const cfgy_value* map, const char* key) {
    if (!map || map->kind != CFGY_MAP || !map->count || !key) return nullptr;
    const auto end = map->children + map->count;
    const auto found = std::lower_bound(map->children, end, std::string_view(key),
        [](const cfgy_value& node, std::string_view name) {
            return std::string_view(node.key.data, node.key.size) < name;
        });
    return found != end && std::string_view(found->key.data, found->key.size) == key ? found : nullptr;
}
