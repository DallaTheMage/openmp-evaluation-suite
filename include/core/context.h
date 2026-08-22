#ifndef CONTEXT_H
#define CONTEXT_H

#include "data/collection.h"

typedef struct WorkContext {
    Collection *input;
    Collection *output;
    int chunksize;
    int threadnumber;
    size_t warmup_iterations;
    size_t work_iterations;
} WorkContext;

#endif /* CONTEXT_H */