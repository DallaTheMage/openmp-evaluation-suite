#ifndef CONTEXT_H
#define CONTEXT_H

#include "data/collection.h"

typedef struct WorkContext {
    Collection *input;
    Collection *output;
    int chunksize;
    int threadnumber;
    int warmup_iterations;
    int work_iterations;
} WorkContext;

#endif /* CONTEXT_H */