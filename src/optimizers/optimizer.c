#include "optimizer.h"

void optimize(IRArray* ir) {
    if (!ir) return;

    fold_constants(ir);
    eliminate_dead_code(ir);
}
