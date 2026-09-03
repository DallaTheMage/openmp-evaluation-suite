#ifndef WRITER_TYPES_H
#define WRITER_TYPES_H
    #include <stdio.h>
    #include "data/result.h"

    typedef struct ResultWriter ResultWriter;
    typedef struct ResultWriterOperations ResultWriterOperations;

    struct ResultWriterOperations {
        unsigned short (*open)(ResultWriter *writer,
                               const char *filename,
                               const char *mode,
                               const char *header);
        unsigned short (*clean)(ResultWriter *writer,
                                const char *filename);
        unsigned short (*write)(ResultWriter *writer,
                                const Result *record);
        unsigned short (*flush)(ResultWriter *writer);
        unsigned short (*close)(ResultWriter *writer);
    };

    struct ResultWriter {
        FILE                   *file;
        ResultWriterOperations operations;
    };
#endif
