#include "dead_code.h"
#include <stdlib.h>
#include <string.h>

static int operand_is_temp(const IROperand* op) {
    if (!op) return 0;
    return op->type == OPERAND_TEMP && op->as.name && op->as.name[0] == 't';
}

static int operand_has_same_name(const IROperand* a, const IROperand* b) {
    if (!a || !b) return 0;

    if ((a->type != OPERAND_TEMP && a->type != OPERAND_VAR) ||
        (b->type != OPERAND_TEMP && b->type != OPERAND_VAR)) {
        return 0;
    }

    if (!a->as.name || !b->as.name) return 0;

    return strcmp(a->as.name, b->as.name) == 0;
}

static void mark_def_used_by_operand(const IRArray* ir, const IROperand* use, int* used) {
    if (!use || !used) return;

    if (use->type != OPERAND_TEMP && use->type != OPERAND_VAR) {
        return;
    }

    for (size_t j = 0; j < ir->count; j++) {
        const IRValue* def = &ir->value[j];

        switch (def->type) {
            case IR_BinaryOp:
                if (operand_has_same_name(&def->Binary.dst, use)) {
                    used[j] = 1;
                }
                break;

            case IR_Assign:
                if (operand_has_same_name(&def->Assign.dst, use)) {
                    used[j] = 1;
                }
                break;

            default:
                break;
        }
    }
}

static void mark_used(const IRArray* ir, int* used) {
    for (size_t i = 0; i < ir->count; i++) {
        const IRValue* v = &ir->value[i];

        switch (v->type) {
            case IR_BinaryOp:
                mark_def_used_by_operand(ir, &v->Binary.left, used);
                mark_def_used_by_operand(ir, &v->Binary.right, used);
                break;

            case IR_Assign:
                mark_def_used_by_operand(ir, &v->Assign.value, used);
                break;

            case IR_JumpIfFalse:
                mark_def_used_by_operand(ir, &v->JumpIfFalse.condition, used);
                break;

            case IR_Label:
            case IR_Jump:
            default:
                break;
        }
    }
}

static int instruction_defines_removable_temp(const IRValue* v) {
    if (!v) return 0;

    switch (v->type) {
        case IR_BinaryOp:
            return operand_is_temp(&v->Binary.dst);

        case IR_Assign:
            return operand_is_temp(&v->Assign.dst);

        default:
            return 0;
    }
}

static void free_ir_value_members(IRValue* v) {
    if (!v) return;

    switch (v->type) {
        case IR_BinaryOp:
            IRFreeOperand(&v->Binary.dst);
            IRFreeOperand(&v->Binary.left);
            IRFreeOperand(&v->Binary.right);
            break;

        case IR_Assign:
            IRFreeOperand(&v->Assign.dst);
            IRFreeOperand(&v->Assign.value);
            break;

        case IR_Label:
            IRFreeOperand(&v->Label.label);
            break;

        case IR_Jump:
            IRFreeOperand(&v->Jump.target);
            break;

        case IR_JumpIfFalse:
            IRFreeOperand(&v->JumpIfFalse.condition);
            IRFreeOperand(&v->JumpIfFalse.target);
            break;

        default:
            break;
    }
}

void eliminate_dead_code(IRArray* ir) {
    if (!ir || ir->count == 0) return;

    int changed;

    do {
        changed = 0;

        int* used = calloc(ir->count, sizeof(int));
        if (!used) {
            exit(EXIT_FAILURE);
        }

        mark_used(ir, used);

        size_t write = 0;
        for (size_t read = 0; read < ir->count; read++) {
            IRValue* v = &ir->value[read];

            if (instruction_defines_removable_temp(v) && !used[read]) {
                free_ir_value_members(v);
                changed = 1;
                continue;
            }

            if (write != read) {
                ir->value[write] = ir->value[read];
            }
            write++;
        }

        ir->count = write;
        free(used);

    } while (changed);
}
