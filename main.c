
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>

#define KeySize 15
#define TermKeySize 4



// What we are going to do?
// we want to make a C Code that Lexes Lua code.
// and just does a numeric constant folding.
// like x = 1 + 2 : x = 3;
// How we need to do that?
// First Lex it, nothing more.


// We only support Numbers for now.




// older lexer functions.

static struct TypeString* newString(const char* str);
static int StringAppend(struct TypeString* str, const char* str2);
static int FreeString(struct TypeString* string);


// Lexer? Lets try to make a LEXER!!
// what we need? printf, strcpy, strlen, malloc, realloc, isalpha, isdigit
// Lexer returns what? a array of a struct
// what the struct needs to have? -> what things we need our array to have?


// C doesnt have Maps, We will use Two Arrays Insted


// My own String Helper

struct TypeString {
	char* str;
	size_t len;
};

static struct TypeString* newString(const char* str) {
	// Ensure str Exists
	if (!str) { return NULL; }


	struct TypeString* OutString = malloc(sizeof(struct TypeString));
	if (!OutString) {
		perror("String allocation failed");
		OutString=NULL; return NULL;
	}
	OutString->len = strlen(str);

	OutString->str = malloc(OutString->len+1); // +1 Because of '/0';
	if (!OutString->str) {
		perror("String allocation failed");
		OutString->str = NULL; free(OutString);
		return NULL;
	}
	memcpy(OutString->str, str, OutString->len+1);
	
	return OutString;
}

static int StringAppend(struct TypeString* str, const char* str2) {
	// What StringAppend should do?
	// It should get a TypeString, Append str2 to it
	// How? 
	// It should realloc and Grow the size of the Memory allocated 
	// How we access the allocated memory? by *str.
	
	if (!str || !str2) { return 0; }
	
	size_t str2_size = strlen(str2);
	
	char *new_str = realloc(str->str, str->len + str2_size+1);
	if (!new_str) {
		perror("String allocation failed");
		new_str=NULL; return 0;
	}
	
	// How to Append them Together? memcpy!
	memcpy(new_str + str->len, str2, str2_size+1);
	str->str = new_str;
	str->len += str2_size;
	return 1;
}

// What FreeString does?
// it makes freeing the TypeString more Easy,
// what parameters it gets? it Gets a TypeString
static int FreeString(struct TypeString* string) {
	if (!string) { return 0; }

	free(string->str);
	free(string);
	
	return 1;
}


typedef struct {
	size_t line;
	size_t col;
} SourcePos;


typedef enum {
	TokenNone=0,
	// Keywords
	Keyword,
	// Types
	Number,
	Identifier,
	// Numeric
	BinaryOp,
	// Parens
	OpenParen,
	CloseParen,
	OpenBrace,
	CloseBrace,
	OpenBracket,
	CloseBracket,
	// Others
	Equal,
	Semicolon,
	Dot,
	
	Eof,
} TokenType;

struct type {
	union { 
		TokenType Type;
	};
};



struct lexValue {
	struct type t;
	SourcePos pos;
	
	union {
	// I dont exacly know why union, but i think it will just make one value to be used
	// And prevent non used types
		char* string;
		double number;
		
		// float number; we currently dont support 'Dot' Token
		unsigned short int boolean;
		
		char operator; // char is just 1 byte, It wont matter much in memory
		// The Union still reserves 4 byte for the int integer so this single char doesnt matter in memory efficiency.
	} v;
};


// Lexer function cannot return raw struct lexValue.
// So we will make a Metadata struct
// what the metadata should contain? 
// The data -> lexed, The count -> *count

// We will use 2 Names to be able to use both struct TokenArray and TokenArray.
typedef struct TokenArray {
	struct lexValue *data;
	size_t count;
} TokenArray;


const TokenType charTokens[256] = {	
    ['+'] = BinaryOp,
    ['-'] = BinaryOp,
    ['*'] = BinaryOp,
    ['/'] = BinaryOp,
    ['^'] = BinaryOp,
    ['('] = OpenParen,
    [')'] = CloseParen,
    ['{'] = OpenBrace,
    ['}'] = CloseBrace,
    ['['] = OpenBracket,
    [']'] = CloseBracket,
    ['='] = Equal,
    [';'] = Semicolon,
    ['.'] = Dot
};



