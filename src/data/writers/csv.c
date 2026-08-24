#include <stdlib.h>
#include "data/writers/csv.h"

static unsigned short open_csv(ResultWriter *writer,
                               const char *filename,
                               const char *mode) {
    if (writer && filename && mode) {
        writer->file = fopen(filename, mode);
        if (writer->file) { return 1; }
    }
    return 0;
}

static unsigned short write_csv(ResultWriter *writer, const Result *record) {
    int result;
    if (writer && writer->file && record) {
        result = fprintf(writer->file,
                         "%s,%ld,%d,%d,%f\n",
                         record->benchname,
                         record->log2n,
                         record->threadnumber,
                         record->chunksize,
                         record->time);
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
    writer->operations.write = write_csv;
    writer->operations.flush = flush_csv;
    writer->operations.close = close_csv;
    return writer;
}