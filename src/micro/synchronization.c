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
    datatype *in;
    datatype *out;

    size_t size;
    size_t quarter;

    int threadnumber;

    if (ctx == NULL ||
        ctx->input == NULL ||
        ctx->output == NULL) {
        return;
    }

    in = ctx->input->data;
    out = ctx->output->data;

    size = ctx->input->size;
    quarter = size / 4;

    threadnumber = ctx->threadnumber;

    #pragma omp parallel \
        num_threads(threadnumber) \
        default(none) \
        shared(in, out, size, quarter)
    {
        #pragma omp sections
        {
            #pragma omp section
            {
                for (size_t i = 0; i < quarter; ++i)
                    out[i] = arithmetic_step(in[i]);
            }

            #pragma omp section
            {
                for (size_t i = quarter; i < 2 * quarter; ++i)
                    out[i] = arithmetic_step(in[i]);
            }

            #pragma omp section
            {
                for (size_t i = 2 * quarter; i < 3 * quarter; ++i)
                    out[i] = arithmetic_step(in[i]);
            }

            #pragma omp section
            {
                for (size_t i = 3 * quarter; i < size; ++i)
                    out[i] = arithmetic_step(in[i]);
            }
        }
    }
}

#endif


#if OPENMP_HAS_BARRIER

void routine_barrier(WorkContext *ctx)
{
    datatype *in;
    datatype *out;

    size_t size;
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

    #pragma omp parallel \
        num_threads(threadnumber) \
        default(none) \
        shared(in, out, size)
    {
        #pragma omp for nowait
        for (size_t i = 0; i < size; ++i)
            out[i] = arithmetic_step(in[i]);

        #pragma omp barrier

        #pragma omp for
        for (size_t i = 0; i < size; ++i)
            out[i] = arithmetic_step(out[i]);
    }
}

#endif


#if OPENMP_HAS_NOWAIT

void routine_nowait(WorkContext *ctx)
{
    datatype *in;
    datatype *out;

    size_t size;
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

    #pragma omp parallel \
        num_threads(threadnumber) \
        default(none) \
        shared(in, out, size)
    {
        #pragma omp for nowait
        for (size_t i = 0; i < size; ++i)
            out[i] = arithmetic_step(in[i]);

        #pragma omp for
        for (size_t i = 0; i < size; ++i)
            out[i] = arithmetic_step(in[i]);
    }
}

#endif


#if OPENMP_HAS_ORDERED

void routine_ordered(WorkContext *ctx)
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
        ordered \
        default(none) \
        shared(in, out, size, chunksize)

    for (size_t i = 0; i < size; ++i) {
        datatype val = arithmetic_step(in[i]);

        #pragma omp ordered
        out[i] = val;
    }
}

#endif


#if OPENMP_HAS_MASKED

void routine_masked(WorkContext *ctx)
{
    datatype *in;
    datatype *out;

    size_t size;
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

    #pragma omp parallel \
        num_threads(threadnumber) \
        default(none) \
        shared(in, out, size)
    {
        #pragma omp masked
        {
            for (size_t i = 0; i < size; ++i)
                out[i] = arithmetic_step(in[i]);
        }
    }
}

#endif