const char* keywords[KeySize] = {
	[0] = "local",
	[1] = "nil",
	[2] = "for",
	[3] = "in",
	[4] = "while",
	[5] = "do",
	[6] = "then",
	[7] = "end",
	[8] = "repeat",
	[9] = "until",
	[10] = "goto",
	[11] = "break",
	[12] = "if",
	[13] = "elseif",
	[14] = "else"
};


// Noise:
// 	  WOW! C ALLOWS ARRAYS BEFORE MAIN? AND IT EVEN HAS KEY = VALUE TOO? Damn...
// EndNoise


TokenType isKeyword(const char* str) {
	for (int i=0;i<KeySize;i++) {
		if (strcmp(str, keywords[i]) == 0) {
			return Keyword;
		} 
	}
	return Identifier;
}




// Helper functions to make Printing the results more Easy

const char* TokenToString(const TokenType token) {
	// why not return NULL?
	// why return NULL when we can just return TokenNone.
	if (!token) { return "TokenNone"; }		

	switch (token) {
		case Keyword: { return "Keyword"; };
		
		case Number: { return "Number"; };
		case Identifier: { return "Identifier"; };
		
		case BinaryOp: { return "BinaryOp"; };
		
		case OpenParen: { return "OpenParen"; };
		case CloseParen: { return "CloseParen"; };
		case OpenBrace: { return "OpenBrace"; };
		case CloseBrace: { return "CloseBrace"; };
		case OpenBracket: { return "OpenBracket"; };
		case CloseBracket: { return "CloseBracket"; };
		
		case Equal: { return "Equal"; };
		case Semicolon: { return "Semicolon"; };
		case Dot: { return "Dot"; };
		
		case Eof: { return "Eof"; };
				
		default: { return "TokenNone"; };
	}
}

void printMyTokens(const TokenArray *array) {
	if (!array) { return; }
	// So, What should printMyTokens do?
	// it will just Print every TokenType and Their Value for Us.
	// How it will do that?
	// It will go thorugh all of TokenArray and Print their t.Type (with the TokenToString), and their v.Value;
	
	for (size_t i=0;i<array->count;i++) {
		TokenType token = array->data[i].t.Type;
		
		printf("Token: %d   '%s'", (int)i, TokenToString(token));
		
		switch (token) {
			case Number: { printf("  Value: Number '%f'\n",array->data[i].v.number); break; };
			
			case Identifier: { printf("  Value: Identifier '%s'\n",array->data[i].v.string); break; };
			case Keyword: { printf("  Value: Keyword '%s'\n",array->data[i].v.string); break; };
			
			case BinaryOp: { printf("  Value: Operator '%c'\n",array->data[i].v.operator); break; };
			
			default: { printf("   Value: None ''\n"); break; }
		}
		
	}
}


static void TokenCleanup(TokenArray *array) {
	if (!array) { return; }

	// What should TokenCleanup do?
	// Just do the Cleaning up more Easy
	for (size_t i=0;i<array->count;i++) {
		switch (array->data[i].t.Type) {
			case Keyword:
			case Identifier: {
				free(array->data[i].v.string); break;
			};
			
			default: {
				
			}
		}
	}
	free(array->data);
	free(array);
}


// My own Error helper, its one function though
void error_at(SourcePos pos, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "[Error] %zu:%zu: ", pos.line, pos.col);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    va_end(args);
    exit(EXIT_FAILURE);
}

// We need a little static inline Helper that makes line/col tracking more easy, and more clean
static inline void advanceChar(const char c, size_t* line, size_t* col) {
	if (c == '\n') { (*line)++; *col=1; }  else { (*col)++; }
}





