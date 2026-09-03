#include <stddef.h>

#include <omp.h>

#include "config/openmp.h"
#include "core/context.h"

#include "micro/arithmetic.h"
#include "micro/tasks.h"


#if OPENMP_HAS_3_0

static void compute_task_recursive(
    datatype *in,
    datatype *out,
    size_t start,
    size_t end
)
{
    const size_t threshold = 1024;
    const size_t task_threshold = 4096;

    if (end - start <= threshold) {
        for (size_t i = start; i < end; ++i) {
            datatype val = in[i];

            for (int k = 0; k < 4; ++k)
                val = arithmetic_step(val);

            out[i] = val;
        }

        return;
    }

    size_t mid = start + (end - start) / 2;

    #pragma omp task shared(in, out) if(end - start > task_threshold)
    compute_task_recursive(in, out, start, mid);

    #pragma omp task shared(in, out) if(end - start > task_threshold)
    compute_task_recursive(in, out, mid, end);

    #pragma omp taskwait
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
        compute_task_recursive(in, out, 0, size);
    }
}

#endif


#if OPENMP_HAS_TASKLOOP

void routine_taskloop_scale(WorkContext *ctx)
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
            #pragma omp taskloop \
                grainsize(512)

            for (size_t i = 0; i < size; ++i) {
                datatype val = in[i];

                for (int k = 0; k < 8; ++k)
                    val = arithmetic_step(val);

                out[i] = val;
            }
        }
    }
}

#endif
