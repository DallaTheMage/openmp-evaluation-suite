#ifndef COMMON_TEST_H
#define COMMON_TEST_H

#include "core/context.h"
#include "micro/microroutines.h"
#include "data/writers/writer.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

void destroy_collections(WorkContext *ctx);
void cleanup_test_context(WorkContext *ctx, ResultWriter *writer);
double benchmark_routine(WorkContext *ctx, void (*run)(WorkContext *));

#endif /* COMMON_TEST_H */