void dynCheck(struct lexValue **array, size_t *capacity, size_t *count) {
	if (!array || !capacity || !count) { return; }
	
	// What should dynCheck do?
	// it should make the work easier to do -> check if the array is full or not, if full, realloc it, else, dont;
	// what is count here? it says How many elements of array is full.
	
	// check if the array is full or not.
	// i will define a macro named 'isNear' to make it more Easy.
	if (*count >= *capacity) {
		*capacity *= 2; // Update Capacity
		// Why *= 2 and not += 2? Because O(1)
		struct lexValue *new_array = realloc(*array, (*capacity) * sizeof(struct lexValue)); 
		if (!new_array) {
			perror("Lexer reallocation failed.\n");
			exit(EXIT_FAILURE);
		}
		*array = new_array;
	}
}


struct TokenArray* lex(const char *str) {
	size_t size_str = strlen(str); size_t ip = 0; 
	unsigned char c;
	size_t count = 0;
	size_t capacity = 16;
	
	size_t lines = 1;
	size_t cols = 1;

	
	struct lexValue* lexed = malloc(capacity * sizeof(struct lexValue));
	if (!lexed) {
		perror("Lexer allocation failed\n"); 
		lexed=NULL;
		return NULL;
	}
	
	while (size_str > ip) {
		c = str[ip];
		
		// if its a space, then we should update char too
		// why should we even update the cols/lines in isspace??
		// imagine this:
		//   We should lex this string : local x =    local;
		// if we dont advance our char, the col stays at = ,
		// and Our error won't tell the Exact colum that the error is comming from,
		// and thats is bad.
		if (isspace((unsigned char)c)) { advanceChar(str[ip],  &lines, &cols); ip++; continue; }
		
		SourcePos start = {lines, cols};
		
		TokenType t = charTokens[(unsigned char)c];
		if (t) {
			dynCheck(&lexed, &capacity, &count);
		
			lexed[count].t.Type = t;
			lexed[count].v.operator = c;
			lexed[count].pos = start;
			count++;
			
			advanceChar(str[ip],  &lines, &cols);
			
			// Manual ip++, just to make sure it advances to the next char
			ip++;
		} else {
			// Handle Mult-Character Things...
			// How it should Handle Multi-Character things?
			
			if (isdigit((unsigned char)c)) {
				// what if it is a digit?
				// While it is a ditgit, read it, save it, Emit it as a Number
				
				// long int because Numbers can be Huge. like (9999999999...)
				long int number = 0;
				while (ip < size_str && isdigit((unsigned char)str[ip])) {
					number = number * 10 + (str[ip] - '0');
					
					advanceChar(str[ip],  &lines, &cols);
					ip++; 
				}
				
				dynCheck(&lexed, &capacity, &count);
				lexed[count].t.Type = Number;
				lexed[count].v.number = number;
				lexed[count].pos = start;
				count++;
			} 
			else if (isalpha((unsigned char)c) || c == '_' ) {
				// What if it is a alpha character?
				// While its a ALpha Character OR its a Digit, Save it, Emit it as a String
				
				struct TypeString* string = newString("");
				while (ip < size_str && (
						isalpha((unsigned char)str[ip]) || 
						isdigit((unsigned char)str[ip]) || 
						str[ip] == '_' ) )
				{
					char buffer[2] = { str[ip], '\0' };
					StringAppend(string, buffer);
					
					advanceChar(str[ip],  &lines, &cols);
					ip++;
				}
				
				dynCheck(&lexed, &capacity, &count);
				
				// Genuis! one Function, Two work!
				lexed[count].t.Type = isKeyword(string->str);
				// why we need strdup (we are even on Windows)?
				// a issue that will happend when strdup is removed
				// it will get a Access Denied return in Windows (Return result: 3221226356)
				
				// but why it gives that Error? string->str is a Pointer. and it gets Freed later.
				// so we need to Duplicate it with a New Heap so it stays.
				
				// more simple, life-time issues.
				lexed[count].v.string = strdup(string->str);  
				lexed[count].pos = start;
								
				FreeString(string);
				
				count++;
			} else {
				error_at(start, "Unknown token : '%c'", c);
			}
			
		}
		
		// for String lexing, this will make Identifiers get skiped
		// ip++; 
	}
	
