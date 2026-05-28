#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>

#define TermKeySize 4

typedef struct {
    size_t line;
    size_t col;
} SourcePos;

#endif

// This header is just to reduce retyping these again and again in other files
