#include <stdlib.h>
#include "data/writers/csv.h"

static unsigned short open_csv(ResultWriter *writer,
                               const char *filename,
                               const char *mode,
                               const char *header) {
    if (writer && filename && mode) {
        writer->file = fopen(filename, mode);
        if (writer->file) {
            if(header) {
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
    // 2. Apriamo in modalità "w" per troncarlo, controllando i puntatori
    FILE *tmp = fopen(filename, "w");
    if (!tmp) { return 0; } // Fallimento apertura (es. permessi)
    // 3. Chiudiamo il file temporaneo in sicurezza
    return fclose(tmp) == 0;
}


static unsigned short write_csv(ResultWriter *writer, const Result *record) {
    int result;
    if (writer && writer->file && record) {
        result = fprintf(writer->file,
                         "%d,%s,%ld,%d,%d,%f,%f,%f\n",
                         record->test_id,
                         record->benchname,
                         record->log2n,
                         record->threadnumber,
                         record->chunksize,
                         record->time,
                         record->speedup,
                         record->overhead);
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
    int result;
    if (writer && writer->file) {
        result = fclose(writer->file);
        writer->file = NULL;
        return result == 0;
    }
    return 0;
}

ResultWriter *create_csv_writer(void) {
    ResultWriter *writer;
    writer = malloc(sizeof *writer);
    if (!writer) { return NULL; }
    writer->file = NULL;
    writer->operations.open = open_csv;
    writer->operations.clean = clean_csv;
    writer->operations.write = write_csv;
    writer->operations.flush = flush_csv;
    writer->operations.close = close_csv;
    return writer;
}