	dynCheck(&lexed, &capacity, &count);
	lexed[count].t.Type = Eof;// Eof
	lexed[count].pos = (SourcePos){lines, cols};

	count++;
	
	TokenArray *result = malloc(sizeof(TokenArray));
	if (!result) { 
		perror("Result allocation failed");
		result=NULL;
		return NULL;
	}
	
	result->data = lexed;
	result->count = count;
	
	// Before Returning, Dont do a Full Token Cleanup in the function.
	// Why?
	// It will free the Data we want to use Later.
	// We will make a Static Helper to do the job;
	
	return result;
}


// Now we need a Parser.
// what the Parser should do?
// it should go through Lexed tokens
// Just make them into a AST.
// Super simple.
// a While loop parser function, Chain of Expectations.

// But before doing those, this is the question:
//    Why make a AST while we can Emit bytecode directly?
//    We are not making a Language, We want to make a Lua Optimizer.


// no struct for now, only functions, structs will be defined by how functions work.



typedef enum {
	astNone = 0,
	AST_Number,
	AST_String,
	
	AST_BinaryOp,
	AST_UnaryOp,
	
	AST_Block,
	
	AST_Call,
	AST_Function,
	AST_Return,
	
	AST_If,
	AST_While,
	AST_Repeat,
	AST_ForNumeric,
	
	AST_Assign,
	AST_Identifier,
	
	AST_Table,
	AST_Index
} astType;

typedef struct {
	TokenArray* array;
	size_t pos;
} parseValue;

struct astNode {
	astType type;
	
	// Memory efficient union :o
	union {
		double number;
		// double number; floats/doubles not supported for now
		
		char* string;
		
		struct {
			struct astNode* left;
			struct astNode* right;
			
			char op;
		} Binary;
		
		// what a variable should have?
		// a value and a identifier
		struct {
			struct astNode* value;
			char* name;
		} Assign;
		
		// now we need Support for Block
		// what is a Block Ast?
		// its a storage for Parser to store Multiple statements.
		// Example:
		//    if (1 == 1) then 
		//      print(1);
		//	  end 
		// This is a Block.
		// now what Block needs to have?
		// it needs a astNode**.
		// why astNode**? 
		// because astNode* will only contain one Node, but we need a Array of astNodes.
		// now we need a astNode** and a count
		
		struct {
			struct astNode** stmts;
			size_t count;
		} Block;
		
		// Now we need a If block.
		// what it needs to have?
		// maybe a condition, a thenBlock and a elseBlock (later we add elseifBlock)
		
		struct {
			struct astNode* condition;
			struct astNode* thenBlock;
			struct astNode* elseBlock;
		} If;
	};
};


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

static inline void expect(parseValue* p, TokenType t) { // Why not make TokenType t a pointer?
	// It will just make things harder, not worth it. and TokenType is just a single integer.
	if (current(p)->t.Type != t) {
		// use our fancy error_at function for cleaner errors
		error_at(current(p)->pos, "Expected <%s>, got <%s>",TokenToString(t), TokenToString(current(p)->t.Type));
	}
}

static inline struct astNode* ast_alloc(void) {
	struct astNode* node = malloc(sizeof(struct astNode));
	if (!node) {
		perror("astNode allocation failed");
		node=NULL; // just for practice
		exit(EXIT_FAILURE);
	}
	
	// Return raw node.
	// Why?
	// Because node is a Local variable defined in ast_alloc, 
	// So returning the pointer of it is the same as NULL
	// because node wont live Much and its in the stack not the Heap.
	return node; 

}

static inline int op_primary(char op) {
	switch(op) {
		case '^': { return 3; }
	
		case '*':
		case '/': { return 2; };
		
		case '+':
		case '-': { return 1; };
		
		default: { return -1; };
	}
}

// for our Simple helper, we need a array that keeps the terminator keywords

