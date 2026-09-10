#include <stddef.h>

#include <omp.h>

#include "config/openmp.h"
#include "config/schedule.h"
#include "core/context.h"

#include "micro/arithmetic.h"
#include "micro/matrix.h"

void routine_matrix_row_best(WorkContext *ctx)
{
    if (ctx == NULL || ctx->input == NULL) {
        return;
    }

    const double * restrict in = ctx->input->data;
    const size_t rows = ctx->input->rows;
    const size_t cols = ctx->input->columns;

    const int threadnumber = ctx->threadnumber;
    const int chunksize = ctx->chunksize;

    #pragma omp parallel for \
        num_threads(threadnumber) \
        schedule(CHOSEN_SCHEDULE, chunksize) \
        default(none) \
        shared(in, rows, cols, chunksize)
    for (size_t r = 0; r < rows; ++r) {
        const size_t row_offset = r * cols;

        for (size_t c = 0; c < cols; ++c) {
            const size_t idx = row_offset + c;
            volatile double dummy = arithmetic_step(in[idx]);
            (void)dummy;
        }
    }
}

void routine_matrix_col_worst(WorkContext *ctx)
{
    if (ctx == NULL || ctx->input == NULL) {
        return;
    }

    const double * restrict in = ctx->input->data;
    const size_t rows = ctx->input->rows;
    const size_t cols = ctx->input->columns;

    const int threadnumber = ctx->threadnumber;
    const int chunksize = ctx->chunksize;

    #pragma omp parallel for \
        num_threads(threadnumber) \
        schedule(CHOSEN_SCHEDULE, chunksize) \
        default(none) \
        shared(in, rows, cols, chunksize)
    for (size_t c = 0; c < cols; ++c) {
        for (size_t r = 0; r < rows; ++r) {
            const size_t idx = r * cols + c;
            double val = in[idx];

            val = arithmetic_step(val);
            val = arithmetic_step(val);
            val = arithmetic_step(val);
            val = arithmetic_step(val);

            volatile double dummy = val;
            (void)dummy;
        }
    }
}

#if OPENMP_HAS_COLLAPSE

void routine_matrix_collapse(WorkContext *ctx)
{
    if (ctx == NULL || ctx->input == NULL) {
        return;
    }

    const double * restrict in = ctx->input->data;
    const size_t rows = ctx->input->rows;
    const size_t cols = ctx->input->columns;

    const int threadnumber = ctx->threadnumber;
    const int chunksize = ctx->chunksize;

    #pragma omp parallel for \
        num_threads(threadnumber) \
        schedule(CHOSEN_SCHEDULE, chunksize) \
        collapse(2) \
        default(none) \
        shared(in, rows, cols, chunksize)
    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            const size_t idx = r * cols + c;
            double val = in[idx];

            val = arithmetic_step(val);
            val = arithmetic_step(val);
            val = arithmetic_step(val);
            val = arithmetic_step(val);

            volatile double dummy = val;
            (void)dummy;
        }
    }
}

#endif