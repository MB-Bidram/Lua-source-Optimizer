#include "parser.h"
#include "utils.h"

static lexValue* current(parseValue* p);
static void advance(parseValue* p);
static void expect(parseValue* p, TokenType t);
static int op_primary(char op);
static int isTerminator(parseValue* p);

static astNode* parse_number(parseValue* p);
static astNode* parse_identifier(parseValue* p);
static astNode* parse_literal_string(parseValue* p);
static astNode* parse_boolean(parseValue* p);
static astNode* parse_nil(parseValue* p);
static astNode* parse_expression(parseValue* p, int pro);
static astNode* parse_primary(parseValue* p);
static astNode* parse_statement(parseValue* p);
static astNode* parse_block(parseValue* p);
static astNode* parse_if(parseValue* p);

astNode* parse(TokenArray* lexed, LuaVersion version);

// Why static inline?
// C inline is diffrent than C++ Inline, 
// use static inline to prevent Linking errors.

static inline struct lexValue* current(parseValue* p) {
	return &p->array->data[p->pos];
}

static inline void advance(parseValue* p) {
	// why p->pos + 1?
	// to be sure Eof is counted too
	if (p->pos + 1< p->array->count) {
		p->pos++;
	}
}

static int keyword_is(parseValue* p, const char* s) {
    return current(p)->t.Type == Keyword && strcmp(current(p)->v.string, s) == 0;
}


static inline void expect(parseValue* p, TokenType t) { // Why not make TokenType t a pointer?
	// It will just make things harder, not worth it. and TokenType is just a single integer.
	if (current(p)->t.Type != t) {
		// use our fancy error_at function for cleaner errors
		error_at(current(p)->pos, "Expected <%s>, got <%s>",TokenToString(t), TokenToString(current(p)->t.Type));
	}
}

static inline int op_primary(char op) {
	switch(op) {
		case '^': return 7;

		case '*':
		case '/':
		case '%': return 6;

		case '+':
		case '-': return 5;

		case '&':
		case '|': return 3;

		default: return -1;
	}
}


static int token_precedence(parseValue* p) {
    lexValue* tok = current(p);

    if (tok->t.Type == BinaryOp) {
        switch (tok->v.operator) {
            case '^': return 7;
            case '*':
            case '/':
            case '%': return 6;
            case '+':
            case '-': return 5;
            case '&':
            case '|': return 3;
            default: return -1;
        }
    }

    switch (tok->t.Type) {
        case FloorDiv: return 6;
        case Concat: return 4;
        case ShiftLeft:
        case ShiftRight: return 3;
        case Less:
        case LessEqual:
        case Greater:
        case GreaterEqual:
        case EqualEqual:
        case NotEqual:
            return 2;
        default:
            return -1;
    }
}

static int token_is_right_associative(parseValue* p) {
	lexValue* tok = current(p);

	if (tok->t.Type == BinaryOp && tok->v.operator == '^') {
		return 1;
	}

	if (tok->t.Type == Concat) {
		return 1;
	}

	return 0;
}

static char* token_to_op_string(parseValue* p) {
	lexValue* tok = current(p);

	switch (tok->t.Type) {
		case BinaryOp: {
			char opbuf[2] = { tok->v.operator, '\0' };
			return strdup(opbuf);
		}

		case FloorDiv:   return strdup("//");
		case Concat:     return strdup("..");
		case ShiftLeft:  return strdup("<<");
		case ShiftRight: return strdup(">>");

		case Less:         return strdup("<");
		case LessEqual:    return strdup("<=");
		case Greater:      return strdup(">");
		case GreaterEqual: return strdup(">=");
		case EqualEqual:   return strdup("==");
		case NotEqual:     return strdup("~=");

		default:
			return NULL;
	}
}


// for our Simple helper, we need a array that keeps the terminator keywords

static const char* termKeywords[TermKeySize] = {
	[0] = "end",
	[1] = "until",
	[2] = "else",
	[3] = "elseif"
};


// a Simple Helper to check if the current Keyword is terminator or not.
// why parseValue* p and not ASTType?
// because we need the kind if the Keyword stored in .v.string 
static inline int isTerminator(parseValue *p) {
	if (current(p)->t.Type != Keyword) { return 0; }
	
	for (int i=0;i<TermKeySize;i++) {
		if (strcmp(current(p)->v.string , termKeywords[i]) == 0) {
			return 1;
		} 
	}
	return 0;
}

struct astNode* parse_number(parseValue *p) {
	// Check if the current Token is a NumberToken or not
	expect(p, Number);
	

	struct astNode* numberNode = ast_alloc();
	// What should parse_number do?
	// It should just make a Simple Number(intger) in the AST.
	// and return it
	
