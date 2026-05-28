#include "ir.h"
#include "utils.h"


static int tempCount = 0;

/*
	// why char*?
	// because the function returns a dynamaticlly allocated string 
	// that its lifetime should extend beyond than the function call
	char* newTemp(void) {
		// why buffer 16 and not 32 or 64?
		// because buffer 16 is big enough to generate many temps we want!
		// although buffer 16 is less safe, i will use snprintf to prevent buffer overflow and UB
		char buffer[16];
		// this line means that :
		//    put t .. tempCount be stored in -> buffer
		snprintf(buffer, sizeof(buffer), "t%d", tempCount); 
		tempCount++;
		// why strdup? 
		// because buffer lives in stack, so it wont live long.
		// and we need a string that is still valid after return.
		return strdup(buffer);
	} 
*/



IROperand IRMakeTemp(char* name) {
    IROperand op;
    op.type = OPERAND_TEMP;
    op.as.name = name;
    return op;
}

IROperand IRMakeVar(const char* name) {
    IROperand op;
    op.type = OPERAND_VAR;
    op.as.name = strdup(name);
    return op;
}

IROperand IRMakeConstNum(double value) {
    IROperand op;
    op.type = OPERAND_CONST_NUM;
    op.as.number = value;
    return op;
}

IROperand IRMakeConstBool(int value) {
    IROperand op;
    op.type = OPERAND_CONST_BOOL;
    op.as.boolean = value ? 1 : 0;
    return op;
}

IROperand IRMakeNil(void) {
    IROperand op;
    op.type = OPERAND_NIL;
    return op;
}

IROperand IRMakeLabel(const char* name) {
    IROperand op;
    op.type = OPERAND_LABEL;
    op.as.name = strdup(name);
    return op;
}

IROperand IRCopyOperand(const IROperand* op) {
    IROperand copy = {0};
    copy.type = op->type;

    switch (op->type) {
        case OPERAND_TEMP:
        case OPERAND_VAR:
        case OPERAND_LABEL:
            copy.as.name = strdup(op->as.name);
            break;
        case OPERAND_CONST_NUM:
            copy.as.number = op->as.number;
            break;
        case OPERAND_CONST_BOOL:
            copy.as.boolean = op->as.boolean;
            break;
        case OPERAND_NIL:
            break;
    }

    return copy;
}


void IRFreeOperand(IROperand* op) {
    if (!op) return;

    switch (op->type) {
        case OPERAND_TEMP:
        case OPERAND_VAR:
        case OPERAND_LABEL:
            free(op->as.name);
            op->as.name = NULL;
            break;
        case OPERAND_CONST_NUM:
        case OPERAND_CONST_BOOL:
        case OPERAND_NIL:
            break;
    }
}


void IRPrintOperand(const IROperand* op) {
    switch (op->type) {
        case OPERAND_TEMP:
        case OPERAND_VAR:
        case OPERAND_LABEL:
            printf("%s", op->as.name);
            break;
        case OPERAND_CONST_NUM:
            printf("%f", op->as.number);
            break;
        case OPERAND_CONST_BOOL:
            printf("%s", op->as.boolean ? "true" : "false");
            break;
        case OPERAND_NIL:
            printf("nil");
            break;
    }
}




IROperand newTempOperand(void) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "t%d", tempCount);
    tempCount++;
    return IRMakeTemp(strdup(buffer));
}



// a small Helper for constant folding optimization.

// what should i do Tommorow?
// i should add IR const folding Optimization,
// Filling constLookup function,
// and Rest... :(

/* int constLookup(ConstTable* t, const char* name, double out) {

} */

// Now we need a function that initilizes a IR
// how it works? what it returns? what it gets?
// it allocates memory for the IR, fills the count and capacity.
// it returns nothing, just initilizes a IR.
// it gets a IRArray.

void IRAlloc(IRArray* ir) {
	if (!ir) { return; }
	
	ir->capacity = 16;
	ir->count = 0;
	// allocate a Heap for the IR
	ir->value = malloc(sizeof(IRValue) * ir->capacity);
	if (!ir->value) {
		perror("IR allocation failed\n");
		ir->value=NULL; // practice
		exit(EXIT_FAILURE);
	}
}

// now we need a function that could Grow a IR Dynamaticly

void IRGrow(IRArray* ir) {
	// safety check
	if (!ir) { return; }
	if (ir->count < ir->capacity) { return; }
	
	
	ir->capacity *= 2; // O(1) :D
	// why not do ir->value = realloc... ?
	// because realloc might fail, and will make ir->value break
	IRValue* newIR = realloc(ir->value, sizeof(IRValue) * ir->capacity);
	if (!newIR) {
		perror("IR reallocation failed\n");
		newIR = NULL;
		exit(EXIT_FAILURE);
	}
	ir->value = newIR;
}

