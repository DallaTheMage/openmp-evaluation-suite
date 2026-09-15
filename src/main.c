#include <stdio.h>
#include <stdlib.h>
#include "core/phases.h"
#include "core/context.h"
#include "core/logger.h"

int main(int argc, char *argv[]) {
    // Parametro opzionale per il path del file raw da leggere durante il postprocess
    const char *raw_file = (argc > 1) ? argv[1] : "raw_results.csv";

    // Phase 1: Preparation & Context Allocation
    // Inizializza l'intero grafo di esecuzione (WorkContext, FileContext, Logger, Reader/Writer)
    GeneralContext *gen_ctx = preparation_phase();
    if (!gen_ctx) {
        fprintf(stderr, "Fatal: Impossibile inizializzare il GeneralContext.\n");
        return EXIT_FAILURE;
    }

    // Phase 2: Test & Data Collection
    if (test_phase(gen_ctx) != 0) {
        if (gen_ctx->logger && gen_ctx->logger->error) {
            gen_ctx->logger->error("Execution aborted due to errors in Test Phase.");
        }
        destroy_context(gen_ctx);
        return EXIT_FAILURE;
    }

    // Phase 3: Post-Processing & Statistical Aggregation
    if (postprocess_phase(gen_ctx, raw_file) != 0) {
        if (gen_ctx->logger && gen_ctx->logger->error) {
            gen_ctx->logger->error("Execution aborted due to errors in Postprocess Phase.");
        }
        destroy_context(gen_ctx);
        return EXIT_FAILURE;
    }

    // Phase 4: Termination & Cleanup Centralizzato
    if (gen_ctx->logger && gen_ctx->logger->log) {
        gen_ctx->logger->log("All benchmark phases executed successfully.");
    }

    // Libera l'intero contesto (WorkContext, FileContext, Reader/Writer, Logger e gen_ctx)
    destroy_context(gen_ctx);

    return EXIT_SUCCESS;
}
