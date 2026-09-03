#include <stddef.h>

#include "config/openmp.h"
#include "config/schedule.h"
#include "core/context.h"
#include "micro/microroutines.h"


static datatype arithmetic_step(datatype value)
{
#if DATATYPE_IS_FLOATING

    return value
         * (datatype)1.000001
         + (datatype)0.000001;

#else

    return value
         * (datatype)2
         + (datatype)1;

#endif
}


void routine_simd(WorkContext *ctx)
{
#if OPENMP_HAS_SIMD

    datatype *in;
    datatype *out;

    size_t size;
    size_t i;

    if (ctx == NULL ||
        ctx->input == NULL ||
        ctx->output == NULL) {
        return;
    }

    in = ctx->input->data;
    out = ctx->output->data;
    size = ctx->input->size;

    #pragma omp simd
    for (i = 0; i < size; ++i) {
        datatype val;
        int k;

        val = in[i];

        for (k = 0; k < 8; ++k) {
            val = arithmetic_step(val);
        }

        out[i] = val;
    }

#else

    (void)ctx;

#endif
}


void routine_parallel_for_simd(WorkContext *ctx)
{
#if OPENMP_HAS_SIMD

    datatype *in;
    datatype *out;

    size_t size;
    size_t i;

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

    #pragma omp parallel for simd \
        num_threads(threadnumber) \
        schedule(CHOSEN_SCHEDULE, chunksize) \
        default(none) \
        shared(in, out, size, chunksize)

    for (i = 0; i < size; ++i) {
        datatype val;
        int k;

        val = in[i];

        for (k = 0; k < 8; ++k) {
            val = arithmetic_step(val);
        }

        out[i] = val;
    }

#else

    (void)ctx;

#endif
}
