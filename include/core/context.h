#ifndef CONTEXT_H
#define CONTEXT_H

#include "data/collection.h"
#include "core/logger.h"
#include "data/writers/writer.h"
#include "data/readers/reader.h"

typedef struct WorkContext {
    Collection *input;
    int chunksize;
    int threadnumber;
    size_t warmup_iterations;
    size_t work_iterations;
} WorkContext;

typedef struct FileContext {
    char         *sample_path;
    char         *postprocess_path;
    ResultWriter *writer;
    ResultReader *reader;
} FileContext;

typedef struct GeneralContext {
    WorkContext  *work_ctx;
    FileContext  *file_ctx;
    Logger       *logger;
} GeneralContext;

GeneralContext* create_context(Collection *input);
void destroy_context(GeneralContext *ctx);
#endif /* CONTEXT_H */
