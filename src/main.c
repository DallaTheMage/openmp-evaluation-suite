#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "test/test.h"
#include "core/logger.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <output-file>\n", argv[0]);
        return 1;
    }

    ResultWriter *writer;
    WorkContext *ctx;
    Logger *logger;
    const char *header = "test_id,test_type,benchname,log2n,threads,chunksize,time_mean,time_min,time_max,time_variance,speedup,efficiency,overhead";
    const Test *tests = get_test_set();
    const size_t num_tests = get_test_count();
    writer = create_writer();
    ctx = (WorkContext *)malloc(sizeof(WorkContext));
    logger = create_logger();
    char *output_filename;
    for (size_t i = 0; i < num_tests; ++i) {
        output_filename = writer->operations.prepare_filepath(tests[i].testname);
        if (writer->operations.clean(writer, output_filename) != 1) {
            logger->error("Error cleaning file");
            cleanup_test_context(ctx, writer);
            return 1;
        }
        if (writer->operations.open(writer, output_filename, "a", header) != 1) {
            logger->error("Error opening file for writing");
            cleanup_test_context(ctx, writer);
            return 1;
        }
        logger->log("Starting test execution...");
        logger->info(tests[i].testname, tests[i].run(writer, ctx, logger) == 0);
        writer->operations.flush(writer);
        writer->operations.close(writer);
        printf("=== Finished %s ===\n", tests[i].testname);
    }
    logger->log("All tests completed successfully.");
    logger->info("All tests", true);
    cleanup_test_context(ctx, writer);
    return 0;
}
