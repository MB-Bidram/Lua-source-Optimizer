#ifndef AST_H
#define AST_H

#include "common.h"

typedef enum {
    astNone = 0,
    AST_Number,
    AST_String,
    AST_Boolean,
    AST_Nil,
    AST_Identifier,

    AST_BinaryOp,
    AST_UnaryOp,

    AST_Block,
    AST_Call,
    AST_Return,
    AST_If,
    AST_Assign
} astType;

typedef struct astNode {
    astType type;

    union {
        double number;
        char* string;
        unsigned short int boolean;

        struct {
            struct astNode* left;
            struct astNode* right;
            char* op;
        } Binary;

        struct {
            struct astNode* value;
            char* name;
            int isLocal;
        } Assign;

        struct {
            struct astNode** stmts;
            size_t count;
        } Block;

        struct {
            struct astNode* condition;
            struct astNode* thenBlock;
            struct astNode* elseBlock;
        } If;
    };
} astNode;

const char* AstToString(astType AstType);
void PrintMyAST(astNode* node, int indent);
void AstCleanup(astNode* node);
astNode* ast_alloc(void);

#endif
