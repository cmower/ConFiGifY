#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include "cfgy.h"

#if LUA_MAXINTEGER < INT64_MAX
#error "ConFiGifY requires a Lua build with 64-bit integers"
#endif
static char null_sentinel;

static void push_value(lua_State *L, const cfgy_value *n) {
    luaL_checkstack(L, 4, "configuration nesting too deep");
    switch (n->kind) {
        case CFGY_NULL: lua_pushlightuserdata(L, &null_sentinel); break;
        case CFGY_BOOL: lua_pushboolean(L, n->integer != 0); break;
        case CFGY_INT: lua_pushinteger(L, (lua_Integer)n->integer); break;
        case CFGY_FLOAT: lua_pushnumber(L, n->number); break;
        case CFGY_STRING: lua_pushlstring(L, n->string.data, n->string.size); break;
        case CFGY_LIST: case CFGY_MAP:
            lua_newtable(L);
            luaL_setmetatable(L, n->kind == CFGY_MAP ? "cfgy.map" : "cfgy.list");
            for (size_t i = 0; i < n->count; ++i) {
                const cfgy_value *child = &n->children[i];
                if (n->kind == CFGY_MAP) lua_pushlstring(L, child->key.data, child->key.size);
                else lua_pushinteger(L, (lua_Integer)i + 1);
                push_value(L, child);
                lua_rawset(L, -3);
            }
            break;
    }
}

static int unpack(lua_State *L) {
    cfgy_document *doc = lua_touserdata(L, 1);
    if (cfgy_error(doc)) return luaL_error(L, "%s", cfgy_error(doc));
    push_value(L, cfgy_root(doc));
    return 1;
}

/* Protect the allocation and conversion of Lua objects so errors release doc. */
static int finish(lua_State *L, cfgy_document *doc) {
    lua_pushlightuserdata(L, doc);
    int status = lua_pcall(L, 1, 1, 0);
    cfgy_free(doc);
    return status == LUA_OK ? 1 : lua_error(L);
}

static int load_file(lua_State *L) {
    size_t length;
    const char *path = luaL_checklstring(L, 1, &length);
    luaL_argcheck(L, length == strlen(path), 1, "NUL in file path");
    lua_pushcfunction(L, unpack);
    return finish(L, cfgy_load(path));
}
static int load_text(lua_State *L) {
    size_t length, name_size;
    const char *data = luaL_checklstring(L, 1, &length);
    const char *source = luaL_optlstring(L, 2, "<string>", &name_size);
    luaL_argcheck(L, name_size == strlen(source), 2, "NUL in source name");
    lua_pushcfunction(L, unpack);
    return finish(L, cfgy_parse(data, length, source));
}

#ifdef _WIN32
__declspec(dllexport)
#endif
int luaopen_cfgy(lua_State *L) {
    const luaL_Reg functions[] = {{"load", load_file}, {"loads", load_text}, {NULL, NULL}};
    luaL_newmetatable(L, "cfgy.list"); lua_pop(L, 1);
    luaL_newmetatable(L, "cfgy.map"); lua_pop(L, 1);
    luaL_newlib(L, functions);
    lua_pushlightuserdata(L, &null_sentinel);
    lua_setfield(L, -2, "null");
    return 1;
}
