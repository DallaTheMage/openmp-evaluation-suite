#include <stddef.h>

#include <omp.h>

#include "config/openmp.h"
#include "core/context.h"

#include "micro/arithmetic.h"
#include "micro/tasks.h"

#if OPENMP_HAS_3_0

static void compute_task_recursive(const double *in, size_t start, size_t end)
{
    const size_t threshold = 1024;
    const size_t task_threshold = 4096;

    if (end - start <= threshold) {
        for (size_t i = start; i < end; ++i) {
            double val = in[i];
            for (int k = 0; k < 4; ++k) {
                val = arithmetic_step(val);
            }
            volatile double dummy = val;
            (void)dummy;
        }
        return;
    }

    size_t mid = start + (end - start) / 2;

    #pragma omp task shared(in) if(end - start > task_threshold)
    compute_task_recursive(in, start, mid);

    #pragma omp task shared(in) if(end - start > task_threshold)
    compute_task_recursive(in, mid, end);

    #pragma omp taskwait
}

void routine_task_divide_conquer(WorkContext *ctx)
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
        #pragma omp single
        compute_task_recursive(in, 0, size);
    }
}

#endif

#if OPENMP_HAS_TASKLOOP

void routine_taskloop_scale(WorkContext *ctx)
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
        #pragma omp single
        {
            #pragma omp taskloop grainsize(512)
            for (size_t i = 0; i < size; ++i) {
                double val = in[i];

                for (int k = 0; k < 8; ++k) {
                    val = arithmetic_step(val);
                }

                volatile double dummy = val;
                (void)dummy;
            }
        }
    }
}

#endif