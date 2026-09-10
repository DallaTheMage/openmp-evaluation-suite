#ifndef TEST_H
#define TEST_H
    #include "core/context.h"
    #include "core/logger.h"
    #include "data/writers/writer.h"

    typedef struct Test Test;

    struct Test {
        const char *testname;
        int (*run)(ResultWriter *writer, WorkContext *ctx, Logger *logger);
    };

    #define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

    void destroy_collections(WorkContext *ctx);
    void cleanup_test_context(WorkContext *ctx, ResultWriter *writer);
    void benchmark_routine(WorkContext *ctx, void (*run)(WorkContext *), TestResult *result);

    const Test *get_test_set(void);
    size_t get_test_count(void);

#endif /* TEST_H */