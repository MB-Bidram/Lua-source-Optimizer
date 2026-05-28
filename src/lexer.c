#include "lexer.h"
#include "utils.h"

static int keyword_allowed(const char* kw, const LuaVersionInfo* vinfo);
static const LuaVersionInfo* lexer_version_info(LuaVersion version);


const TokenType charTokens[256] = {
    ['+'] = BinaryOp,
    ['-'] = BinaryOp,
    ['*'] = BinaryOp,
    ['/'] = BinaryOp,
    ['^'] = BinaryOp,
    ['%'] = BinaryOp,
    ['&'] = BinaryOp,
    ['|'] = BinaryOp,

    ['('] = OpenParen,
    [')'] = CloseParen,
    ['{'] = OpenBrace,
    ['}'] = CloseBrace,
    ['['] = OpenBracket,
    [']'] = CloseBracket,

    [','] = Comma,
    [';'] = Semicolon,
};



static const char* keywords[] = {
    "and", "break", "do", "else", "elseif", "end", "false",
    "for", "function", "goto", "if", "in", "local", "nil",
    "not", "or", "repeat", "return", "then", "true", "until", "while"
};



static TokenType isKeyword(const char* str, const LuaVersionInfo* vinfo) {
    size_t n = sizeof(keywords) / sizeof(keywords[0]);

    for (size_t i = 0; i < n; i++) {
        if (strcmp(str, keywords[i]) == 0) {
            if (!keyword_allowed(str, vinfo)) {
                return Identifier;
            }

            if (strcmp(str, "true") == 0 || strcmp(str, "false") == 0) return Boolean;
            if (strcmp(str, "nil") == 0) return Nil;
            return Keyword;
        }
    }

    return Identifier;
}



static const LuaVersionInfo* lexer_version_info(LuaVersion version) {
    return get_lua_version_info(version);
}

static int keyword_allowed(const char* kw, const LuaVersionInfo* vinfo) {
    if (strcmp(kw, "goto") == 0) {
        return vinfo->has_goto;
    }
    return 1;
}


// Helper functions to make Printing the results more Easy

const char* TokenToString(const TokenType token) {
	// why not return NULL?
	// why return NULL when we can just return TokenNone.
	if (!token) { return "TokenNone"; }		

	switch (token) {
		case Keyword: return "Keyword";
		case Number: return "Number";
		case String: return "String";
		case Identifier: return "Identifier";
		case Boolean: return "Boolean";
		case Nil: return "Nil";

		case BinaryOp: return "BinaryOp";
		case UnaryOp: return "UnaryOp";

		case OpenParen: return "OpenParen";
		case CloseParen: return "CloseParen";
		case OpenBrace: return "OpenBrace";
		case CloseBrace: return "CloseBrace";
		case OpenBracket: return "OpenBracket";
		case CloseBracket: return "CloseBracket";

		case Comma: return "Comma";
		case Semicolon: return "Semicolon";
		case Dot: return "Dot";
		case Colon: return "Colon";

		case Equal: return "Equal";
		case EqualEqual: return "EqualEqual";
		case NotEqual: return "NotEqual";
		case Less: return "Less";
		case LessEqual: return "LessEqual";
		case Greater: return "Greater";
		case GreaterEqual: return "GreaterEqual";

		case Concat: return "Concat";
		case VarArg: return "VarArg";
		case DoubleColon: return "DoubleColon";
		case FloorDiv: return "FloorDiv";
		case ShiftLeft: return "ShiftLeft";
		case ShiftRight: return "ShiftRight";

		case Eof: return "Eof";
		default: return "TokenNone";
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
            case Number: printf("  Value: Number '%f'\n", array->data[i].v.number); break;

            case Identifier:
            case Keyword:
            case String: printf("  Value: String '%s'\n", array->data[i].v.string); break;

            case Boolean: printf("  Value: Boolean '%s'\n", array->data[i].v.boolean ? "true" : "false"); break;

            case BinaryOp:
            case UnaryOp: printf("  Value: Operator '%c'\n", array->data[i].v.operator); break;

            default: printf("   Value: None ''\n"); break;
        }
		
	}
}


