#include <stdio.h>
#include <stdlib.h>
#include "test/test.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <output-file>\n", argv[0]);
        return 1;
    }

    ResultWriter *writer;
    WorkContext *ctx;
    const char *header = "Test ID, Test type, Test name, Problem size (log2n), Thread number, Chunk size, AVG time, Speedup, Overhead, Efficiency";
    const Test *tests = get_test_set();
    const size_t num_tests = get_test_count();
    writer = create_writer();
    ctx = (WorkContext *)malloc(sizeof(WorkContext));
    char *output_filename;
    for (size_t i = 0; i < num_tests; ++i) {
        output_filename = writer->operations.prepare_filepath(tests[i].testname);
        if (writer->operations.clean(writer, output_filename) != 1) {
            fprintf(stderr, "Error cleaning file: %s\n", output_filename);
            cleanup_test_context(ctx, writer);
            return 1;
        }
        if (writer->operations.open(writer, output_filename, "a", header) != 1) {
            fprintf(stderr, "Error opening file for writing: %s\n", output_filename);
            cleanup_test_context(ctx, writer);
            return 1;
        }
        printf("=== Starting %s ===\n", tests[i].testname);
        if (tests[i].run(writer, ctx) != 0) {
            fprintf(stderr, "Error during %s execution.\n", tests[i].testname);
            return 1;
        }
        writer->operations.flush(writer);
        writer->operations.close(writer);
        printf("=== Finished %s ===\n", tests[i].testname);
    }
    printf("\nAll benchmarks completed successfully.");
    printf("\nCleaning up resources...\n");
    cleanup_test_context(ctx, writer);
    return 0;
}