	numberNode->type = AST_Number;
	numberNode->number = current(p)->v.number;
	advance(p); // Pos++;
	
	return numberNode;
}
struct astNode* parse_identifier(parseValue *p) {
	expect(p, Identifier);

	struct astNode* identNode = ast_alloc();
	identNode->type = AST_Identifier;
	identNode->string = strdup(current(p)->v.string);
	advance(p);

	return identNode;
}

struct astNode* parse_literal_string(parseValue *p) {
	expect(p, String);

	struct astNode* stringNode = ast_alloc();
	stringNode->type = AST_String;
	stringNode->string = strdup(current(p)->v.string);
	advance(p);

	return stringNode;
}

struct astNode* parse_boolean(parseValue *p) {
	expect(p, Boolean);

	struct astNode* boolNode = ast_alloc();
	boolNode->type = AST_Boolean;
	boolNode->boolean = current(p)->v.boolean;
	advance(p);

	return boolNode;
}

struct astNode* parse_nil(parseValue *p) {
	expect(p, Nil);

	struct astNode* nilNode = ast_alloc();
	nilNode->type = AST_Nil;
	advance(p);

	return nilNode;
}



struct astNode* parse_expression(parseValue* p, int pro) {
	struct astNode* left = parse_primary(p);
	
	while (1) {
		// Get the operator for prec.
		// use const, why not? we dont need to change any of these 2 variables
		const int prec = token_precedence(p);
		
		if (prec < pro) { break; }

		// Convert the current operator token into a string form.
		// This is needed because Binary.op is now char*,
		// and some Lua operators are multi-character like >>, <<, //, .., <=, == ...
		int rightAssoc = token_is_right_associative(p);

		char* op = token_to_op_string(p);
		if (!op) {
			error_at(current(p)->pos, "Unsupported operator token in expression: '%s'",
			         TokenToString(current(p)->t.Type));
		}

		advance(p);
		
		// Lua ^ is right associative
		// DO NOT do prec+1 when the operator is ^
		
		// this line means :
		//    if ( operator is right associative ) return prec
		//    else return prec+1;
		int Prec = rightAssoc ? prec : prec + 1;
		
		// the next operator SHOULD be atleast pro+1 to be tighter
		struct astNode* right = parse_expression(p, Prec);
		
		// initilize a astNode to return it.
		struct astNode* binOp = ast_alloc();
		binOp->type = AST_BinaryOp;
		binOp->Binary.left = left;
		binOp->Binary.right = right;
		binOp->Binary.op = op;
		
		left = binOp; // Reuse of efficiency
	}
	
	return left;
}


// Noise:
//    What parse_rimary does?
// 	  i dont know :D
// EndNoise
struct astNode* parse_primary(parseValue *p) {
	struct lexValue* currentToken = current(p);

	switch (currentToken->t.Type) {
		case Number:
			return parse_number(p);

		case Identifier:
			return parse_identifier(p);

		case String:
			return parse_literal_string(p);

		case Boolean:
			return parse_boolean(p);

		case Nil:
			return parse_nil(p);

		case OpenParen: {
			advance(p); // Pass over (

			struct astNode* parenNode = parse_expression(p, 0);

			expect(p, CloseParen);
			advance(p); // Pass over )

			return parenNode;
		}

		case Eof:
			error_at(current(p)->pos, "Unexpected <eof>");
			break;

		default:
			error_at(
				current(p)->pos,
				"unexpected <token> '%s'",
				TokenToString(current(p)->t.Type)
			);
	}

	return NULL;
}



struct astNode* parse_statement(parseValue* p) {
	
	switch (current(p)->t.Type) {
		case Keyword: {
			// What should it do?
			// If keyword -> local then :
            if (strcmp(current(p)->v.string, "local") == 0) {
                advance(p); // pass over local

                struct astNode* assignNode = ast_alloc();
                assignNode->type = AST_Assign;
                assignNode->Assign.isLocal = 1;

                expect(p, Identifier);
                assignNode->Assign.name = strdup(current(p)->v.string);
                advance(p);

                if (current(p)->t.Type == Equal) {
                    advance(p);
                    assignNode->Assign.value = parse_expression(p, 0);
                } else {
                    assignNode->Assign.value = NULL;
                }

                if (current(p)->t.Type == Semicolon) { advance(p); }

                return assignNode;
            }
			
			if (strcmp(current(p)->v.string, "if") == 0) {
				return parse_if(p);
			}

			if (strcmp(current(p)->v.string, "goto") == 0) {
				if (!p->vinfo->has_goto) {
					error_at(current(p)->pos, "`goto` is not supported in this Lua version");
				}
				error_at(current(p)->pos, "`goto` lexed correctly, but parser support is not implemented yet");
			}

			error_at(current(p)->pos, "Unsupported <keyword> : '%s'",current(p)->v.string);
			break;
			break;
		};
		
		case Identifier: {
			char* name = current(p)->v.string;
			advance(p);

			expect(p, Equal);
			advance(p);

			struct astNode* expr = parse_expression(p, 0);
			if (current(p)->t.Type == Semicolon) { advance(p); }

			struct astNode* assignNode = ast_alloc();
			assignNode->type = AST_Assign;
			assignNode->Assign.isLocal = 0;
			assignNode->Assign.name = strdup(name);
			assignNode->Assign.value = expr;

			return assignNode;
		};
		
		default: {
			break; // jump to the down printf error and exit failure.
		};
	}

