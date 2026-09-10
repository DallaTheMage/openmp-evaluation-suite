#include <stdio.h>
#include <stdlib.h>
#include "data/writers/utils.h"
#include "data/writers/csv.h"

static char *add_csv_extension(const char *filename) {
    return prepare_filepath(filename, ".csv");
}

static unsigned short open_csv(ResultWriter *writer,
                               const char *filename,
                               const char *mode,
                               const char *header) {
    if (writer && filename && mode) {
        writer->file = fopen(filename, mode);
        if (writer->file) {
            if (header) {
                fprintf(writer->file, "%s\n", header);
                fflush(writer->file);
            }
            return 1;
        }
    }
    return 0;
}

static unsigned short clean_csv(ResultWriter *writer, const char *filename) {
    if (!writer || !filename) { return 0; }

    // 1. Se il file era già aperto internamente, lo chiudiamo in sicurezza
    if (writer->file) {
        fclose(writer->file);
        writer->file = NULL;
    }

    // 2. Apriamo in modalità "w" per troncarlo
    FILE *tmp = fopen(filename, "w");
    if (!tmp) { return 0; }

    // 3. Chiudiamo il file temporaneo
    return fclose(tmp) == 0;
}

static unsigned short write_csv(ResultWriter *writer, const TestResult *record) {
    if (writer && writer->file && record) {
        /*
         * Tracciamo tutti i campi della nuova struct TestResult:
         * Meta, Config, Time (mean, min, max, variance) e ParallelMetrics.
         * Formato IEEE 754 con %.9g per preservare la massima precisione dei tempi.
         */
        int result = fprintf(
            writer->file,
            "%d,%s,%s,%ld,%d,%d,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g\n",
            record->meta.id,
            record->meta.type ? record->meta.type : "",
            record->meta.benchname ? record->meta.benchname : "",
            record->config.log2n,
            record->config.thread_number,
            record->config.chunksize,
            record->time.mean,
            record->time.min,
            record->time.max,
            record->time.variance,
            record->metrics.speedup,
            record->metrics.efficiency,
            record->metrics.overhead
        );
        return result >= 0;
    }
    return 0;
}

static unsigned short flush_csv(ResultWriter *writer) {
    if (writer && writer->file) {
        return fflush(writer->file) == 0;
    }
    return 0;
}

static unsigned short close_csv(ResultWriter *writer) {
    if (writer && writer->file) {
        int result = fclose(writer->file);
        writer->file = NULL;
        return result == 0;
    }
    return 0;
}

ResultWriter *create_csv_writer(void) {
    ResultWriter *writer = malloc(sizeof *writer);
    if (!writer) { return NULL; }

    writer->file = NULL;
    writer->operations.prepare_filepath = add_csv_extension;
    writer->operations.open = open_csv;
    writer->operations.clean = clean_csv;
    writer->operations.write = write_csv;
    writer->operations.flush = flush_csv;
    writer->operations.close = close_csv;

    return writer;
}