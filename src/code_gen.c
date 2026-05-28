#include "code_gen.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* name;
    char* expr;
} ExprEntry;

typedef struct {
    ExprEntry* data;
    size_t count;
    size_t capacity;
} ExprTable;

typedef struct {
    const IRArray* ir;
    FILE* out;
    ExprTable exprs;
    int indent;
} LuaGenContext;

/* =========================
   Expression table helpers
   ========================= */

static void expr_table_init(ExprTable* table) {
    table->count = 0;
    table->capacity = 16;
    table->data = malloc(sizeof(ExprEntry) * table->capacity);
    if (!table->data) {
        perror("ExprTable allocation failed");
        exit(EXIT_FAILURE);
    }
}

static void expr_table_free(ExprTable* table) {
    if (!table) return;

    for (size_t i = 0; i < table->count; i++) {
        free(table->data[i].name);
        free(table->data[i].expr);
    }

    free(table->data);
    table->data = NULL;
    table->count = 0;
    table->capacity = 0;
}

static void expr_table_grow(ExprTable* table) {
    if (table->count < table->capacity) return;

    table->capacity *= 2;
    ExprEntry* newData = realloc(table->data, sizeof(ExprEntry) * table->capacity);
    if (!newData) {
        perror("ExprTable reallocation failed");
        exit(EXIT_FAILURE);
    }

    table->data = newData;
}

static ExprEntry* expr_table_find(ExprTable* table, const char* name) {
    if (!table || !name) return NULL;

    for (size_t i = 0; i < table->count; i++) {
        if (strcmp(table->data[i].name, name) == 0) {
            return &table->data[i];
        }
    }

    return NULL;
}

static void expr_table_set(ExprTable* table, const char* name, const char* expr) {
    ExprEntry* entry;

    if (!table || !name || !expr) return;

    entry = expr_table_find(table, name);
    if (entry) {
        free(entry->expr);
        entry->expr = strdup(expr);
        return;
    }

    expr_table_grow(table);
    table->data[table->count].name = strdup(name);
    table->data[table->count].expr = strdup(expr);
    table->count++;
}

static void expr_table_kill(ExprTable* table, const char* name) {
    ExprEntry* entry;

    if (!table || !name) return;

    entry = expr_table_find(table, name);
    if (!entry) return;

    free(entry->expr);
    entry->expr = NULL;
}

static const char* expr_table_get(ExprTable* table, const char* name) {
    ExprEntry* entry;

    if (!table || !name) return NULL;

    entry = expr_table_find(table, name);
    if (!entry) return NULL;

    return entry->expr;
}

/* =========================
   Formatting helpers
   ========================= */

static void emit_indent(FILE* out, int indent) {
    for (int i = 0; i < indent; i++) {
        fputs("    ", out);
    }
}

static char* make_number_expr(double value) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.17g", value);
    return strdup(buffer);
}

static char* make_bool_expr(int value) {
    return strdup(value ? "true" : "false");
}

static char* make_nil_expr(void) {
    return strdup("nil");
}

static char* make_name_expr(const char* name) {
    if (!name) return strdup("nil");
    return strdup(name);
}

static char* make_binary_expr(const char* left, const char* op, const char* right) {
    size_t len;
    char* result;

    if (!left || !op || !right) {
        return strdup("nil");
    }

    len = strlen(left) + strlen(op) + strlen(right) + 6;
    result = malloc(len);
    if (!result) {
        perror("make_binary_expr allocation failed");
        exit(EXIT_FAILURE);
    }

    snprintf(result, len, "(%s %s %s)", left, op, right);
    return result;
}

static int is_temp_operand(const IROperand* op) {
    if (!op || op->type != OPERAND_TEMP || !op->as.name) return 0;

    if (op->as.name[0] != 't') return 0;

    for (size_t i = 1; op->as.name[i] != '\0'; i++) {
        if (op->as.name[i] < '0' || op->as.name[i] > '9') {
            return 0;
        }
    }

    return 1;
}

static int operand_has_name(const IROperand* op) {
    if (!op) return 0;
    return op->type == OPERAND_TEMP || op->type == OPERAND_VAR || op->type == OPERAND_LABEL;
}

static const char* operand_name(const IROperand* op) {
    if (!operand_has_name(op)) return NULL;
    return op->as.name;
}

