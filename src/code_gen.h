#ifndef CODE_GEN_H
#define CODE_GEN_H

#include <stdio.h>
#include "ir.h"

void CodeGenLua(const IRArray* ir, FILE* out);

#endif
