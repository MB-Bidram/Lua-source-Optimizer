#ifndef LUAVERSIONS_H
#define LUAVERSIONS_H

#include "common.h"

typedef enum {
    LUA_51 = 0,
    LUA_52,
    LUA_53,
    LUA_54
} LuaVersion;

typedef struct {
    LuaVersion version;
    const char* name;

    int has_goto;
    int has_labels;
    int has_bitwise;
    int has_close_attr;
    int has_const_attr;
    int has_integer_div;
    int has_shift_ops;
    int has_floor_div;
    int has_vararg;
} LuaVersionInfo;

const LuaVersionInfo* get_lua_version_info(LuaVersion version);

#endif
