#include "luaVersions.h"

static const LuaVersionInfo LUA_VERSION_TABLE[] = {
    [LUA_51] = {
        LUA_51, "Lua 5.1",
        0, /* has_goto */
        0, /* has_labels */
        0, /* has_bitwise */
        0, /* has_close_attr */
        0, /* has_const_attr */
        0, /* has_integer_div */
        0, /* has_shift_ops */
        0, /* has_floor_div */
        1  /* has_vararg */
    },
    [LUA_52] = {
        LUA_52, "Lua 5.2",
        1,
        1,
        0,
        0,
        0,
        0,
        0,
        0,
        1
    },
    [LUA_53] = {
        LUA_53, "Lua 5.3",
        1,
        1,
        1,
        0,
        0,
        1,
        1,
        1,
        1
    },
    [LUA_54] = {
        LUA_54, "Lua 5.4",
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1
    }
};

const LuaVersionInfo* get_lua_version_info(LuaVersion version) {
    if (version < LUA_51 || version > LUA_54) {
        return &LUA_VERSION_TABLE[LUA_54];
    }
    return &LUA_VERSION_TABLE[version];
}
