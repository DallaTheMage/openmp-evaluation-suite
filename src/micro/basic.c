#include <stddef.h>

#include <omp.h>

#include "config/schedule.h"
#include "core/context.h"

#include "micro/arithmetic.h"
#include "micro/basic.h"

void routine_micro_scale(WorkContext *ctx)
{
    if (ctx == NULL || ctx->input == NULL) {
        return;
    }

    const double *in = ctx->input->data;
    size_t size = ctx->input->size;
    int threadnumber = ctx->threadnumber;
    int chunksize = ctx->chunksize;

    #pragma omp parallel for \
        num_threads(threadnumber) \
        schedule(CHOSEN_SCHEDULE, chunksize) \
        default(none) \
        shared(in, size, chunksize)
    for (size_t i = 0; i < size; ++i) {
        volatile double dummy = arithmetic_step(in[i]);
        (void)dummy;
    }
}

void routine_for(WorkContext *ctx)
{
    if (ctx == NULL || ctx->input == NULL) {
        return;
    }

    const double *in = ctx->input->data;
    size_t size = ctx->input->size;
    int threadnumber = ctx->threadnumber;
    int chunksize = ctx->chunksize;

    #pragma omp parallel \
        num_threads(threadnumber) \
        default(none) \
        shared(in, size, chunksize)
    {
        #pragma omp for \
            schedule(CHOSEN_SCHEDULE, chunksize)
        for (size_t i = 0; i < size; ++i) {
            volatile double dummy = arithmetic_step(in[i]);
            (void)dummy;
        }
    }
}