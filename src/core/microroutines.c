#include <stddef.h>
#include <omp.h>

#include "config/types.h"
#include "core/context.h"
#include "core/microroutines.h"


/*
 * ============================================================
 * Generic arithmetic operation
 * ============================================================
 */

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


/*
 * ============================================================
 * 1. Parallel scale
 * ============================================================
 */

void routine_micro_scale(WorkContext *ctx)
{
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

    #pragma omp parallel for \
        num_threads(threadnumber) \
        schedule(CHOSEN_SCHEDULE, chunksize) \
        default(none) \
        shared(in, out, size, chunksize) \
        private(i)

    for (i = 0; i < size; ++i) {

        datatype val;
        int k;

        val = in[i];

        for (k = 0; k < 8; ++k) {
            val = arithmetic_step(val);
        }

        out[i] = val;
    }
}


/*
 * ============================================================
 * 2. Matrix row-major
 * ============================================================
 */

void routine_matrix_row_best(WorkContext *ctx)
{
    datatype *in;
    datatype *out;

    size_t rows;
    size_t cols;

    size_t r;
    size_t c;

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
        shared(in, out, rows, cols, chunksize) \
        private(r, c)

    for (r = 0; r < rows; ++r) {

        for (c = 0; c < cols; ++c) {

            size_t idx;
            datatype val;
            int k;

            idx = r * cols + c;

            val = in[idx];

            for (k = 0; k < 4; ++k) {
                val = arithmetic_step(val);
            }

            out[idx] = val;
        }
    }
}


/*
 * ============================================================
 * 3. Matrix column-major
 * ============================================================
 */

void routine_matrix_col_worst(WorkContext *ctx)
{
    datatype *in;
    datatype *out;

    size_t rows;
    size_t cols;

    size_t r;
    size_t c;

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
        shared(in, out, rows, cols, chunksize) \
        private(r, c)

    for (c = 0; c < cols; ++c) {

        for (r = 0; r < rows; ++r) {

            size_t idx;
            datatype val;
            int k;

            idx = r * cols + c;

            val = in[idx];

            for (k = 0; k < 4; ++k) {
                val = arithmetic_step(val);
            }

            out[idx] = val;
        }
    }
}


/*
 * ============================================================
 * 4. Parallel reduction
 * ============================================================
 */

void routine_reduction_sum(WorkContext *ctx)
{
    datatype *in;
    datatype *out;

    size_t size;
    size_t i;

    int threadnumber;
    int chunksize;

    datatype sum;

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

    sum = (datatype)0;

    #pragma omp parallel for \
        num_threads(threadnumber) \
        schedule(CHOSEN_SCHEDULE, chunksize) \
        default(none) \
        shared(in, size, chunksize) \
        private(i) \
        reduction(+:sum)

    for (i = 0; i < size; ++i) {
        sum += in[i];
    }

    out[0] = sum;
}


/*
 * ============================================================
 * 5. Parallel inclusive scan
 * ============================================================
 */

void routine_scan_inclusive(WorkContext *ctx)
{
    datatype *in;
    datatype *out;

    size_t size;
    size_t i;

    int threadnumber;

    datatype running_sum;

    if (ctx == NULL ||
        ctx->input == NULL ||
        ctx->output == NULL) {
        return;
    }

    in = ctx->input->data;
    out = ctx->output->data;

    size = ctx->input->size;

    threadnumber = ctx->threadnumber;

    running_sum = (datatype)0;

    #pragma omp parallel for \
        num_threads(threadnumber) \
        default(none) \
        shared(in, out, size) \
        private(i) \
        reduction(inscan, +:running_sum)

    for (i = 0; i < size; ++i) {

        running_sum += in[i];

        #pragma omp scan inclusive(running_sum)

        out[i] = running_sum;
    }
}


/*
 * ============================================================
 * 6. Task divide-and-conquer
 * ============================================================
 */

static void compute_task_recursive(
    datatype *in,
    datatype *out,
    size_t start,
    size_t end
)
{
    const size_t threshold = 1024;

    if (end - start <= threshold) {

        size_t i;

        for (i = start; i < end; ++i) {

            datatype val;
            int k;

            val = in[i];

            for (k = 0; k < 4; ++k) {
                val = arithmetic_step(val);
            }

            out[i] = val;
        }

        return;
    }

    {
        size_t mid;

        mid = start + (end - start) / 2;

        #pragma omp task \
            shared(in, out) \
            if(end - start > 4096)

        compute_task_recursive(
            in,
            out,
            start,
            mid
        );

        #pragma omp task \
            shared(in, out) \
            if(end - start > 4096)

        compute_task_recursive(
            in,
            out,
            mid,
            end
        );

        #pragma omp taskwait
    }
}


void routine_task_divide_conquer(WorkContext *ctx)
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
        #pragma omp single
        {
            compute_task_recursive(
                in,
                out,
                0,
                size
            );
        }
    }
}


/*
 * ============================================================
 * 7. Atomic reduction
 * ============================================================
 */

void routine_atomic_reduction(WorkContext *ctx)
{
    datatype *in;
    datatype *out;

    size_t size;
    size_t i;

    int threadnumber;
    int chunksize;

    datatype global_sum;

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

    global_sum = (datatype)0;

    #pragma omp parallel for \
        num_threads(threadnumber) \
        schedule(CHOSEN_SCHEDULE, chunksize) \
        default(none) \
        shared(in, size, chunksize, global_sum) \
        private(i)

    for (i = 0; i < size; ++i) {

        #pragma omp atomic
        global_sum += in[i];
    }

    out[0] = global_sum;
}


/*
 * ============================================================
 * 8. Taskloop scale
 * ============================================================
 */

void routine_taskloop_scale(WorkContext *ctx)
{
    datatype *in;
    datatype *out;

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

    #pragma omp parallel \
        num_threads(threadnumber) \
        default(none) \
        shared(in, out, size)
    {
        #pragma omp single
        {
            #pragma omp taskloop \
                grainsize(512) \
                shared(in, out, size) \
                private(i)

            for (i = 0; i < size; ++i) {

                datatype val;
                int k;

                val = in[i];

                for (k = 0; k < 8; ++k) {
                    val = arithmetic_step(val);
                }

                out[i] = val;
            }
        }
    }
}