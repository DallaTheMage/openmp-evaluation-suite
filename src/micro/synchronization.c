#include <stddef.h>

#include <omp.h>

#include "config/openmp.h"
#include "config/schedule.h"
#include "core/context.h"

#include "micro/arithmetic.h"
#include "micro/synchronization.h"

#if OPENMP_HAS_SECTIONS

void routine_sections(WorkContext *ctx)
{
    if (ctx == NULL || ctx->input == NULL) {
        return;
    }

    const double *in = ctx->input->data;
    size_t size = ctx->input->size;
    size_t quarter = size / 4;
    int threadnumber = ctx->threadnumber;

    #pragma omp parallel \
        num_threads(threadnumber) \
        default(none) \
        shared(in, size, quarter)
    {
        #pragma omp sections
        {
            #pragma omp section
            {
                for (size_t i = 0; i < quarter; ++i) {
                    volatile double dummy = arithmetic_step(in[i]);
                    (void)dummy;
                }
            }

            #pragma omp section
            {
                for (size_t i = quarter; i < 2 * quarter; ++i) {
                    volatile double dummy = arithmetic_step(in[i]);
                    (void)dummy;
                }
            }

            #pragma omp section
            {
                for (size_t i = 2 * quarter; i < 3 * quarter; ++i) {
                    volatile double dummy = arithmetic_step(in[i]);
                    (void)dummy;
                }
            }

            #pragma omp section
            {
                for (size_t i = 3 * quarter; i < size; ++i) {
                    volatile double dummy = arithmetic_step(in[i]);
                    (void)dummy;
                }
            }
        }
    }
}

#endif

#if OPENMP_HAS_BARRIER

void routine_barrier(WorkContext *ctx)
{
    if (ctx == NULL || ctx->input == NULL) {
        return;
    }

    const double *in = ctx->input->data;
    size_t size = ctx->input->size;
    int threadnumber = ctx->threadnumber;

    #pragma omp parallel \
        num_threads(threadnumber) \
        default(none) \
        shared(in, size)
    {
        #pragma omp for nowait
        for (size_t i = 0; i < size; ++i) {
            volatile double dummy = arithmetic_step(in[i]);
            (void)dummy;
        }

        #pragma omp barrier

        #pragma omp for
        for (size_t i = 0; i < size; ++i) {
            volatile double dummy = arithmetic_step(in[i]);
            (void)dummy;
        }
    }
}

#endif

#if OPENMP_HAS_NOWAIT

void routine_nowait(WorkContext *ctx)
{
    if (ctx == NULL || ctx->input == NULL) {
        return;
    }

    const double *in = ctx->input->data;
    size_t size = ctx->input->size;
    int threadnumber = ctx->threadnumber;

    #pragma omp parallel \
        num_threads(threadnumber) \
        default(none) \
        shared(in, size)
    {
        #pragma omp for nowait
        for (size_t i = 0; i < size; ++i) {
            volatile double dummy = arithmetic_step(in[i]);
            (void)dummy;
        }

        #pragma omp for
        for (size_t i = 0; i < size; ++i) {
            volatile double dummy = arithmetic_step(in[i]);
            (void)dummy;
        }
    }
}

#endif

#if OPENMP_HAS_ORDERED

void routine_ordered(WorkContext *ctx)
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
        ordered \
        default(none) \
        shared(in, size, chunksize)

    for (size_t i = 0; i < size; ++i) {
        double val = arithmetic_step(in[i]);

        #pragma omp ordered
        {
            volatile double dummy = val;
            (void)dummy;
        }
    }
}

#endif

#if OPENMP_HAS_MASKED

void routine_masked(WorkContext *ctx)
{
    if (ctx == NULL || ctx->input == NULL) {
        return;
    }

    const double *in = ctx->input->data;
    size_t size = ctx->input->size;
    int threadnumber = ctx->threadnumber;

    #pragma omp parallel \
        num_threads(threadnumber) \
        default(none) \
        shared(in, size)
    {
        #pragma omp masked
        {
            for (size_t i = 0; i < size; ++i) {
                volatile double dummy = arithmetic_step(in[i]);
                (void)dummy;
            }
        }
    }
}

#endif