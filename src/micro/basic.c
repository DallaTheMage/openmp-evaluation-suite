#include <stddef.h>

#include <omp.h>

#include "config/schedule.h"
#include "core/context.h"

#include "micro/arithmetic.h"
#include "micro/basic.h"


void routine_micro_scale(WorkContext *ctx)
{
    datatype *in;
    datatype *out;

    size_t size;
    int threadnumber;
    int chunksize;

    if (ctx == NULL ||
        ctx->input == NULL ||
        ctx->output == NULL) {
        return;
    }

    in = ctx->input->data;
    out = ctx->output->data;
    size = ctx->input->size;

    threadnumber = ctx->threadnumber;
    chunksize = ctx->chunksize;

    #pragma omp parallel for \
        num_threads(threadnumber) \
        schedule(CHOSEN_SCHEDULE, chunksize) \
        default(none) \
        shared(in, out, size, chunksize)

    for (size_t i = 0; i < size; ++i) {
        datatype val = in[i];

        for (int k = 0; k < 8; ++k)
            val = arithmetic_step(val);

        out[i] = val;
    }
}


void routine_for(WorkContext *ctx)
{
    datatype *in;
    datatype *out;

    size_t size;
    int threadnumber;
    int chunksize;

    if (ctx == NULL ||
        ctx->input == NULL ||
        ctx->output == NULL) {
        return;
    }

    in = ctx->input->data;
    out = ctx->output->data;
    size = ctx->input->size;

    threadnumber = ctx->threadnumber;
    chunksize = ctx->chunksize;

    #pragma omp parallel \
        num_threads(threadnumber) \
        default(none) \
        shared(in, out, size, chunksize)
    {
        #pragma omp for \
            schedule(CHOSEN_SCHEDULE, chunksize)
        for (size_t i = 0; i < size; ++i) {
            datatype val = in[i];

            for (int k = 0; k < 8; ++k)
                val = arithmetic_step(val);

            out[i] = val;
        }
    }
}
