#ifndef WRITER_UTILS_H
#define WRITER_UTILS_H

#include <stdio.h>
#include <stddef.h>
#include "data/result.h"

char *prepare_filepath(const char *testname, const char *extension);

// Utility generica per scrivere header e dati usando ColumnDesc
void write_csv_header_from_desc(FILE *fp, const ColumnDesc *cols, size_t num_cols);
void write_csv_row_from_desc(FILE *fp, const void *struct_ptr, const ColumnDesc *cols, size_t num_cols);

#endif /* WRITER_UTILS_H */
