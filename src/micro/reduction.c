#include <stddef.h>

#include <omp.h>

#include "config/openmp.h"
#include "config/schedule.h"
#include "core/context.h"

#include "micro/reduction.h"


void routine_reduction_sum(WorkContext *ctx)
{
    datatype *in;
    datatype *out;

    size_t size;
    int threadnumber;
    int chunksize;

    datatype sum = (datatype)0;

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
        shared(in, size, chunksize) \
        reduction(+:sum)

    for (size_t i = 0; i < size; ++i)
        sum += in[i];

    out[0] = sum;
}


#if OPENMP_HAS_SCAN

void routine_scan_inclusive(WorkContext *ctx)
{
    datatype *in;
    datatype *out;

    size_t size;
    int threadnumber;

    datatype running_sum = (datatype)0;

    if (ctx == NULL ||
        ctx->input == NULL ||
        ctx->output == NULL) {
        return;
    }

    in = ctx->input->data;
    out = ctx->output->data;
    size = ctx->input->size;

    threadnumber = ctx->threadnumber;

    #pragma omp parallel for \
        num_threads(threadnumber) \
        default(none) \
        shared(in, out, size) \
        reduction(inscan, +:running_sum)

    for (size_t i = 0; i < size; ++i) {
        running_sum += in[i];

        #pragma omp scan inclusive(running_sum)

        out[i] = running_sum;
    }
}

#endif


#if OPENMP_HAS_CRITICAL

void routine_critical_reduction(WorkContext *ctx)
{
    datatype *in;
    datatype *out;

    size_t size;
    int threadnumber;
    int chunksize;

    datatype global_sum = (datatype)0;

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
        shared(in, size, global_sum)

    for (size_t i = 0; i < size; ++i) {
        #pragma omp critical
        global_sum += in[i];
    }

    out[0] = global_sum;
}

#endif


#if OPENMP_HAS_ATOMIC

void routine_atomic_reduction(WorkContext *ctx)
{
    datatype *in;
    datatype *out;

    size_t size;
    int threadnumber;
    int chunksize;

    datatype global_sum = (datatype)0;

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
        shared(in, size, global_sum)

    for (size_t i = 0; i < size; ++i) {
        #pragma omp atomic
        global_sum += in[i];
    }

    out[0] = global_sum;
}

#endif
