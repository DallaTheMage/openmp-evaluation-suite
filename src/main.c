#include <stdio.h>
#include <stdlib.h>
#include "core/phases.h"
#include "core/logger.h"

int main(int argc, char *argv[]) {
    const char *raw_file = (argc > 1) ? argv[1] : "raw_results.csv";
    const char *agg_file = (argc > 2) ? argv[2] : "aggregated_results.csv";

    Logger *logger = create_logger();
    if (!logger) return 1;

    // Phase 1: Test & Data Collection
    if (run_test_phase(raw_file, logger) != 0) {
        logger->error("Execution aborted due to errors in Test Phase.");
        free(logger);
        return 1;
    }

    // Phase 2: Post-Processing & Statistical Aggregation
    if (run_postprocess_phase(raw_file, agg_file, logger) != 0) {
        logger->error("Execution aborted due to errors in Postprocess Phase.");
        free(logger);
        return 1;
    }

    // Phase 3: Cleanup / End
    logger->log("All benchmark phases executed successfully.");
    free(logger);
    return 0;
}
