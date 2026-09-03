#include <stddef.h>

#include <omp.h>

#include "config/openmp.h"
#include "config/schedule.h"
#include "core/context.h"

#include "micro/arithmetic.h"
#include "micro/matrix.h"


void routine_matrix_row_best(WorkContext *ctx)
{
    datatype *in;
    datatype *out;

    size_t rows;
    size_t cols;

    int threadnumber;
    int chunksize;

    if (ctx == NULL ||
        ctx->input == NULL ||
        ctx->output == NULL) {
        return;
    }

    in = ctx->input->data;
    out = ctx->output->data;

    rows = ctx->input->rows;
    cols = ctx->input->columns;

    threadnumber = ctx->threadnumber;
    chunksize = ctx->chunksize;

    #pragma omp parallel for \
        num_threads(threadnumber) \
        schedule(CHOSEN_SCHEDULE, chunksize) \
        default(none) \
        shared(in, out, rows, cols, chunksize)

    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            size_t idx = r * cols + c;
            datatype val = in[idx];

            for (int k = 0; k < 4; ++k)
                val = arithmetic_step(val);

            out[idx] = val;
        }
    }
}


void routine_matrix_col_worst(WorkContext *ctx)
{
    datatype *in;
    datatype *out;

    size_t rows;
    size_t cols;

    int threadnumber;
    int chunksize;

    if (ctx == NULL ||
        ctx->input == NULL ||
        ctx->output == NULL) {
        return;
    }

    in = ctx->input->data;
    out = ctx->output->data;

    rows = ctx->input->rows;
    cols = ctx->input->columns;

    threadnumber = ctx->threadnumber;
    chunksize = ctx->chunksize;

    #pragma omp parallel for \
        num_threads(threadnumber) \
        schedule(CHOSEN_SCHEDULE, chunksize) \
        default(none) \
        shared(in, out, rows, cols, chunksize)

    for (size_t c = 0; c < cols; ++c) {
        for (size_t r = 0; r < rows; ++r) {
            size_t idx = r * cols + c;
            datatype val = in[idx];

            for (int k = 0; k < 4; ++k)
                val = arithmetic_step(val);

            out[idx] = val;
        }
    }
}


#if OPENMP_HAS_COLLAPSE

void routine_matrix_collapse(WorkContext *ctx)
{
    datatype *in;
    datatype *out;

    size_t rows;
    size_t cols;

    int threadnumber;
    int chunksize;

    if (ctx == NULL ||
        ctx->input == NULL ||
        ctx->output == NULL) {
        return;
    }

    in = ctx->input->data;
    out = ctx->output->data;

    rows = ctx->input->rows;
    cols = ctx->input->columns;

    threadnumber = ctx->threadnumber;
    chunksize = ctx->chunksize;

    #pragma omp parallel for \
        num_threads(threadnumber) \
        schedule(CHOSEN_SCHEDULE, chunksize) \
        collapse(2) \
        default(none) \
        shared(in, out, rows, cols, chunksize)

    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            size_t idx = r * cols + c;
            datatype val = in[idx];

            for (int k = 0; k < 4; ++k)
                val = arithmetic_step(val);

            out[idx] = val;
        }
    }
}

#endif
