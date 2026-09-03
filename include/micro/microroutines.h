#ifndef CORE_MICROROUTINES_H
#define CORE_MICROROUTINES_H

#include <stddef.h>

#include "core/context.h"

typedef struct {
    const char *name;
    void (*run)(WorkContext *ctx);
} MicroRoutine;

const MicroRoutine *get_microroutines(void);
size_t get_microroutines_count(void);

#endif