static char* resolve_operand_expr(ExprTable* table, const IROperand* op) {
    const char* found;

    if (!op) return strdup("nil");

    switch (op->type) {
        case OPERAND_CONST_NUM:
            return make_number_expr(op->as.number);

        case OPERAND_CONST_BOOL:
            return make_bool_expr(op->as.boolean);

        case OPERAND_NIL:
            return make_nil_expr();

        case OPERAND_TEMP:
            found = expr_table_get(table, op->as.name);
            if (found) return strdup(found);
            return make_name_expr(op->as.name);

        case OPERAND_VAR:
            return make_name_expr(op->as.name);

        case OPERAND_LABEL:
            return make_name_expr(op->as.name);
    }

    return strdup("nil");
}


static int operands_same_label(const IROperand* a, const IROperand* b) {
    if (!a || !b) return 0;
    if (a->type != OPERAND_LABEL || b->type != OPERAND_LABEL) return 0;
    if (!a->as.name || !b->as.name) return 0;
    return strcmp(a->as.name, b->as.name) == 0;
}

static int is_label_instr(const IRValue* v, const IROperand* label) {
    if (!v || !label) return 0;
    if (v->type != IR_Label) return 0;
    return operands_same_label(&v->Label.label, label);
}

static int is_jump_instr(const IRValue* v, const IROperand* target) {
    if (!v || !target) return 0;
    if (v->type != IR_Jump) return 0;
    return operands_same_label(&v->Jump.target, target);
}

/* =========================
   Forward declarations
   ========================= */

static void emit_range(LuaGenContext* ctx, size_t start, size_t end);

static int find_label_index(const IRArray* ir, const IROperand* label, size_t* outIndex) {
    if (!ir || !label || !outIndex) return 0;

    for (size_t i = 0; i < ir->count; i++) {
        if (ir->value[i].type == IR_Label &&
            operands_same_label(&ir->value[i].Label.label, label)) {
            *outIndex = i;
            return 1;
        }
    }

    return 0;
}

/*
   Match canonical if/else patterns:

   if without else:
       i:   JumpIfFalse cond -> end
       ...
       k:   Label end

   if with else:
       i:   JumpIfFalse cond -> elseLabel
       ...
       j:   Jump endLabel
       j+1: Label elseLabel
       ...
       k:   Label endLabel
*/
static int try_emit_if(LuaGenContext* ctx, size_t* indexPtr, size_t end) {
    size_t i = *indexPtr;
    const IRArray* ir = ctx->ir;
    const IRValue* head;
    size_t elseLabelIndex;
    size_t search;
    char* condExpr;

    if (i >= end) return 0;

    head = &ir->value[i];
    if (head->type != IR_JumpIfFalse) return 0;

    if (!find_label_index(ir, &head->JumpIfFalse.target, &elseLabelIndex)) {
        return 0;
    }

    if (elseLabelIndex >= end) {
        return 0;
    }

    condExpr = resolve_operand_expr(&ctx->exprs, &head->JumpIfFalse.condition);

    /*
       Try if/else:
       find a "goto endLabel" immediately before elseLabelIndex
    */
    if (elseLabelIndex > i + 1) {
        const IRValue* beforeElse = &ir->value[elseLabelIndex - 1];
        if (beforeElse->type == IR_Jump) {
            size_t endLabelIndex;
            if (find_label_index(ir, &beforeElse->Jump.target, &endLabelIndex) &&
                endLabelIndex < end &&
                endLabelIndex > elseLabelIndex &&
                is_label_instr(&ir->value[elseLabelIndex], &head->JumpIfFalse.target)) {

                emit_indent(ctx->out, ctx->indent);
                fprintf(ctx->out, "if %s then\n", condExpr);

                ctx->indent++;
                emit_range(ctx, i + 1, elseLabelIndex - 1);
                ctx->indent--;

                emit_indent(ctx->out, ctx->indent);
                fprintf(ctx->out, "else\n");

                ctx->indent++;
                emit_range(ctx, elseLabelIndex + 1, endLabelIndex);
                ctx->indent--;

                emit_indent(ctx->out, ctx->indent);
                fprintf(ctx->out, "end\n");

                *indexPtr = endLabelIndex + 1;
                free(condExpr);
                return 1;
            }
        }
    }

    /*
       Try if without else:
       target label is end label, and body is i+1 .. elseLabelIndex-1
    */
    search = elseLabelIndex;
    if (search < end && is_label_instr(&ir->value[search], &head->JumpIfFalse.target)) {
        emit_indent(ctx->out, ctx->indent);
        fprintf(ctx->out, "if %s then\n", condExpr);

        ctx->indent++;
        emit_range(ctx, i + 1, elseLabelIndex);
        ctx->indent--;

        emit_indent(ctx->out, ctx->indent);
        fprintf(ctx->out, "end\n");

        *indexPtr = elseLabelIndex + 1;
        free(condExpr);
        return 1;
    }

    free(condExpr);
    return 0;
}

