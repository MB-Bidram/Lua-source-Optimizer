#include "constant_fold.h"
#include "../utils.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* name;
    int known;
    IROperand value;
} ConstEntry;

typedef struct {
    ConstEntry* data;
    size_t count;
    size_t capacity;
} ConstTable;

static int operand_is_temp(const IROperand* op) {
    return op && op->type == OPERAND_TEMP && op->as.name;
}

static int operand_has_name(const IROperand* op) {
    if (!op) return 0;
    return op->type == OPERAND_TEMP || op->type == OPERAND_VAR;
}

static const char* operand_name(const IROperand* op) {
    if (!operand_has_name(op)) return NULL;
    return op->as.name;
}

static int operand_is_numeric_const(const IROperand* op, double* out) {
    if (!op || !out) return 0;
    if (op->type != OPERAND_CONST_NUM) return 0;
    *out = op->as.number;
    return 1;
}

static int operand_is_bool_const(const IROperand* op, int* out) {
    if (!op || !out) return 0;
    if (op->type != OPERAND_CONST_BOOL) return 0;
    *out = op->as.boolean;
    return 1;
}

static void const_table_init(ConstTable* table) {
    table->count = 0;
    table->capacity = 16;
    table->data = malloc(sizeof(ConstEntry) * table->capacity);
    if (!table->data) {
        perror("ConstTable allocation failed");
        exit(EXIT_FAILURE);
    }
}

static void const_table_free(ConstTable* table) {
    if (!table) return;

    for (size_t i = 0; i < table->count; i++) {
        free(table->data[i].name);
        if (table->data[i].known) {
            IRFreeOperand(&table->data[i].value);
        }
    }

    free(table->data);
    table->data = NULL;
    table->count = 0;
    table->capacity = 0;
}

static void const_table_grow(ConstTable* table) {
    if (table->count < table->capacity) return;

    table->capacity *= 2;
    ConstEntry* newData = realloc(table->data, sizeof(ConstEntry) * table->capacity);
    if (!newData) {
        perror("ConstTable reallocation failed");
        exit(EXIT_FAILURE);
    }

    table->data = newData;
}

static ConstEntry* const_table_find(ConstTable* table, const char* name) {
    if (!table || !name) return NULL;

    for (size_t i = 0; i < table->count; i++) {
        if (strcmp(table->data[i].name, name) == 0) {
            return &table->data[i];
        }
    }

    return NULL;
}

static void const_table_set(ConstTable* table, const char* name, const IROperand* value) {
    ConstEntry* entry;

    if (!table || !name || !value) return;

    entry = const_table_find(table, name);
    if (entry) {
        if (entry->known) {
            IRFreeOperand(&entry->value);
        }
        entry->known = 1;
        entry->value = IRCopyOperand(value);
        return;
    }

    const_table_grow(table);

    table->data[table->count].name = strdup(name);
    table->data[table->count].known = 1;
    table->data[table->count].value = IRCopyOperand(value);
    table->count++;
}

static void const_table_kill(ConstTable* table, const char* name) {
    ConstEntry* entry;

    if (!table || !name) return;

    entry = const_table_find(table, name);
    if (entry && entry->known) {
        IRFreeOperand(&entry->value);
        entry->known = 0;
    }
}

static void const_table_clear(ConstTable* table) {
    if (!table) return;

    for (size_t i = 0; i < table->count; i++) {
        if (table->data[i].known) {
            IRFreeOperand(&table->data[i].value);
            table->data[i].known = 0;
        }
    }
}

static int const_table_get(ConstTable* table, const char* name, IROperand* out) {
    ConstEntry* entry;

    if (!table || !name || !out) return 0;

    entry = const_table_find(table, name);
    if (!entry || !entry->known) return 0;

    *out = IRCopyOperand(&entry->value);
    return 1;
}

