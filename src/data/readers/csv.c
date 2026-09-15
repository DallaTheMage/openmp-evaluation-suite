#include "data/readers/csv.h"
#include "data/readers/utils.h"
#include <stdlib.h>

static unsigned short csv_read_open(ResultReader *reader, const char *filename) {
    if (!reader || !filename) return 0;
    reader->file = fopen(filename, "r");
    return reader->file != NULL;
}

static unsigned short csv_skip_header(ResultReader *reader) {
    if (!reader || !reader->file) return 0;
    char buffer[1024];
    return fgets(buffer, sizeof(buffer), reader->file) != NULL;
}

static unsigned short csv_read_record(ResultReader *reader, void *out_record, const ColumnDesc *cols, size_t num_cols) {
    if (!reader || !reader->file || !out_record) return 0;
    return parse_csv_line(reader->file, out_record, cols, num_cols);
}

static unsigned short csv_read_raw(ResultReader *reader, RawSample *out_sample) {
    size_t num_cols = 0;
    const ColumnDesc *cols = obtain_raw_column_descs(&num_cols);
    return csv_read_record(reader, out_sample, cols, num_cols);
}

static unsigned short csv_read_aggregated(ResultReader *reader, AggregatedResult *out_result) {
    size_t num_cols = 0;
    const ColumnDesc *cols = obtain_aggregated_column_descs(&num_cols);
    return csv_read_record(reader, out_result, cols, num_cols);
}

static unsigned short csv_read_close(ResultReader *reader) {
    if (!reader || !reader->file) return 0;
    int res = fclose(reader->file);
    reader->file = NULL;
    return res == 0;
}

ResultReader *create_csv_reader(void) {
    ResultReader *reader = malloc(sizeof(ResultReader));
    if (!reader) return NULL;

    reader->file = NULL;
    reader->operations.open            = csv_read_open;
    reader->operations.skip_header     = csv_skip_header;
    reader->operations.read_record     = csv_read_record;
    reader->operations.read_raw        = csv_read_raw;
    reader->operations.read_aggregated = csv_read_aggregated;
    reader->operations.close           = csv_read_close;

    return reader;
}

void destroy_csv_reader(ResultReader *reader) {
    if (reader == NULL) return;

    if (reader->file != NULL) {
        fclose(reader->file);
        reader->file = NULL;
    }

    free(reader);
}
