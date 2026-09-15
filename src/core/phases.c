#include "core/phases.h"
#include "test/test.h"
#include "data/writers/writer.h"
#include "data/readers/reader.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

int run_test_phase(const char *raw_output_filename, Logger *logger) {
    logger->log("=== STARTING TEST PHASE (RAW DATA COLLECTION) ===");

    ResultWriter *writer = create_writer();
    WorkContext *ctx = malloc(sizeof(WorkContext));
    if (!writer || !ctx) {
        logger->error("Failed to allocate test context or writer.");
        cleanup_test_context(ctx, writer, NULL);
        return 1;
    }

    size_t col_count = 0;
    const ColumnDesc *cols = obtain_raw_column_descs(&col_count);

    writer->operations.clean(writer, raw_output_filename);
    if (!writer->operations.open(writer, raw_output_filename, "w")) {
        logger->error("Failed to open raw output file.");
        cleanup_test_context(ctx, writer, NULL);
        return 1;
    }

    writer->operations.write_header(writer, cols, col_count);

    const Test *tests = get_test_set();
    size_t num_tests = get_test_count();

    for (size_t i = 0; i < num_tests; ++i) {
        logger->info(tests[i].testname, tests[i].run(writer, ctx, logger) == 0);
        writer->operations.flush(writer);
    }

    cleanup_test_context(ctx, writer, NULL);
    logger->log("=== TEST PHASE COMPLETED SUCCESSFULLY ===");
    return 0;
}

int run_postprocess_phase(const char *raw_input_filename, const char *aggregated_output_filename, Logger *logger) {
    logger->log("=== STARTING POSTPROCESS PHASE (AGGREGATION) ===");

    ResultReader *reader = create_reader();
    ResultWriter *writer = create_writer();

    if (!reader->operations.open(reader, raw_input_filename)) {
        logger->error("Failed to open raw CSV file for reading.");
        free(reader); free(writer);
        return 1;
    }

    writer->operations.clean(writer, aggregated_output_filename);
    if (!writer->operations.open(writer, aggregated_output_filename, "w")) {
        logger->error("Failed to open aggregated CSV file for writing.");
        reader->operations.close(reader);
        free(reader); free(writer);
        return 1;
    }

    // Scrive l'header del file aggregato
    size_t agg_col_count = 0;
    const ColumnDesc *agg_cols = obtain_aggregated_column_descs(&agg_col_count);
    writer->operations.write_header(writer, agg_cols, agg_col_count);

    reader->operations.skip_header(reader);

    RawSample sample;
    AggregatedResult current_agg = {0};

    double baseline_time = 0.0;
    size_t count = 0;
    double mean = 0.0, M2 = 0.0, min_t = 0.0, max_t = 0.0;

    while (reader->operations.read_raw(reader, &sample)) {
        // Rilevamento cambio di configurazione (gruppo di campioni)
        bool new_group = (count > 0) && (
            sample.config.thread_number != current_agg.config.thread_number ||
            sample.config.log2n != current_agg.config.log2n ||
            sample.config.chunksize != current_agg.config.chunksize ||
            strcmp(sample.meta.benchname, current_agg.meta.benchname) != 0
        );

        if (new_group) {
            // Finalizza il gruppo precedente
            current_agg.time.mean = mean;
            current_agg.time.min = min_t;
            current_agg.time.max = max_t;
            current_agg.time.variance = (count > 1) ? (M2 / (count - 1)) : 0.0;

            if (current_agg.config.thread_number == 1) {
                baseline_time = current_agg.time.mean;
                current_agg.metrics.speedup = 1.0;
                current_agg.metrics.efficiency = 1.0;
                current_agg.metrics.overhead = 0.0;
            } else {
                double threads = (double)current_agg.config.thread_number;
                current_agg.metrics.speedup = (baseline_time > 0 && mean > 0) ? (baseline_time / mean) : 0.0;
                current_agg.metrics.efficiency = current_agg.metrics.speedup / threads;
                current_agg.metrics.overhead = mean - (baseline_time / threads);
            }

            writer->operations.write_aggregated(writer, &current_agg);

            // Reset accumulatore
            count = 0; mean = 0.0; M2 = 0.0;
            if (sample.meta.type) free((void*)sample.meta.type);
            if (sample.meta.benchname) free((void*)sample.meta.benchname);
        }

        // Accumulo campioni (Welford)
        if (count == 0) {
            current_agg.meta = sample.meta;
            current_agg.config = sample.config;
            min_t = sample.elapsed_time;
            max_t = sample.elapsed_time;
        } else {
            if (sample.elapsed_time < min_t) min_t = sample.elapsed_time;
            if (sample.elapsed_time > max_t) max_t = sample.elapsed_time;
        }

        count++;
        double delta = sample.elapsed_time - mean;
        mean += delta / count;
        double delta2 = sample.elapsed_time - mean;
        M2 += delta * delta2;
    }

    // Scrittura dell'ultimo gruppo
    if (count > 0) {
        current_agg.time.mean = mean;
        current_agg.time.min = min_t;
        current_agg.time.max = max_t;
        current_agg.time.variance = (count > 1) ? (M2 / (count - 1)) : 0.0;

        if (current_agg.config.thread_number == 1) {
            current_agg.metrics.speedup = 1.0;
            current_agg.metrics.efficiency = 1.0;
            current_agg.metrics.overhead = 0.0;
        } else {
            double threads = (double)current_agg.config.thread_number;
            current_agg.metrics.speedup = (baseline_time > 0 && mean > 0) ? (baseline_time / mean) : 0.0;
            current_agg.metrics.efficiency = current_agg.metrics.speedup / threads;
            current_agg.metrics.overhead = mean - (baseline_time / threads);
        }

        writer->operations.write_aggregated(writer, &current_agg);
        if (sample.meta.type) free((void*)sample.meta.type);
        if (sample.meta.benchname) free((void*)sample.meta.benchname);
    }

    reader->operations.close(reader);
    writer->operations.close(writer);
    free(reader); free(writer);

    logger->log("=== POSTPROCESS PHASE COMPLETED SUCCESSFULLY ===");
    return 0;
}
