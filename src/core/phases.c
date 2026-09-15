#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "core/phases.h"
#include "core/context.h"
#include "config/sizes.h"
#include "test/test.h"
#include "data/generator.h"
#include "data/writers/writer.h"
#include "data/readers/reader.h"

GeneralContext* preparation_phase(void) {
    // Il contesto generale viene istanziato all'inizio.
    // L'input può essere allocato subito oppure lasciato a NULL e gestito nei benchmark.
    uint64_t real_size = (uint64_t)1 << MAX_PROBLEM_SIZE;
    Collection *input = collection_create(MAX_PROBLEM_SIZE);

    if (input) {
        DataGenerator *generator = generator_random_create(0.0, (double)real_size);
        if (generator) {
            generator_fill(generator, input);
            generator_destroy(generator);
        }
    }

    GeneralContext *gen_ctx = create_context(input);
    return gen_ctx;
}

int test_phase(GeneralContext *gen_ctx) {
    if (!gen_ctx || !gen_ctx->file_ctx || !gen_ctx->file_ctx->writer) {
        return 1;
    }

    Logger *logger = gen_ctx->logger;
    ResultWriter *writer = gen_ctx->file_ctx->writer;

    if (logger && logger->log) {
        logger->log("=== STARTING TEST PHASE (RAW DATA COLLECTION) ===");
    }

    size_t col_count = 0;
    const ColumnDesc *cols = obtain_raw_column_descs(&col_count);

    const Test *tests = get_test_set();
    size_t num_tests = get_test_count();

    for (size_t i = 0; i < num_tests; ++i) {
        // Genera il percorso estraendo il nome esatto del test (es. "memory_stress", "weak_scaling")
        // Risultato atteso: "output/<COMPILERNAME>/samples/<testname>.csv"
        char *output_filename = writer->operations.prepare_filepath(COMPILERNAME, tests[i].name);
        if (!output_filename) {
            if (logger && logger->error) {
                logger->error("Impossibile preparare il percorso per il file di output.");
            }
            continue;
        }

        // Prepara ed apre il file CSV specifico del test
        writer->operations.clean(writer, output_filename);
        if (!writer->operations.open(writer, output_filename, "w")) {
            if (logger && logger->error) {
                logger->error("Impossibile aprire il file di output per il test.");
            }
            free(output_filename);
            continue;
        }

        // Scrive l'intestazione delle colonne CSV
        writer->operations.write_header(writer, cols, col_count);

        // Esegue il benchmark passando il contesto generale
        int res = tests[i].run(gen_ctx);
        if (logger && logger->info) {
            logger->info(tests[i].name, res == 0);
        }
        writer->operations.flush(writer);

        // Chiude il file ed elimina la stringa allocata dinamicamente
        writer->operations.close(writer);
        free(output_filename);
    }

    if (logger && logger->log) {
        logger->log("=== TEST PHASE COMPLETED SUCCESSFULLY ===");
    }
    return 0;
}

int postprocess_phase(GeneralContext *gen_ctx, const char *raw_input_filename) {
    if (!gen_ctx || !gen_ctx->file_ctx || !gen_ctx->file_ctx->reader || !gen_ctx->file_ctx->writer) {
        return 1;
    }

    Logger *logger = gen_ctx->logger;
    ResultReader *reader = gen_ctx->file_ctx->reader;
    ResultWriter *writer = gen_ctx->file_ctx->writer;

    if (logger && logger->log) {
        logger->log("=== STARTING POSTPROCESS PHASE (AGGREGATION) ===");
    }

    if (!reader->operations.open(reader, raw_input_filename)) {
        if (logger && logger->error) logger->error("Failed to open raw CSV file for reading.");
        return 1;
    }

    char *output_filename = writer->operations.prepare_filepath(COMPILERNAME, "aggregated_results");
    if (!output_filename) {
        if (logger && logger->error) logger->error("Failed to prepare aggregated filepath.");
        reader->operations.close(reader);
        return 1;
    }

    writer->operations.clean(writer, output_filename);
    if (!writer->operations.open(writer, output_filename, "w")) {
        if (logger && logger->error) logger->error("Failed to open aggregated CSV file for writing.");
        free(output_filename);
        reader->operations.close(reader);
        return 1;
    }

    size_t agg_col_count = 0;
    const ColumnDesc *agg_cols = obtain_aggregated_column_descs(&agg_col_count);
    writer->operations.write_header(writer, agg_cols, agg_col_count);

    reader->operations.skip_header(reader);

    RawSample sample = {0};
    AggregatedResult current_agg = {0};

    double baseline_time = 0.0;
    size_t count = 0;
    double mean = 0.0, M2 = 0.0, min_t = 0.0, max_t = 0.0;

    while (reader->operations.read_raw(reader, &sample)) {
        bool new_group = (count > 0) && (
            sample.config.thread_number != current_agg.config.thread_number ||
            sample.config.log2n != current_agg.config.log2n ||
            sample.config.chunksize != current_agg.config.chunksize ||
            strcmp(sample.meta.benchname, current_agg.meta.benchname) != 0
        );

        if (new_group) {
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

            count = 0; mean = 0.0; M2 = 0.0;
            if (sample.meta.type) free((void*)sample.meta.type);
            if (sample.meta.benchname) free((void*)sample.meta.benchname);
        }

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
    free(output_filename);

    if (logger && logger->log) {
        logger->log("=== POSTPROCESS PHASE COMPLETED SUCCESSFULLY ===");
    }
    return 0;
}