static int compute_binary(IRBinaryOpKind op, double left, double right, IROperand* out) {
    if (!out) return 0;

    switch (op) {
        case IR_OP_ADD:
            *out = IRMakeConstNum(left + right);
            return 1;

        case IR_OP_SUB:
            *out = IRMakeConstNum(left - right);
            return 1;

        case IR_OP_MUL:
            *out = IRMakeConstNum(left * right);
            return 1;

        case IR_OP_DIV:
            if (right == 0.0) return 0;
            *out = IRMakeConstNum(left / right);
            return 1;

        case IR_OP_MOD:
            if (right == 0.0) return 0;
            *out = IRMakeConstNum(fmod(left, right));
            return 1;

        case IR_OP_LT:
            *out = IRMakeConstBool(left < right);
            return 1;

        case IR_OP_GT:
            *out = IRMakeConstBool(left > right);
            return 1;

        case IR_OP_LE:
            *out = IRMakeConstBool(left <= right);
            return 1;

        case IR_OP_GE:
            *out = IRMakeConstBool(left >= right);
            return 1;

        case IR_OP_EQ:
            *out = IRMakeConstBool(left == right);
            return 1;

        case IR_OP_NE:
            *out = IRMakeConstBool(left != right);
            return 1;

        default:
            return 0;
    }
}

static int operand_is_foldable_const(const IROperand* op) {
    if (!op) return 0;
    return op->type == OPERAND_CONST_NUM ||
           op->type == OPERAND_CONST_BOOL ||
           op->type == OPERAND_NIL;
}

static void replace_operand_if_known(ConstTable* table, IROperand* op) {
    IROperand known;

    if (!operand_is_temp(op)) return;

    if (const_table_get(table, op->as.name, &known)) {
        IRFreeOperand(op);
        *op = known;
    }
}

void fold_constants(IRArray* ir) {
    ConstTable table;

    if (!ir) return;

    const_table_init(&table);

    for (size_t i = 0; i < ir->count; i++) {
        IRValue* v = &ir->value[i];

        switch (v->type) {
            case IR_BinaryOp: {
                IROperand folded;
                double leftValue, rightValue;

                replace_operand_if_known(&table, &v->Binary.left);
                replace_operand_if_known(&table, &v->Binary.right);

                if (operand_is_numeric_const(&v->Binary.left, &leftValue) &&
                    operand_is_numeric_const(&v->Binary.right, &rightValue) &&
                    compute_binary(v->Binary.op, leftValue, rightValue, &folded)) {

                    IROperand old_dst = v->Binary.dst;
                    IROperand old_left = v->Binary.left;
                    IROperand old_right = v->Binary.right;

                    v->type = IR_Assign;
                    v->Assign.dst = old_dst;
                    v->Assign.value = folded;
                    v->Assign.isLocal = 0;

                    if (operand_is_temp(&v->Assign.dst)) {
                        const_table_set(&table, v->Assign.dst.as.name, &v->Assign.value);
                    }

                    IRFreeOperand(&old_left);
                    IRFreeOperand(&old_right);
                } else {
                    if (operand_is_temp(&v->Binary.dst)) {
                        const_table_kill(&table, v->Binary.dst.as.name);
                    }
                }

                break;
            }

            case IR_Assign: {
                replace_operand_if_known(&table, &v->Assign.value);

                if (operand_is_temp(&v->Assign.dst)) {
                    const char* dst_name = v->Assign.dst.as.name;

                    if (operand_is_foldable_const(&v->Assign.value)) {
                        const_table_set(&table, dst_name, &v->Assign.value);
                    } else {
                        const_table_kill(&table, dst_name);
                    }
                }

                break;
            }

            case IR_JumpIfFalse: {
                replace_operand_if_known(&table, &v->JumpIfFalse.condition);

                /* control flow split: facts after this point are no longer linear-safe */
                const_table_clear(&table);
                break;
            }

            case IR_Jump: {
                /* unconditional control transfer */
                const_table_clear(&table);
                break;
            }

            case IR_Label: {
                /* join point: different predecessors may reach here */
                const_table_clear(&table);
                break;
            }

            default:
                break;
        }
    }

    const_table_free(&table);
}
