#include "utils.h"
#include <stdlib.h>
#include <string.h>

struct TypeString* newString(const char* str) {
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

int StringAppend(struct TypeString* str, const char* str2) {
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
int FreeString(struct TypeString* string) {
	if (!string) { return 0; }

	free(string->str);
	free(string);
	
	return 1;
}


char* my_strdup(const char* s) {
    if (!s) {
        return NULL;
    }

    size_t len = strlen(s) + 1;
    char* copy = malloc(len);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, s, len);
    return copy;
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
inline void advanceChar(const char c, size_t* line, size_t* col) {
	if (c == '\n') { (*line)++; *col=1; }  else { (*col)++; }
}
