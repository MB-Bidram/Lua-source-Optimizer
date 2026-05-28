#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "../ir.h"
#include "constant_fold.h"
#include "dead_code.h"

void optimize(IRArray* ir);

#endif
