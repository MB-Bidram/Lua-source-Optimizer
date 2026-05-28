#ifndef LEXER_H
#define LEXER_H

#include "common.h"
#include "luaVersions.h"

typedef enum {
    TokenNone = 0,

    Keyword,
    Number,
    String,
    Identifier,
    Boolean,
    Nil,

    BinaryOp,
    UnaryOp,

    OpenParen,
    CloseParen,
    OpenBrace,
    CloseBrace,
    OpenBracket,
    CloseBracket,

    Comma,
    Semicolon,
    Dot,
    Colon,

    Equal,
    EqualEqual,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    Concat,
    VarArg,
    DoubleColon,
    FloorDiv,
    ShiftLeft,
    ShiftRight,

    Eof,
} TokenType;

typedef struct type {
    union {
        TokenType Type;
    };
} type;

typedef struct lexValue {
    type t;
    SourcePos pos;

    union {
        char* string;
        double number;
        unsigned short int boolean;
        char operator;
    } v;
} lexValue;

typedef struct TokenArray {
    lexValue* data;
    size_t count;
    LuaVersion version;
} TokenArray;

const char* TokenToString(TokenType token);
TokenArray* lex(const char* str, LuaVersion version);
void printMyTokens(const TokenArray* array);
void TokenCleanup(TokenArray* array);

#endif
