#include "ast.h"
#include "utils.h"

struct astNode* ast_alloc(void) {
	struct astNode* node = calloc(1, sizeof(struct astNode));
	if (!node) {
		perror("astNode allocation failed");
		exit(EXIT_FAILURE);
	}
	
	// Return raw node.
	// Why?
	// Because node is a Local variable defined in ast_alloc, 
	// So returning the pointer of it is the same as NULL
	// because node wont live Much and its in the stack not the Heap.
	return node; 
}



// we need to print our parsed AST result.
// we also need a Simple switch-case function that does astType -> string.

const char* AstToString(astType AstType) {
	
	switch(AstType) {
		case AST_Block: {
			return "Block";	
		};
		case AST_Number: {
			return "NumberAst";
		};		
		case AST_String: {
			return "StringAst";
		};
		case AST_BinaryOp: {
			return "BinaryOpAst";
		};
		case AST_Identifier: {
			return "IdentifierAst";
		};
		case AST_Assign: {
			return "AssignAst";
		};
		case AST_If: {
			return "IfAst";
		};
        case AST_Boolean: { return "BooleanAst"; };
        case AST_Nil: { return "NilAst"; };
		
		default: {
			return "AstNone";
		}
	}
}

void PrintMyAST(struct astNode* node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; i++) printf("  ");

    switch (node->type) {
    	case AST_Block: {
    		printf("Block\n");
    		for (size_t i=0;i<node->Block.count;i++) {
    			PrintMyAST(node->Block.stmts[i], indent+1);
    		}
    		break;
    	};
    	
        case AST_Number: {
			// why not (int)node->integer?
			// because we want to be able to print long int numbers :/
            printf("Number: %f\n", node->number);
            break;
        };    
        
		case AST_String: {
			printf("String: %s\n", node->string);
			break;
		};
        
        case AST_Boolean: {
            printf("Boolean: %s\n", node->boolean ? "true" : "false");
            break;
        };

        case AST_Nil: {
            printf("Nil\n");
            break;
        };
			
        case AST_Identifier: {
            printf("Identifier: %s\n", node->string);
            break;
        };

        case AST_BinaryOp: {
			printf("BinaryOp: %s\n", node->Binary.op);
            PrintMyAST(node->Binary.left, indent + 1);
            PrintMyAST(node->Binary.right, indent + 1);
            break;
    	};

        case AST_Assign: {
            if (node->Assign.isLocal) {
                printf("Local_Assign: %s =\n", node->Assign.name);
            } else {
                printf("Global_Assign: %s =\n", node->Assign.name);
            }

            if (node->Assign.value) {
                PrintMyAST(node->Assign.value, indent + 1);
            } else {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Nil/NoValue\n");
            }
            break;
        };

        
        case AST_If: {
        	printf("If:\n");
        	PrintMyAST(node->If.condition, indent + 1);
        	printf("Then:\n");
        	PrintMyAST(node->If.thenBlock, indent + 1);
        	
        	// only print else if it exists.
        	if (node->If.elseBlock) {
	        	printf("Else:\n");
	        	PrintMyAST(node->If.elseBlock, indent + 1);
        	}
        	break;
        };

        default:
            printf("Unknown node type '%s'\n", AstToString(node->type));
    }
}

// Ok, So we need a free_ast() now.
// how should it work?
// maybe switch on a astNode

void AstCleanup(struct astNode* node) {
	// safety guard
	if (!node) { return; }
	
	// switch on the Node, if it was Ast_*, free its childs
	switch(node->type) {
		case AST_Block: {
			for (size_t i=0;i<node->Block.count;i++) {
				AstCleanup(node->Block.stmts[i]);
			}
			
			// We need to free the stmts itself too.
			// C is like this : it has a container []
			// when all of the elements of the container is freed, 
			// still the container exists, but doesnt have anything in itself,
			// MEMORY LEAK! :O
			free(node->Block.stmts);
			break;
		};
		
		case AST_Identifier:
		case AST_String: {
			free(node->string);
			break;
		};
		case AST_BinaryOp: {
			AstCleanup(node->Binary.left);
			AstCleanup(node->Binary.right);
			free(node->Binary.op);
			break;
		};
		case AST_Assign: {
			AstCleanup(node->Assign.value);
			free(node->Assign.name); // Assign.name is a char*, not a astNode*
			break;
		};
		
		case AST_If: {
			AstCleanup(node->If.condition);
			AstCleanup(node->If.thenBlock);
			AstCleanup(node->If.elseBlock);
			break;
		};
		
		// Safety pass to default
		case AST_Number:
		default: { break; }
	}
	free(node);
}
