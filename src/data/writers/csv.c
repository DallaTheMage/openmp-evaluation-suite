#include "data/writers/csv.h"
#include "data/writers/utils.h"
#include <stdlib.h>

static unsigned short csv_open(ResultWriter *writer, const char *filename, const char *mode) {
    if (!writer || !filename || !mode) return 0;
    writer->file = fopen(filename, mode);
    return writer->file != NULL;
}

static unsigned short csv_write_header(ResultWriter *writer, const ColumnDesc *cols, size_t num_cols) {
    if (!writer || !writer->file) return 0;
    write_csv_header_from_desc(writer->file, cols, num_cols);
    return 1;
}

static unsigned short csv_write_record(ResultWriter *writer, const void *record, const ColumnDesc *cols, size_t num_cols) {
    if (!writer || !writer->file || !record) return 0;
    write_csv_row_from_desc(writer->file, record, cols, num_cols);
    return 1;
}

static unsigned short csv_write_raw(ResultWriter *writer, const RawSample *sample) {
    size_t num_cols = 0;
    const ColumnDesc *cols = obtain_raw_column_descs(&num_cols);
    return csv_write_record(writer, sample, cols, num_cols);
}

static unsigned short csv_write_aggregated(ResultWriter *writer, const AggregatedResult *result) {
    size_t num_cols = 0;
    const ColumnDesc *cols = obtain_aggregated_column_descs(&num_cols);
    return csv_write_record(writer, result, cols, num_cols);
}

static unsigned short csv_clean(ResultWriter *writer, const char *filename) {
    if (writer && writer->file) {
        fclose(writer->file);
        writer->file = NULL;
    }
    return remove(filename) == 0;
}

static unsigned short csv_flush(ResultWriter *writer) {
    if (!writer || !writer->file) return 0;
    return fflush(writer->file) == 0;
}

static unsigned short csv_close(ResultWriter *writer) {
    if (!writer || !writer->file) return 0;
    int res = fclose(writer->file);
    writer->file = NULL;
    return res == 0;
}

ResultWriter *create_csv_writer(void) {
    ResultWriter *writer = malloc(sizeof(ResultWriter));
    if (!writer) return NULL;

    writer->file = NULL;
    writer->operations.prepare_filepath = prepare_filepath;
    writer->operations.open             = csv_open;
    writer->operations.write_header     = csv_write_header;
    writer->operations.write_record     = csv_write_record;
    writer->operations.write_raw        = csv_write_raw;
    writer->operations.write_aggregated = csv_write_aggregated;
    writer->operations.clean            = csv_clean;
    writer->operations.flush            = csv_flush;
    writer->operations.close            = csv_close;

    return writer;
}

void destroy_csv_writer(ResultWriter *writer) {
    if (writer == NULL) return;

    // Chiude il file se è ancora aperto prima di deallocare
    if (writer->file != NULL) {
        fclose(writer->file);
        writer->file = NULL;
    }

    free(writer);
}