// now we need a emitter!
// its not that hard. so no reasoning in this part
void IREmit(IRArray* ir, IRValue value) {
	// Grow heap size if needed
	IRGrow(ir);
	ir->value[ir->count++] = value;
}


// If statement IR Helpers that will be needed
// or the IR will become a code hell

void IREmitLabel(IRArray* ir, IROperand label) {
    IRValue v = {0};
    v.type = IR_Label;
    v.Label.label = IRCopyOperand(&label);
    IREmit(ir, v);
}

void IREmitJump(IRArray* ir, IROperand target) {
    IRValue v = {0};
    v.type = IR_Jump;
    v.Jump.target = IRCopyOperand(&target);
    IREmit(ir, v);
}

void IREmitJumpIfFalse(IRArray* ir, IROperand condition, IROperand target) {
    IRValue v = {0};
    v.type = IR_JumpIfFalse;
    v.JumpIfFalse.condition = IRCopyOperand(&condition);
    v.JumpIfFalse.target = IRCopyOperand(&target);
    IREmit(ir, v);
}


// binary operator mappings
IRBinaryOpKind IRMapBinaryOp(const char* op) {
    if (!op) return IR_OP_ADD; /* fallback, but better to error */

    if (strcmp(op, "+") == 0) return IR_OP_ADD;
    if (strcmp(op, "-") == 0) return IR_OP_SUB;
    if (strcmp(op, "*") == 0) return IR_OP_MUL;
    if (strcmp(op, "/") == 0) return IR_OP_DIV;
    if (strcmp(op, "%") == 0) return IR_OP_MOD;
    if (strcmp(op, "<") == 0) return IR_OP_LT;
    if (strcmp(op, ">") == 0) return IR_OP_GT;
    if (strcmp(op, "<=") == 0) return IR_OP_LE;
    if (strcmp(op, ">=") == 0) return IR_OP_GE;
    if (strcmp(op, "==") == 0) return IR_OP_EQ;
    if (strcmp(op, "!=") == 0) return IR_OP_NE;

    fprintf(stderr, "Unknown binary operator: %s\n", op);
    exit(EXIT_FAILURE);
}

const char* IRBinaryOpName(IRBinaryOpKind op) {
    switch (op) {
        case IR_OP_ADD: return "+";
        case IR_OP_SUB: return "-";
        case IR_OP_MUL: return "*";
        case IR_OP_DIV: return "/";
        case IR_OP_MOD: return "%";
        case IR_OP_LT: return "<";
        case IR_OP_GT: return ">";
        case IR_OP_LE: return "<=";
        case IR_OP_GE: return ">=";
        case IR_OP_EQ: return "==";
        case IR_OP_NE: return "!=";
        default: return "?";
    }
}





// 	// now we need a AST -> IR function. it returns a char*

IROperand IRGen(struct astNode* node, IRArray* ir) {
    if (!node || !ir) {
        fprintf(stderr, "IRGen called with missing arguments\n");
        exit(EXIT_FAILURE);
    }

    switch (node->type) {
        case AST_Number: {
            return IRMakeConstNum(node->number);
        }

        case AST_Boolean: {
            return IRMakeConstBool(node->boolean);
        }

        case AST_Identifier: {
            return IRMakeVar(node->string);
        }

        case AST_BinaryOp: {
            IROperand left = IRGen(node->Binary.left, ir);
            IROperand right = IRGen(node->Binary.right, ir);
            IROperand temp = newTempOperand();

            IRValue v = {0};
            v.type = IR_BinaryOp;
            v.Binary.dst = IRCopyOperand(&temp);
            v.Binary.op = IRMapBinaryOp(node->Binary.op);
            v.Binary.left = IRCopyOperand(&left);
            v.Binary.right = IRCopyOperand(&right);

            IREmit(ir, v);

            IRFreeOperand(&left);
            IRFreeOperand(&right);

            return temp;
        }

        default: {
            fprintf(stderr, "Unsupported AST node in IRGen\n");
            exit(EXIT_FAILURE);
        }
    }
}


// Now for IRGenIf,
// what it does?
// it converts AST_If nodes to a Meaningful IR.
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// Tag: Fix this comment, its shitty as hell O_O

void IRStmtGen(IRArray* ir, struct astNode* node);