void TokenCleanup(TokenArray *array) {
	if (!array) { return; }

	// What should TokenCleanup do?
	// Just do the Cleaning up more Easy
	for (size_t i=0;i<array->count;i++) {
		switch (array->data[i].t.Type) {
			case Keyword:
			case Identifier:
			case String: {
				free(array->data[i].v.string);
				break;
			}
			
			default: {
				
			}
		}
	}
	free(array->data);
	free(array);
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

static inline int peek_char(const char* str, size_t size_str, size_t ip, size_t offset) {
	size_t idx = ip + offset;
	if (idx >= size_str) return 0;
	return (unsigned char)str[idx];
}

static void emit_simple_token(struct lexValue **lexed, size_t *capacity, size_t *count,
                              TokenType type, SourcePos pos) {
	dynCheck(lexed, capacity, count);
	(*lexed)[*count].t.Type = type;
	(*lexed)[*count].pos = pos;
	(*count)++;
}

static void emit_op_token(struct lexValue **lexed, size_t *capacity, size_t *count,
                          TokenType type, char op, SourcePos pos) {
	dynCheck(lexed, capacity, count);
	(*lexed)[*count].t.Type = type;
	(*lexed)[*count].v.operator = op;
	(*lexed)[*count].pos = pos;
	(*count)++;
}


struct TokenArray* lex(const char *str, LuaVersion version) {
	const LuaVersionInfo* vinfo = lexer_version_info(version);

	size_t size_str = strlen(str), ip = 0;
	size_t count = 0, capacity = 16;
	size_t lines = 1, cols = 1;

	struct lexValue* lexed = malloc(capacity * sizeof(struct lexValue));
	if (!lexed) {
		perror("Lexer allocation failed\n");
		return NULL;
	}

	while (ip < size_str) {
		unsigned char c = (unsigned char)str[ip];

		// if its a space, then we should update char too
		// why should we even update the cols/lines in isspace??
		// imagine this:
		//   We should lex this string : local x =    local;
		// if we dont advance our char, the col stays at = ,
		// and Our error won't tell the Exact colum that the error is comming from,
		// and thats is bad.
		if (isspace(c)) {
			advanceChar(str[ip], &lines, &cols);
			ip++;
			continue;
		}

		SourcePos start = {lines, cols};

		// Handle comments first
		// Why first?
		// because -- starts with -, and - is also a operator.
		// if we dont do this first, comment lexing gets cooked badly :D
		if (c == '-' && peek_char(str, size_str, ip, 1) == '-') {
			advanceChar(str[ip], &lines, &cols); ip++;
			advanceChar(str[ip], &lines, &cols); ip++;

			// For now only single-line comments.
			// Long comments [[...]] can be added after this without rewriting half the lexer.
			while (ip < size_str && str[ip] != '\n') {
				advanceChar(str[ip], &lines, &cols);
				ip++;
			}
			continue;
		}

		// Handle strings
		// Lua has "..." and '...'
		if (c == '"' || c == '\'') {
			char quote = (char)c;
			struct TypeString* string = newString("");
			if (!string) exit(EXIT_FAILURE);

			advanceChar(str[ip], &lines, &cols);
			ip++; // pass over opening quote

			while (ip < size_str && str[ip] != quote) {
				// Mini escape support
				// not full Lua escapes yet, but way better than nothing.
				if (str[ip] == '\\' && ip + 1 < size_str) {
					char esc = str[ip + 1];
					char out[2] = {0, 0};

					switch (esc) {
						case 'n': out[0] = '\n'; break;
						case 't': out[0] = '\t'; break;
						case 'r': out[0] = '\r'; break;
						case '\\': out[0] = '\\'; break;
						case '"': out[0] = '"'; break;
						case '\'': out[0] = '\''; break;
						default: out[0] = esc; break;
					}

					StringAppend(string, out);
					advanceChar(str[ip], &lines, &cols); ip++;
					advanceChar(str[ip], &lines, &cols); ip++;
					continue;
				}

				char buffer[2] = { str[ip], '\0' };
				StringAppend(string, buffer);
				advanceChar(str[ip], &lines, &cols);
				ip++;
			}

			if (ip >= size_str) {
				FreeString(string);
				error_at(start, "Unterminated string");
			}

			advanceChar(str[ip], &lines, &cols);
			ip++; // pass over closing quote

			dynCheck(&lexed, &capacity, &count);
			lexed[count].t.Type = String;
			lexed[count].v.string = strdup(string->str);
			lexed[count].pos = start;
			count++;

			FreeString(string);
			continue;
		}

		// Handle numbers
		// We now support:
		//    123
		//    12.34
		// We still dont support:
		//    0x12
		//    1e10
		//    0x1p4
		// Lua number lexing is a monster, we are not summoning that demon fully right now.
		if (isdigit(c) || (c == '.' && isdigit(peek_char(str, size_str, ip, 1)))) {
			char* endptr;
			double number = strtod(&str[ip], &endptr);
			size_t consumed = (size_t)(endptr - &str[ip]);

			for (size_t i = 0; i < consumed; i++) {
				advanceChar(str[ip], &lines, &cols);
				ip++;
			}

			dynCheck(&lexed, &capacity, &count);
			lexed[count].t.Type = Number;
			lexed[count].v.number = number;
			lexed[count].pos = start;
			count++;
			continue;
		}

		// Handle identifiers / keywords / booleans / nil
		if (isalpha(c) || c == '_') {
			struct TypeString* string = newString("");
			if (!string) exit(EXIT_FAILURE);

			while (ip < size_str &&
			       (isalpha((unsigned char)str[ip]) ||
			        isdigit((unsigned char)str[ip]) ||
			        str[ip] == '_')) {
				char buffer[2] = { str[ip], '\0' };
				StringAppend(string, buffer);
				advanceChar(str[ip], &lines, &cols);
				ip++;
			}

			TokenType kind = isKeyword(string->str, vinfo);

			dynCheck(&lexed, &capacity, &count);
			lexed[count].t.Type = kind;
			lexed[count].pos = start;

			if (kind == Boolean) {
				lexed[count].v.boolean = (strcmp(string->str, "true") == 0);
			} else if (kind == Nil) {
				// no payload needed
			} else {
				lexed[count].v.string = strdup(string->str);
			}

			count++;
			FreeString(string);
			continue;
		}

		// Handle multi-char operators / punctuation
		if (c == '=') {
			if (peek_char(str, size_str, ip, 1) == '=') {
				emit_simple_token(&lexed, &capacity, &count, EqualEqual, start);
				advanceChar(str[ip], &lines, &cols); ip++;
				advanceChar(str[ip], &lines, &cols); ip++;
			} else {
				emit_simple_token(&lexed, &capacity, &count, Equal, start);
				advanceChar(str[ip], &lines, &cols); ip++;
			}
			continue;
		}

		if (c == '~') {
			if (peek_char(str, size_str, ip, 1) == '=') {
				emit_simple_token(&lexed, &capacity, &count, NotEqual, start);
				advanceChar(str[ip], &lines, &cols); ip++;
				advanceChar(str[ip], &lines, &cols); ip++;
			} else {
				if (!vinfo->has_bitwise) {
					error_at(start, "Bitwise not '~' is only valid in Lua 5.3+");
				}
				emit_op_token(&lexed, &capacity, &count, UnaryOp, '~', start);
				advanceChar(str[ip], &lines, &cols); ip++;
			}
			continue;
		}

		if (c == '<') {
			if (peek_char(str, size_str, ip, 1) == '=') {
				emit_simple_token(&lexed, &capacity, &count, LessEqual, start);
				advanceChar(str[ip], &lines, &cols); ip++;
				advanceChar(str[ip], &lines, &cols); ip++;
			} else if (peek_char(str, size_str, ip, 1) == '<') {
				if (!vinfo->has_shift_ops) {
					error_at(start, "Shift operator '<<' is only valid in Lua 5.3+");
				}
				emit_simple_token(&lexed, &capacity, &count, ShiftLeft, start);
				advanceChar(str[ip], &lines, &cols); ip++;
				advanceChar(str[ip], &lines, &cols); ip++;
			} else {
				emit_simple_token(&lexed, &capacity, &count, Less, start);
				advanceChar(str[ip], &lines, &cols); ip++;
			}
			continue;
		}

		if (c == '>') {
			if (peek_char(str, size_str, ip, 1) == '=') {
				emit_simple_token(&lexed, &capacity, &count, GreaterEqual, start);
				advanceChar(str[ip], &lines, &cols); ip++;
				advanceChar(str[ip], &lines, &cols); ip++;
			} else if (peek_char(str, size_str, ip, 1) == '>') {
				if (!vinfo->has_shift_ops) {
					error_at(start, "Shift operator '>>' is only valid in Lua 5.3+");
				}
				emit_simple_token(&lexed, &capacity, &count, ShiftRight, start);
				advanceChar(str[ip], &lines, &cols); ip++;
				advanceChar(str[ip], &lines, &cols); ip++;
			} else {
				emit_simple_token(&lexed, &capacity, &count, Greater, start);
				advanceChar(str[ip], &lines, &cols); ip++;
			}
			continue;
		}

		if (c == '/') {
			if (peek_char(str, size_str, ip, 1) == '/') {
				if (!vinfo->has_floor_div) {
					error_at(start, "Floor division '//' is only valid in Lua 5.3+");
				}
				emit_simple_token(&lexed, &capacity, &count, FloorDiv, start);
				advanceChar(str[ip], &lines, &cols); ip++;
				advanceChar(str[ip], &lines, &cols); ip++;
			} else {
				emit_op_token(&lexed, &capacity, &count, BinaryOp, '/', start);
				advanceChar(str[ip], &lines, &cols); ip++;
			}
			continue;
		}

		if (c == '.') {
			if (peek_char(str, size_str, ip, 1) == '.' &&
			    peek_char(str, size_str, ip, 2) == '.') {
				emit_simple_token(&lexed, &capacity, &count, VarArg, start);
				advanceChar(str[ip], &lines, &cols); ip++;
				advanceChar(str[ip], &lines, &cols); ip++;
				advanceChar(str[ip], &lines, &cols); ip++;
			} else if (peek_char(str, size_str, ip, 1) == '.') {
				emit_simple_token(&lexed, &capacity, &count, Concat, start);
				advanceChar(str[ip], &lines, &cols); ip++;
				advanceChar(str[ip], &lines, &cols); ip++;
			} else {
				emit_simple_token(&lexed, &capacity, &count, Dot, start);
				advanceChar(str[ip], &lines, &cols); ip++;
			}
			continue;
		}

		if (c == ':') {
			if (peek_char(str, size_str, ip, 1) == ':') {
				if (!vinfo->has_labels) {
					error_at(start, "Labels '::name::' are only valid in Lua 5.2+");
				}
				emit_simple_token(&lexed, &capacity, &count, DoubleColon, start);
				advanceChar(str[ip], &lines, &cols); ip++;
				advanceChar(str[ip], &lines, &cols); ip++;
			} else {
				emit_simple_token(&lexed, &capacity, &count, Colon, start);
				advanceChar(str[ip], &lines, &cols); ip++;
			}
			continue;
		}

		// Handle simple one-char table tokens
		TokenType t = charTokens[c];
		if (t) {
			if ((c == '&' || c == '|') && !vinfo->has_bitwise) {
				error_at(start, "Bitwise operator '%c' is only valid in Lua 5.3+", c);
			}

			if (t == BinaryOp || t == UnaryOp)
				emit_op_token(&lexed, &capacity, &count, t, (char)c, start);
			else
				emit_simple_token(&lexed, &capacity, &count, t, start);

			advanceChar(str[ip], &lines, &cols);
			ip++;
			continue;
		}

		error_at(start, "Unknown token : '%c'", c);
	}

	dynCheck(&lexed, &capacity, &count);
	lexed[count].t.Type = Eof;
	lexed[count].pos = (SourcePos){lines, cols};
	count++;

	TokenArray *result = malloc(sizeof(TokenArray));
	if (!result) {
		perror("Result allocation failed");
		free(lexed);
		return NULL;
	}

	result->data = lexed;
	result->count = count;
	result->version = version;
	return result;
}
