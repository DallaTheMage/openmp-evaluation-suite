#ifndef TEST_H
#define TEST_H

#include <stddef.h>
#include "core/context.h"
#include "core/logger.h"
#include "data/writers/writer.h"
#include "data/result.h"

typedef struct Test Test;

struct Test {
    const char *name;
    int (*run)(GeneralContext *ctx);
};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

void destroy_collections(WorkContext *ctx);
void cleanup_test_context(WorkContext *ctx, ResultWriter *writer, Logger *logger);

// Nuova firma: scrive direttamente i campioni grezzi tramite il writer
void benchmark_routine(WorkContext *ctx, void (*run)(WorkContext *),
                       ResultWriter *writer, RawSample *base_sample);

const Test *get_test_set(void);
size_t get_test_count(void);

#endif /* TEST_H */