static void emit_assign(LuaGenContext* ctx, const IRValue* v) {
    char* rhs = resolve_operand_expr(&ctx->exprs, &v->Assign.value);
    const char* dst = operand_name(&v->Assign.dst);

    if (!dst) {
        free(rhs);
        return;
    }

    emit_indent(ctx->out, ctx->indent);
    if (v->Assign.isLocal) {
        fprintf(ctx->out, "local %s = %s\n", dst, rhs);
    } else {
        fprintf(ctx->out, "%s = %s\n", dst, rhs);
    }

    if (is_temp_operand(&v->Assign.dst)) {
        expr_table_set(&ctx->exprs, dst, rhs);
    } else {
        expr_table_kill(&ctx->exprs, dst);
    }

    free(rhs);
}


static void emit_binary(LuaGenContext* ctx, const IRValue* v) {
    char* left = resolve_operand_expr(&ctx->exprs, &v->Binary.left);
    char* right = resolve_operand_expr(&ctx->exprs, &v->Binary.right);
    char* expr = make_binary_expr(left, IRBinaryOpName(v->Binary.op), right);
    const char* dst = operand_name(&v->Binary.dst);

    if (dst) {
        if (is_temp_operand(&v->Binary.dst)) {
            expr_table_set(&ctx->exprs, dst, expr);
        } else {
            emit_indent(ctx->out, ctx->indent);
            fprintf(ctx->out, "%s = %s\n", dst, expr);
            expr_table_set(&ctx->exprs, dst, expr);
        }
    }

    free(left);
    free(right);
    free(expr);
}

static void emit_fallback_comment(LuaGenContext* ctx, const IRValue* v) {
    emit_indent(ctx->out, ctx->indent);

    switch (v->type) {
        case IR_Label:
            fprintf(ctx->out, "--[[ label %s ]]\n", operand_name(&v->Label.label));
            break;

        case IR_Jump:
            fprintf(ctx->out, "--[[ unsupported jump to %s in Lua 5.1 backend ]]\n",
                    operand_name(&v->Jump.target));
            break;

        case IR_JumpIfFalse: {
            char* cond = resolve_operand_expr(&ctx->exprs, &v->JumpIfFalse.condition);
            fprintf(ctx->out,
                    "--[[ unsupported branch: if not %s then goto %s end ]]\n",
                    cond,
                    operand_name(&v->JumpIfFalse.target));
            free(cond);
            break;
        }

        default:
            fprintf(ctx->out, "--[[ unsupported IR ]]\n");
            break;
    }
}

static void emit_range(LuaGenContext* ctx, size_t start, size_t end) {
    size_t i = start;

    while (i < end) {
        const IRValue* v = &ctx->ir->value[i];

        if (try_emit_if(ctx, &i, end)) {
            continue;
        }

        switch (v->type) {
            case IR_BinaryOp:
                emit_binary(ctx, v);
                i++;
                break;

            case IR_Assign:
                emit_assign(ctx, v);
                i++;
                break;

            case IR_Label:
                /*
                   Usually consumed by try_emit_if.
                   If one survives here, it means the IR is not fully structured.
                */
                i++;
                break;

            case IR_Jump:
            case IR_JumpIfFalse:
                emit_fallback_comment(ctx, v);
                i++;
                break;

            default:
                emit_fallback_comment(ctx, v);
                i++;
                break;
        }
    }
}

void CodeGenLua(const IRArray* ir, FILE* out) {
    LuaGenContext ctx;

    if (!ir || !out) return;

    ctx.ir = ir;
    ctx.out = out;
    ctx.indent = 0;
    expr_table_init(&ctx.exprs);

    emit_range(&ctx, 0, ir->count);

    expr_table_free(&ctx.exprs);
}
