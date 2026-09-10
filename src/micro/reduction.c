#include <stddef.h>

#include <omp.h>

#include "config/openmp.h"
#include "config/schedule.h"
#include "core/context.h"

#include "micro/reduction.h"

void routine_reduction_sum(WorkContext *ctx)
{
    if (ctx == NULL || ctx->input == NULL) {
        return;
    }

    const double *in = ctx->input->data;
    size_t size = ctx->input->size;
    int threadnumber = ctx->threadnumber;
    int chunksize = ctx->chunksize;

    double sum = (double)0;

    #pragma omp parallel for \
        num_threads(threadnumber) \
        schedule(CHOSEN_SCHEDULE, chunksize) \
        default(none) \
        shared(in, size, chunksize) \
        reduction(+:sum)
    for (size_t i = 0; i < size; ++i) {
        sum += in[i];
    }

    volatile double dummy = sum;
    (void)dummy;
}

#if OPENMP_HAS_SCAN

void routine_scan_inclusive(WorkContext *ctx)
{
    if (ctx == NULL || ctx->input == NULL) {
        return;
    }

    const double *in = ctx->input->data;
    size_t size = ctx->input->size;
    int threadnumber = ctx->threadnumber;

    double running_sum = (double)0;

    #pragma omp parallel for \
        num_threads(threadnumber) \
        default(none) \
        shared(in, size) \
        reduction(inscan, +:running_sum)
    for (size_t i = 0; i < size; ++i) {
        running_sum += in[i];

        #pragma omp scan inclusive(running_sum)

        volatile double dummy = running_sum;
        (void)dummy;
    }
}

#endif

#if OPENMP_HAS_CRITICAL

void routine_critical_reduction(WorkContext *ctx)
{
    if (ctx == NULL || ctx->input == NULL) {
        return;
    }

    const double *in = ctx->input->data;
    size_t size = ctx->input->size;
    int threadnumber = ctx->threadnumber;
    int chunksize = ctx->chunksize;

    double global_sum = (double)0;

    #pragma omp parallel for \
        num_threads(threadnumber) \
        schedule(CHOSEN_SCHEDULE, chunksize) \
        default(none) \
        shared(in, size, global_sum)
    for (size_t i = 0; i < size; ++i) {
        #pragma omp critical
        global_sum += in[i];
    }

    volatile double dummy = global_sum;
    (void)dummy;
}

#endif

#if OPENMP_HAS_ATOMIC

void routine_atomic_reduction(WorkContext *ctx)
{
    if (ctx == NULL || ctx->input == NULL) {
        return;
    }

    const double *in = ctx->input->data;
    size_t size = ctx->input->size;
    int threadnumber = ctx->threadnumber;
    int chunksize = ctx->chunksize;

    double global_sum = (double)0;

    #pragma omp parallel for \
        num_threads(threadnumber) \
        schedule(CHOSEN_SCHEDULE, chunksize) \
        default(none) \
        shared(in, size, global_sum)
    for (size_t i = 0; i < size; ++i) {
        #pragma omp atomic
        global_sum += in[i];
    }

    volatile double dummy = global_sum;
    (void)dummy;
}

#endif