#ifndef WRITER_H
#define WRITER_H

#include <stdio.h>
#include "data/result.h"

typedef struct Writer Writer;

typedef struct {
    unsigned short (*open)(Writer *writer,
                           const char *filename,
                           const char *mode);
    unsigned short (*write)(Writer *writer,
                            Result *record);
    unsigned short (*flush)(Writer *writer);
    unsigned short (*close)(Writer *writer);
} WriterOperations;

struct Writer {
    FILE *file;
    WriterOperations operations;
};

Writer *create_csv_writer(void);

#endif /* WRITER_H */