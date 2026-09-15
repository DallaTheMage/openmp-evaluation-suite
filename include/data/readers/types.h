#ifndef READER_TYPES_H
#define READER_TYPES_H

#include <stdio.h>
#include <stddef.h>
#include "data/result.h"

typedef struct ResultReader ResultReader;
typedef struct ResultReaderOperations ResultReaderOperations;

struct ResultReaderOperations {
    // Apertura del file
    unsigned short (*open)(ResultReader *reader, const char *filename);

    // Salta la prima riga del CSV (l'header)
    unsigned short (*skip_header)(ResultReader *reader);

    // Lettura generica basata sui descrittori di colonna
    // Popola 'out_record' (che dev'essere già allocato dal chiamante)
    unsigned short (*read_record)(ResultReader *reader,
                                  void *out_record,
                                  const ColumnDesc *cols,
                                  size_t num_cols);

    // Metodi di comodo tipizzati per le tue struct principali
    // Restituiscono 1 (successo/record letto), 0 (EOF o errore)
    unsigned short (*read_raw)(ResultReader *reader, RawSample *out_sample);
    unsigned short (*read_aggregated)(ResultReader *reader, AggregatedResult *out_result);

    // Chiusura del file
    unsigned short (*close)(ResultReader *reader);
};

struct ResultReader {
    FILE                   *file;
    ResultReaderOperations operations;
};

#endif /* READER_TYPES_H */
