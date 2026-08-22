#include <stdlib.h>
#include "data/writer.h"

static unsigned short open_csv(Writer *writer,
                               const char *filename,
                               const char *mode) {
    if (writer && filename && mode) {
        writer->file = fopen(filename, mode);
        if (writer->file) { return 1; }
    }
    return 0;
}

static unsigned short write_csv(Writer *writer, Result *record)
{
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

static unsigned short flush_csv(Writer *writer) {
    if (writer && writer->file) {
        return fflush(writer->file) == 0;
    }
    return 0;
}

static unsigned short close_csv(Writer *writer)
{
    int result;

    if (writer && writer->file) {
        result = fclose(writer->file);
        writer->file = NULL;

        return result == 0;
    }

    return 0;
}

Writer *create_csv_writer(void)
{
    Writer *writer;

    writer = malloc(sizeof *writer);

    if (!writer)
        return NULL;

    writer->file = NULL;

    writer->operations.open = open_csv;
    writer->operations.write = write_csv;
    writer->operations.flush = flush_csv;
    writer->operations.close = close_csv;
    return writer;
}