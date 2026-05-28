#ifndef IR_H
#define IR_H

#include "ast.h"

typedef enum {
    IR_BinaryOp,
    IR_Assign,
    IR_Label,
    IR_Jump,
    IR_JumpIfFalse
} IRType;

typedef enum {
    IR_OP_ADD,
    IR_OP_SUB,
    IR_OP_MUL,
    IR_OP_DIV,
    IR_OP_MOD,
    IR_OP_LT,
    IR_OP_GT,
    IR_OP_LE,
    IR_OP_GE,
    IR_OP_EQ,
    IR_OP_NE
} IRBinaryOpKind;


typedef enum {
    OPERAND_TEMP,
    OPERAND_VAR,
    OPERAND_CONST_NUM,
    OPERAND_CONST_BOOL,
    OPERAND_NIL,
    OPERAND_LABEL
} IROperandType;

typedef struct {
    IROperandType type;
    union {
        char* name;    
        double number; 
        int boolean;   
    } as;
} IROperand;


typedef struct {
    IRType type;

    union {
        struct {
            IROperand dst;
            IRBinaryOpKind op;
            IROperand left;
            IROperand right;
        } Binary;

        struct {
            IROperand dst;     // usually a variable
            IROperand value;
            int isLocal;
        } Assign;

        struct {
            IROperand label;
        } Label;

        struct {
            IROperand target;  // label operand
        } Jump;

        struct {
            IROperand condition;
            IROperand target;  // label operand
        } JumpIfFalse;
    };
} IRValue;


typedef struct {
    IRValue* value;
    size_t capacity;
    size_t count;
} IRArray;


void IRAlloc(IRArray* ir);
void IRGrow(IRArray* ir);
void IREmit(IRArray* ir, IRValue value);

void IRFreeOperand(IROperand* op);
IROperand IRCopyOperand(const IROperand* op);
void IRPrintOperand(const IROperand* op);

IRBinaryOpKind IRMapBinaryOp(const char* op);
const char* IRBinaryOpName(IRBinaryOpKind op);


IROperand IRMakeTemp(char* name);
IROperand IRMakeVar(const char* name);
IROperand IRMakeConstNum(double value);
IROperand IRMakeConstBool(int value);
IROperand IRMakeNil(void);
IROperand IRMakeLabel(const char* name);

IROperand newTempOperand(void);

void IREmitLabel(IRArray* ir, IROperand label);
void IREmitJump(IRArray* ir, IROperand target);
void IREmitJumpIfFalse(IRArray* ir, IROperand condition, IROperand target);

IROperand IRGen(astNode* node, IRArray* ir);

void IRGenIf(IRArray* ir, astNode* node);
void IRStmtGen(IRArray* ir, astNode* node);

void IRPrint(const IRArray* ir);
void IRCleanup(IRArray* ir);

#endif