	error_at(current(p)->pos, "Unsupported <TokenType> : '%s'",TokenToString(current(p)->t.Type));
	
	return NULL; // wont be reached
}

// now we need a parse_block.
struct astNode* parse_block(parseValue* p) {
	
	struct astNode* BlockNode = ast_alloc();
	BlockNode->type = AST_Block;
	
	size_t capacity = 16;
	size_t count = 0;
	
	struct astNode** Stmt = malloc(sizeof(struct astNode*) * capacity);
	if (!Stmt) {
		perror("Block allocation failed\n");
		exit(EXIT_FAILURE);
	}
	
	while (current(p)->t.Type != Eof && !isTerminator(p)) {
		
		if (count >= capacity) {
			// what should we do if count >= capacity?
			// we should do a realloc, if failed, perror and exit.
			// we need to do capacity *= 2 too!
			// and why *= 2? because O(1) :o
			capacity *= 2;
			struct astNode** newStmt = realloc(Stmt, sizeof(struct astNode*) * capacity);
			if (!newStmt) {
				perror("Block reallocation failed\n");
				exit(EXIT_FAILURE);
			}
			
			Stmt = newStmt;
		}
		
		Stmt[count++] = parse_statement(p);
	}
	
	BlockNode->Block.stmts = Stmt;
	BlockNode->Block.count = count;
	
	return BlockNode;
}

// Now we need a parse_if .
struct astNode* parse_if(parseValue* p) {
	if (!p) { return NULL; }
	
	// pass over the If statement
	advance(p); 
		
	// parse the condition
	struct astNode* cond = parse_expression(p, 0);
	
	// a Small guard to give more accurate errors about Eof.
	if (current(p)->t.Type == Eof) {
		error_at(current(p)->pos, "Expected Condition after if, got <eof>");
	}
	
	// expect then (or keyword)
	expect(p, Keyword);
	if (strcmp(current(p)->v.string, "then") != 0) {
		error_at(current(p)->pos, "Missed symbol `then`.");
	}
	
	// Now pass over the Then keyword
	advance(p);
	
	// parse until a Terminator keyword
	struct astNode* thenBlock = parse_block(p);
	
	struct astNode* elseBlock = NULL;
	
	if (current(p)->t.Type == Keyword && strcmp(current(p)->v.string, "else") == 0) {
		// pass over else 
		advance(p);
		elseBlock = parse_block(p);
	}
	
	expect(p, Keyword);
	if (strcmp(current(p)->v.string, "end") != 0) {
		error_at(current(p)->pos, "Missed symbol 'end'.");
	}
	// pass over end
	advance(p);
	
	// Initilize a astNode to return.
	// what should we initilize it with?
	// maybe allocate a ast node first,
	// set the type as AST_If,
	// fill the struct If with the values we made in the upper code.
	struct astNode* ifNode = ast_alloc();
	
	ifNode->type = AST_If;
	
	ifNode->If.condition = cond;
	ifNode->If.elseBlock = elseBlock;
	ifNode->If.thenBlock = thenBlock;
	
	
	// return our node
	return ifNode;
}




struct astNode* parse(TokenArray* lexed, LuaVersion version) {
	// What this should produce? local x = 12;
	// localStatement:
	// 	  Identifier(x)
	//    Value(12)
	
	// Maybe something like this:
	/* if (currentToken == keyword_local) {
		cursor++;
		expect(equal_token);
		cursor++;
		expect(identifier);
		char* name = currentToken;
		cursor++;
		
		if (currentToken == number || currentToken == string) {
			auto val = currentToken;
		}
		emit(localStatement);
		emit(identifier);
		emit(name);
		emit(value);
		emit(val);
	   }
	*/
	
	// i dont like chained parse_functions, so i will make everything be called from here.
	
	// Why not a Pointer?
	// Because *p will need Manual memory control.
	// a Stack based parseValue is more easy to control and its more Safe.
	
	// No UB
	parseValue p; 
	
	p.array = lexed;
	p.pos = 0;
    p.version = version;
    p.vinfo = get_lua_version_info(version);
	
	return parse_block(&p);
}
