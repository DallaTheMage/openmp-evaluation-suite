#include <stddef.h>

#include "config/openmp.h"
#include "config/schedule.h"
#include "core/context.h"
#include "micro/microroutines.h"


static double arithmetic_step(double value)
{
#if DATATYPE_IS_FLOATING

    return value
         * (double)1.000001
         + (double)0.000001;

#else

    return value
         * (double)2
         + (double)1;

#endif
}


void routine_simd(WorkContext *ctx)
{
#if OPENMP_HAS_SIMD

    double *in;
    double *out;

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
        double val;
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

    double *in;
    double *out;

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
        double val;
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

void routine_loop(WorkContext *ctx)
{
#if OPENMP_HAS_LOOP

    double *in;
    double *out;

    size_t size;
    size_t i;

    int threadnumber;

    if (ctx == NULL ||
        ctx->input == NULL ||
        ctx->output == NULL) {
        return;
    }

    in = ctx->input->data;
    out = ctx->output->data;
    size = ctx->input->size;
    threadnumber = ctx->threadnumber;

    /* Utilizza la direttiva 'loop' introdotta in OpenMP 5.0 */
    #pragma omp parallel num_threads(threadnumber) default(none) shared(in, out, size)
    {
        #pragma omp loop
        for (i = 0; i < size; ++i) {
            double val = in[i];
            int k;

            for (k = 0; k < 8; ++k) {
                val = arithmetic_step(val);
            }

            out[i] = val;
        }
    }

#else

    (void)ctx;

#endif
}
