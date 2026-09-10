#include <stddef.h>

#include "config/openmp.h"
#include "config/schedule.h"
#include "core/context.h"
#include "micro/microroutines.h"
#include "micro/simd.h"

static double arithmetic_step(double value)
{
#if DATATYPE_IS_FLOATING
    return value * (double)1.000001 + (double)0.000001;
#else
    return value * (double)2 + (double)1;
#endif
}

void routine_simd(WorkContext *ctx)
{
#if OPENMP_HAS_SIMD
    if (ctx == NULL || ctx->input == NULL) {
        return;
    }

    const double *in = ctx->input->data;
    size_t size = ctx->input->size;

    #pragma omp simd
    for (size_t i = 0; i < size; ++i) {
        double val = in[i];

        for (int k = 0; k < 8; ++k) {
            val = arithmetic_step(val);
        }

        volatile double dummy = val;
        (void)dummy;
    }
#else
    (void)ctx;
#endif
}

void routine_parallel_for_simd(WorkContext *ctx)
{
#if OPENMP_HAS_SIMD
    if (ctx == NULL || ctx->input == NULL) {
        return;
    }

    const double *in = ctx->input->data;
    size_t size = ctx->input->size;
    int threadnumber = ctx->threadnumber;
    int chunksize = ctx->chunksize;

    #pragma omp parallel for simd \
        num_threads(threadnumber) \
        schedule(CHOSEN_SCHEDULE, chunksize) \
        default(none) \
        shared(in, size, chunksize)
    for (size_t i = 0; i < size; ++i) {
        double val = in[i];

        for (int k = 0; k < 8; ++k) {
            val = arithmetic_step(val);
        }

        volatile double dummy = val;
        (void)dummy;
    }
#else
    (void)ctx;
#endif
}

void routine_loop(WorkContext *ctx)
{
#if OPENMP_HAS_LOOP
    if (ctx == NULL || ctx->input == NULL) {
        return;
    }

    const double *in = ctx->input->data;
    size_t size = ctx->input->size;
    int threadnumber = ctx->threadnumber;

    #pragma omp parallel num_threads(threadnumber) default(none) shared(in, size)
    {
        #pragma omp loop
        for (size_t i = 0; i < size; ++i) {
            double val = in[i];

            for (int k = 0; k < 8; ++k) {
                val = arithmetic_step(val);
            }

            volatile double dummy = val;
            (void)dummy;
        }
    }
#else
    (void)ctx;
#endif
}