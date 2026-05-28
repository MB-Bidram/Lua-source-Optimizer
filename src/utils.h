#ifndef UTILS_H
#define UTILS_H

#include "common.h"

typedef struct TypeString {
    char* str;
    size_t len;
} TypeString;

TypeString* newString(const char* str);
int StringAppend(TypeString* str, const char* str2);
int FreeString(TypeString* string);

char* my_strdup(const char* str);

void error_at(SourcePos pos, const char* fmt, ...);
void advanceChar(char c, size_t* line, size_t* col);

#endif