void IRGenIf(IRArray* ir, struct astNode* node) {
    static int labelCount = 0;

    char elseBuf[32];
    char endBuf[32];

    snprintf(elseBuf, sizeof(elseBuf), "elseLabel_%d", labelCount);
    snprintf(endBuf, sizeof(endBuf), "endLabel_%d", labelCount);
    labelCount++;

    IROperand cond = IRGen(node->If.condition, ir);

    if (node->If.elseBlock) {
        IROperand elseLabel = IRMakeLabel(elseBuf);
        IROperand endLabel = IRMakeLabel(endBuf);

        IREmitJumpIfFalse(ir, cond, elseLabel);
        IRStmtGen(ir, node->If.thenBlock);
        IREmitJump(ir, endLabel);
        IREmitLabel(ir, elseLabel);
        IRStmtGen(ir, node->If.elseBlock);
        IREmitLabel(ir, endLabel);

        IRFreeOperand(&elseLabel);
        IRFreeOperand(&endLabel);
    } else {
        IROperand endLabel = IRMakeLabel(endBuf);

        IREmitJumpIfFalse(ir, cond, endLabel);
        IRStmtGen(ir, node->If.thenBlock);
        IREmitLabel(ir, endLabel);

        IRFreeOperand(&endLabel);
    }

    IRFreeOperand(&cond);
}



// Now we need a IR function that generates assignment IR for us ( because we only support Assignment for now )
void IRStmtGen(IRArray* ir, struct astNode* node) {
	//safety check
	if (!node || !ir) { 
		printf("IRStmtGen called with missing argunments\n");
		exit(EXIT_FAILURE);
	}
	
	switch(node->type) {
		case AST_Block: {
			for (size_t i=0;i<node->Block.count;i++) {
				IRStmtGen(ir, node->Block.stmts[i]);
			}	
			break;
		};
		
		case AST_Assign: {
			IROperand value;
			if (node->Assign.value) {
				value = IRGen(node->Assign.value, ir);
			} else {
				value = IRMakeNil();
			}

			IROperand dst = IRMakeVar(node->Assign.name);

			IRValue v = {0};
			v.type = IR_Assign;
			v.Assign.dst = IRCopyOperand(&dst);
			v.Assign.value = IRCopyOperand(&value);
			v.Assign.isLocal = node->Assign.isLocal;

			IREmit(ir, v);

			IRFreeOperand(&dst);
			IRFreeOperand(&value);
			break;
		}
		
		case AST_If: {
			IRGenIf(ir, node);
			break;
		}
		
		// default Case
		default: { break; }
	}
}


void IRPrint(const IRArray* ir) {
    for (size_t i = 0; i < ir->count; i++) {
		const IRValue* v = &ir->value[i];

        switch (v->type) {

			case IR_BinaryOp: {
				IRPrintOperand(&v->Binary.dst);
				printf(" = ");
				IRPrintOperand(&v->Binary.left);
				printf(" %s ", IRBinaryOpName(v->Binary.op));
				IRPrintOperand(&v->Binary.right);
				printf("\n");
				break;
			};

			case IR_Assign: {
				if (v->Assign.isLocal) {
					printf("local ");
				}
				IRPrintOperand(&v->Assign.dst);
				printf(" = ");
				IRPrintOperand(&v->Assign.value);
				printf("\n");
				break;
			};
            
			case IR_Label: {
				IRPrintOperand(&v->Label.label);
				printf(":\n");
				break;
			};

			case IR_Jump: {
				printf("goto ");
				IRPrintOperand(&v->Jump.target);
				printf("\n");
				break;
			};

			case IR_JumpIfFalse: {
				printf("if_false ");
				IRPrintOperand(&v->JumpIfFalse.condition);
				printf(" goto ");
				IRPrintOperand(&v->JumpIfFalse.target);
				printf("\n");
				break;
			};
			
                
            default : {
            	printf("Unsupported IR\n");
            	break;
            }
        }
    }
}

// And now, our IR is leaking Badly, we need a IRCleanup to handle the job for us.
void IRCleanup(IRArray* ir) {
	if (!ir) { return; }
	
	for (size_t i=0;i<ir->count;i++) {
		IRValue* IRVal = &ir->value[i];
		
		switch (IRVal->type) {
			case IR_Assign: {
				IRFreeOperand(&IRVal->Assign.dst);
				IRFreeOperand(&IRVal->Assign.value);
				break;
			};
			
			case IR_BinaryOp: {
				IRFreeOperand(&IRVal->Binary.dst);
				IRFreeOperand(&IRVal->Binary.left);
				IRFreeOperand(&IRVal->Binary.right);
				break;
			};
			
			case IR_Label: {
				IRFreeOperand(&IRVal->Label.label);
				break;
			};
			
			case IR_Jump: {
				IRFreeOperand(&IRVal->Jump.target);
				break;
			};
			
			case IR_JumpIfFalse: {
				IRFreeOperand(&IRVal->JumpIfFalse.condition);
				IRFreeOperand(&IRVal->JumpIfFalse.target);
				break;
			};
			

			default: { break; }
		}
	}
	// free the ir value container
	free(ir->value);
	ir->value = NULL;
	ir->count = 0;
	ir->capacity = 0;
}
