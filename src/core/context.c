#include <stdlib.h>
#include "core/context.h"
#include "core/logger.h"
#include "config/sizes.h"
#include "test/test.h"

static FileContext* create_file_context(void) {
    FileContext *ctx = (FileContext *)malloc(sizeof(FileContext));
    if (!ctx) {
        return NULL;
    }

    ctx->sample_path = NULL;
    ctx->postprocess_path = NULL;

    ctx->writer = create_writer();
    ctx->reader = create_reader();

    if (!ctx->writer || !ctx->reader) {
        if (ctx->writer) destroy_writer(ctx->writer);
        if (ctx->reader) destroy_reader(ctx->reader);
        free(ctx);
        return NULL;
    }

    return ctx;
}

static void destroy_file_context(FileContext *ctx) {
    if (!ctx) return;

    if (ctx->writer) destroy_writer(ctx->writer);
    if (ctx->reader) destroy_reader(ctx->reader);

    if (ctx->sample_path) free(ctx->sample_path);
    if (ctx->postprocess_path) free(ctx->postprocess_path);

    free(ctx);
}

GeneralContext* create_context(Collection *input) {
    GeneralContext *ctx = (GeneralContext *)malloc(sizeof(GeneralContext));
    if (!ctx) {
        return NULL;
    }

    ctx->file_ctx = NULL;
    ctx->work_ctx = NULL;
    ctx->logger = NULL;

    ctx->file_ctx = create_file_context();
    ctx->work_ctx = (WorkContext *)malloc(sizeof(WorkContext));
    ctx->logger = create_logger(); // Inizializza correttamente i puntatori a funzione

    // Verifica la corretta allocazione delle sotto-strutture
    if (!ctx->file_ctx || !ctx->work_ctx || !ctx->logger) {
        if (ctx->file_ctx) destroy_file_context(ctx->file_ctx);
        if (ctx->work_ctx) free(ctx->work_ctx);
        if (ctx->logger) destroy_logger(ctx->logger);
        free(ctx);
        return NULL;
    }

    // Inizializzazione sicura di WorkContext
    ctx->work_ctx->input = input;
    ctx->work_ctx->chunksize = 0;
    ctx->work_ctx->threadnumber = 0;
    ctx->work_ctx->warmup_iterations = 0;
    ctx->work_ctx->work_iterations = 0;

    return ctx;
}

void destroy_context(GeneralContext *ctx) {
    if (!ctx) return;

    if (ctx->file_ctx) {
        destroy_file_context(ctx->file_ctx);
    }

    if (ctx->work_ctx) {
        destroy_collections(ctx->work_ctx);
        free(ctx->work_ctx);
    }

    if (ctx->logger) {
        destroy_logger(ctx->logger);
    }

    free(ctx);
}
