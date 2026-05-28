#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include "luaVersions.h"

typedef struct {
    TokenArray* array;
    size_t pos;
    LuaVersion version;
    const LuaVersionInfo* vinfo;
} parseValue;

astNode* parse(TokenArray* lexed, LuaVersion version);

#endif