const char* termKeywords[TermKeySize] = {
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


// we need to print our parsed AST result.
// we also need a Simple switch-case function that does astType -> string.

char* AstToString(astType AstType) {
	
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
			
        case AST_Identifier: {
            printf("Identifier: %s\n", node->string);
            break;
        };

        case AST_BinaryOp: {
            printf("BinaryOp: %c\n", node->Binary.op);
            PrintMyAST(node->Binary.left, indent + 1);
            PrintMyAST(node->Binary.right, indent + 1);
            break;
    	};

        case AST_Assign: {
            printf("Assign: %s =\n", node->Assign.name);
            PrintMyAST(node->Assign.value, indent + 1);
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



// Prototypes to keep things Clean.
struct astNode* parse_number(parseValue *p);
struct astNode* parse_string(parseValue *p);
struct astNode* parse_expression(parseValue* p, int pro);
struct astNode* parse_block(parseValue* p);
struct astNode* parse_if(parseValue* p);
struct astNode* parse_primary(parseValue *p);
struct astNode* parse_statement(parseValue* p);
struct astNode* parse_block(parseValue* p);

struct astNode* parse(TokenArray* lexed);


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

struct astNode* parse_string(parseValue *p) {
	// Check if the current Token is a Identifier or not
	expect(p, Identifier);
	

	struct astNode* identNode = ast_alloc();
	// What should parse_string do?
	// It should just make a Simple Number(intger) in the AST.
	// and return it
	
	identNode->type = AST_Identifier;
	// why a strdup?
	// because it will prevent use-after-freed issues.
	// TokenCleanup will clean all of strings and identifiers.
	identNode->string = strdup(current(p)->v.string);
	advance(p); // Pos++;
	
	return identNode;
}


struct astNode* parse_expression(parseValue* p, int pro) {
	struct astNode* left = parse_primary(p);
	
	while (current(p)->t.Type == BinaryOp) {
		// Get the operator for prec.
		// use const, why not? we dont need to change any of these 2 variables
		const char op = current(p)->v.operator;
		const int prec = op_primary(op);
		
		if (prec < pro) { break; }
		advance(p);
		
		// Lua ^ is right associative
		// DO NOT do prec+1 when the operator is ^
		
		// this line means :
		//    if ( op == '^' ) return prec
		//    else return prec+1;
		int Prec = (op == '^') ? prec : prec+1;
		
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
		case Number: {
			return parse_number(p);
		};
		
		case Identifier: {
			return parse_string(p);	
		};
		
		case OpenParen: {
			advance(p); // Pass over (
			
			struct astNode* parenNode = parse_expression(p, 0);
			
			expect(p, CloseParen);
			advance(p); // Pass over )
			
			return parenNode;
		}
		
		
		// EOF safety
		case Eof: {
			// why not return NULL?
			// Why do? it will cause UB;
			error_at(current(p)->pos, "Unexpected <eof>");
			break;
		}
		
		default: {
			error_at(
				current(p)->pos,
				"unexpected <token> '%s'",
				TokenToString(current(p)->t.Type)
			);
		}
	}
	
	// Do not worry, this return wont be reached. 
	// just to make GCC chill from giving warnings
	return NULL;
}



struct astNode* parse_statement(parseValue* p) {
	
	switch (current(p)->t.Type) {
		case Keyword: {
			// What should it do?
			// If keyword -> local then :
			if (strcmp(current(p)->v.string, "local") == 0) {
				advance(p); // pass over local 
				
				// Initilize early,
				// because we need to use it now
				struct astNode* assignNode = ast_alloc();
				
				expect(p,Identifier);
				// More efficient, and more Safe
				assignNode->Assign.name = strdup(current(p)->v.string);
				advance(p);
				
				if (current(p)->t.Type == Equal) { advance(p); }
				
				
				struct astNode* expr = parse_expression(p, 0);
				// If semicolon, Advance so prevent later issues
				if (current(p)->t.Type == Semicolon) { advance(p); }
				
				
				assignNode->type = AST_Assign;
				assignNode->Assign.value = expr;
				
				return assignNode;
			}
			
			if (strcmp(current(p)->v.string, "if") == 0) {
				return parse_if(p);
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
			// If semicolon, Advance so prevent later issues
			if (current(p)->t.Type == Semicolon) { advance(p); }
							
			// Initilize a Node to return it as astNode*;
			struct astNode* assignNode = ast_alloc();
			assignNode->type = AST_Assign;
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




struct astNode* parse(TokenArray* lexed) {
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
	
	return parse_block(&p);
}





// Now for the EXITING PART! OPTIMIZATIONS!
// a Simple Constant Folding will be Cool.

struct astNode* fold_constants(struct astNode* node) {
    if (!node) return NULL;

    switch (node->type) {
		case AST_Block: {
			// Why a Loop for calling fold_constants and not just call it with the block statement directly?
			// because **stmt has more than one AST nodes stored in it self, so we need to index it with a for loop.
			for (size_t i=0;i<node->Block.count;i++) {
				fold_constants(node->Block.stmts[i]);
			}
			return node;
		}; 
        case AST_BinaryOp: {
            node->Binary.left  = fold_constants(node->Binary.left);
            node->Binary.right = fold_constants(node->Binary.right);

            struct astNode* L = node->Binary.left;
            struct astNode* R = node->Binary.right;
			
			// prevent passing L or R as NULL
            if (L && R && L->type == AST_Number && R->type == AST_Number) {

                double result;
                switch (node->Binary.op) {
                    case '+': result = L->number + R->number; break;
                    case '-': result = L->number - R->number; break;
                    case '*': result = L->number * R->number; break;
                    case '/': { 
                    	if (R->number == 0) { return node; }
						result = L->number / R->number; 
						break;
					}
                    case '^': {
                    	// a Mini guard to keep things safe :)
                    	if (R->number < 0) return node;
						result = pow(L->number, R->number);
						break;
                    }
                    default: return node;
                }
                AstCleanup(L);
        		AstCleanup(R);

                node->type = AST_Number;
                node->number = result;
            }
            

            return node;
        }

        case AST_Assign: {
            node->Assign.value = fold_constants(node->Assign.value);
            return node;
		};
	
        default: {
            return node;
        };
    }
}


struct astNode* optimize(struct astNode* parsed) {
	struct astNode *node;
	node = parsed;
	
	// Optimization passes
	node = fold_constants(node);
	
	
	// return optimized node
	return node;
}


// What is next Tomorrow?
// i need to make a free_ast() function <--- Done
// i need to Fix the ^ operator. <--- Done
// I need to add support for functions, loops e.g




// Now we need IR Optimizations, for the IR Optimizations,
// We need two new structs, 
// we will Remove the AST Based Constant fold with a IR one.

/*
typedef struct {
    char* name;
    double value;
} ConstEntry;

typedef struct {
    ConstEntry* data;
    size_t count;
    size_t cap;
} ConstTable;
*/


// Now we need a IR!
// why a IR?
// AST is only good for sytnax, but its a nightmare for optimizations :(

// IR is a exiting thing!
// what we need to implent a enum for our IR

// a small enum for IR
typedef enum {
	IR_BinaryOp,
	IR_Assign,
	IR_LoadConst,
	
	IR_ToBool, // Needed for if statements
	
	IR_Label,
	IR_Jump,
	IR_JumpIfFalse,
} IRType;

typedef struct {
	IRType type;
	
	// memory efficent union :o
	union {
		
		struct {
			// what Binary needs?
			// a Operator, a place to save the result,
			// left and right
			
			char op;
			
			char* result;
			
			char* left;
			char* right;
			
			// But why char* and not int?
			// IR is symbolic, left could be t0, making Types, Consts... will make it MUCH harder
			
			// char* is flexible and simple
		} Binary;
		
		struct {
			// what Assign needs?
			// a Name, and a Value
			char* identifier;
			char* value;
		} Assign;
		
		// we need a LoadConst struct for the IR_LoadConst Type;
		// what should it contain? a name and a long int?
		// we said that IR needs strings insted of integers, why use long int?
		
		// Because : the Optimizer will work on Numeric values and not strings.
		// and strings are super expensive to store, it needs memory allocation, 
		// but integers can just stay in the struct! Memory Efficient, Fast!! :o
		
		struct {
			char* identifier;
			double value;
		} LoadConst;
		
		// Now for If statements support in our IR,
		// we will make some structs that have label name, target jump, conditions ...
		
		struct {
			char* labelName;
		} Label;
		
		struct {
			char* target;
		} Jump;
		
		struct {
			char* condition;
			char* target;
		} JumpIfFalse;
		
		// Now for IR_ToBool, we need a struct that stores result and value
		struct {
			char* result;
			char* value;
		} ToBool;
	};
} IRValue;

typedef struct {
	IRValue* value;
	// what other thing it needs?
	// it needs capacity, count
	size_t capacity;
	size_t count;
} IRArray;

// Now we will write some Helpers to make IR more easy to write

// now we need a function to generate temp variable names for the IR!
// what we need? a static integer to store the temps we made.
// and a function that just does a concat with a t + temp_int

static int tempCount = 0;

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

void IREmitLabel(IRArray* ir, const char* labelName) {
	IRValue IRVal;
	
	IRVal.type = IR_Label;
	IRVal.Label.labelName = strdup(labelName);
	
	IREmit(ir, IRVal);
}

void IREmitJump(IRArray* ir, const char* target) {
	IRValue IRVal;
	
	IRVal.type = IR_Jump;
	IRVal.Jump.target = strdup(target);
	
	IREmit(ir, IRVal);
}

void IREmitJumpIfFalse(IRArray* ir, const char* condition, const char* target) {
	IRValue IRVal;
	
	IRVal.type = IR_JumpIfFalse;
	IRVal.JumpIfFalse.condition = strdup(condition);
	IRVal.JumpIfFalse.target = strdup(target);
	
	IREmit(ir, IRVal);
}



// now we need a AST -> IR function. it returns a char*
char* IRGen(struct astNode* node, IRArray* ir) {
	// safety check
	if (!node || !ir) { 
		// return NULL;
		// why not return NULL? it will make UB!
		printf("IRGen called with missing argunments\n");
		exit(EXIT_FAILURE);
	}
	
	switch(node->type) {
	
		case AST_Number: {
			// generate temp name
			char* tempName = newTemp();
			
			IRValue val;
			val.type = IR_LoadConst;
			val.LoadConst.identifier = strdup(tempName);
			val.LoadConst.value = node->number;
			
			IREmit(ir, val);
			
			return tempName;
		};
		
		case AST_Identifier: {
			return strdup(node->string);
		};
		
		case AST_BinaryOp: {
			char* left = IRGen(node->Binary.left, ir);
			char* right = IRGen(node->Binary.right, ir);
			
			// generate temp name
			char* tempName = newTemp();
			
			IRValue val;
			val.type = IR_BinaryOp;
			val.Binary.result = strdup(tempName);
			val.Binary.op = node->Binary.op;
			val.Binary.left = strdup(left);
			val.Binary.right = strdup(right);
			
			// We need to free left and right before returning anything,
			// to prevent memory leaks
			free(left);
			free(right);
			
			IREmit(ir, val);
			
			return tempName;
		};
		
		
		// default case
		default: { return NULL; }
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
	
	char elseLabel[32];
	char endLabel[32];
	
	snprintf(elseLabel, sizeof(elseLabel), "elseLabel_%d", labelCount);
	snprintf(endLabel, sizeof(endLabel), "endLabel_%d", labelCount);
	labelCount++;
	
	char* raw = IRGen(node->If.condition, ir);
	char* boolTemp = newTemp();
	
	IRValue val;
	val.type = IR_ToBool;
	val.ToBool.result = strdup(boolTemp);
	val.ToBool.value = strdup(raw);
	
	IREmit(ir, val);
	
	IREmitJumpIfFalse(ir, boolTemp, elseLabel);
	
	IRStmtGen(ir, node->If.thenBlock);
	
	IREmitJump(ir, endLabel);
	
	IREmitLabel(ir, elseLabel);

	// to prevent memory leaks
	free(raw);
	free(boolTemp);
	
	if (node->If.elseBlock) {
		IRStmtGen(ir, node->If.elseBlock);
	}
	
	IREmitLabel(ir, endLabel);
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
		    char* value = IRGen(node->Assign.value, ir);
		    IRValue v;
		    v.type = IR_Assign;
		    v.Assign.identifier = strdup(node->Assign.name);
		    v.Assign.value = strdup(value);

			// Free value before returning
			free(value);

		    IREmit(ir, v);
		    
		    break;
		};
		
		case AST_If: {
			IRGenIf(ir, node);
			break;
		}
		
		// default Case
		default: { break; }
	}
}


void IRPrint(IRArray* ir) {
    for (size_t i = 0; i < ir->count; i++) {
        IRValue* v = &ir->value[i];

        switch (v->type) {

            case IR_LoadConst: {
                printf("%s = %f\n",
                       v->LoadConst.identifier,
                       v->LoadConst.value);
                break;
            };

            case IR_BinaryOp: {
                printf("%s = %s %c %s\n",
                       v->Binary.result,
                       v->Binary.left,
                       v->Binary.op,
                       v->Binary.right);
                break;
            };

            case IR_Assign: {
                printf("%s = %s\n",
                       v->Assign.identifier,
                       v->Assign.value);
                break;
            };
            
            case IR_Label: {
                printf("%s: ; Create a Label named %s\n", v->Label.labelName, v->Label.labelName);
                break;
            }

            case IR_Jump:
                printf("goto %s ; Jump to Label %s\n", v->Jump.target, v->Jump.target);
                break;

            case IR_JumpIfFalse: {
                printf("if_false %s goto %s ; Jump to Label %s if %s is False\n",
                       v->JumpIfFalse.condition,
                       v->JumpIfFalse.target,
					   v->JumpIfFalse.target,
					   v->JumpIfFalse.condition);
                break;
            };
            case IR_ToBool: {
			    printf("%s = tobool %s ; Convert %s to a Boolean value and store it in %s\n",
			           v->ToBool.result,
			           v->ToBool.value,
					   v->ToBool.value,
					   v->ToBool.result);
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
			case IR_LoadConst: {
				free(IRVal->LoadConst.identifier);
				break;
			};
			
			case IR_Assign: {
				free(IRVal->Assign.identifier);
				free(IRVal->Assign.value);
				break;
			};
			
			case IR_BinaryOp: {
				free(IRVal->Binary.left);
				free(IRVal->Binary.result);
				free(IRVal->Binary.right);
				break;
			};
			
			case IR_Label: {
				free(IRVal->Label.labelName);
				break;
			};
			
			case IR_Jump: {
				free(IRVal->Jump.target);
				break;
			};
			
			case IR_JumpIfFalse: {
				free(IRVal->JumpIfFalse.condition);
				free(IRVal->JumpIfFalse.target);
				break;
			};
			
			case IR_ToBool: {
				free(IRVal->ToBool.result);
				free(IRVal->ToBool.value);
				break;
			};
			
			default: { break; }
		}
	}
	// free the ir value container
	free(ir->value);
	
}


int main(void) {
	// Test

	/* 
	char *code = 
		"local x = 1 + 12\n"
		"if (x) then\n"
		"	local g = 1;\n"
		"else\n"
		"	if (g) then\n"
		"		local p = 1;\n"
		"	else\n"
		"		local p = 0;\n"
		"	end\n"
		"	local g = 0\n"
		"end\n";
	*/
		
	char *code = 
		"if (x) then\n"
		"	local g = 1;\n"
		"else\n"
		"	local g = 0;\n"
		"end\n";
	struct TokenArray* lexed = lex(code);
	
	printMyTokens(lexed);
	
	struct astNode* node = parse(lexed);
	PrintMyAST(node, 0);
	
	// Optimization Test.
	node = optimize(node);
	PrintMyAST(node, 0);
	
	// IR part
	IRArray ir;
	IRAlloc(&ir);
	
	IRStmtGen(&ir, node);
	IRPrint(&ir);
		
	// Free before ending
	TokenCleanup(lexed);
	AstCleanup(node);
	IRCleanup(&ir);
	
	return 0;
}
