#ifndef WRITER_TYPES_H
#define WRITER_TYPES_H

#include <stdio.h>
#include <stddef.h>
#include "data/result.h"

typedef struct ResultWriter ResultWriter;
typedef struct ResultWriterOperations ResultWriterOperations;

struct ResultWriterOperations {
    char *(*prepare_filepath)(const char *compiler_name,
                              const char *test_name);
    unsigned short (*open)(ResultWriter *writer,
                           const char *filename,
                           const char *mode);

    // Scrive l'header partendo dai descrittori delle colonne
    unsigned short (*write_header)(ResultWriter *writer,
                                  const ColumnDesc *cols,
                                  size_t num_cols);

    // Scrive una riga generica sfruttando i descrittori e offsetof
    unsigned short (*write_record)(ResultWriter *writer,
                                   const void *record,
                                   const ColumnDesc *cols,
                                   size_t num_cols);

    // Metodi di comodo specifici (wrapper sui generici)
    unsigned short (*write_raw)(ResultWriter *writer, const RawSample *sample);
    unsigned short (*write_aggregated)(ResultWriter *writer, const AggregatedResult *result);

    unsigned short (*clean)(ResultWriter *writer, const char *filename);
    unsigned short (*flush)(ResultWriter *writer);
    unsigned short (*close)(ResultWriter *writer);
};

struct ResultWriter {
    FILE                   *file;
    ResultWriterOperations operations;
};

#endif /* WRITER_TYPES_H */
