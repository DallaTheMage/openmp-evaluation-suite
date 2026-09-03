#include <stddef.h>

#include <omp.h>

#include "config/openmp.h"
#include "config/types.h"
#include "core/context.h"
#include "core/microroutines.h"
#include "config/sizes.h"
#include "config/iterations.h"
#include "config/schedule.h"
#include "config/thread.h"
#include "config/chunksize.h"

static datatype arithmetic_step(datatype value) {
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


void routine_micro_scale(WorkContext *ctx) {
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


void routine_matrix_row_best(WorkContext *ctx) {
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


void routine_matrix_col_worst(WorkContext *ctx) {
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


void routine_matrix_collapse(WorkContext *ctx) {
#if OPENMP_HAS_COLLAPSE

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
        collapse(2) \
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

#else

    (void)ctx;

#endif
}


void routine_for(WorkContext *ctx) {
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

    #pragma omp parallel \
        num_threads(threadnumber) \
        default(none) \
        shared(in, out, size, chunksize)
    {
        #pragma omp for \
            schedule(CHOSEN_SCHEDULE, chunksize)
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


void routine_reduction_sum(WorkContext *ctx) {
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
        reduction(+:sum)

    for (i = 0; i < size; ++i) {
        sum += in[i];
    }

    out[0] = sum;
}


void routine_scan_inclusive(WorkContext *ctx) {
#if OPENMP_HAS_SCAN

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
        reduction(inscan, +:running_sum)

    for (i = 0; i < size; ++i) {
        running_sum += in[i];

        #pragma omp scan inclusive(running_sum)

        out[i] = running_sum;
    }

#else

    (void)ctx;

#endif
}


void routine_sections(WorkContext *ctx) {
#if OPENMP_HAS_SECTIONS

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
                size_t i;

                for (i = 0; i < quarter; ++i) {
                    out[i] = arithmetic_step(in[i]);
                }
            }

            #pragma omp section
            {
                size_t i;

                for (i = quarter; i < 2 * quarter; ++i) {
                    out[i] = arithmetic_step(in[i]);
                }
            }

            #pragma omp section
            {
                size_t i;

                for (i = 2 * quarter; i < 3 * quarter; ++i) {
                    out[i] = arithmetic_step(in[i]);
                }
            }

            #pragma omp section
            {
                size_t i;

                for (i = 3 * quarter; i < size; ++i) {
                    out[i] = arithmetic_step(in[i]);
                }
            }
        }
    }

#else

    (void)ctx;

#endif
}


void routine_critical_reduction(WorkContext *ctx) {
#if OPENMP_HAS_CRITICAL

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
        shared(in, size, chunksize, global_sum)

    for (i = 0; i < size; ++i) {
        #pragma omp critical
        {
            global_sum += in[i];
        }
    }

    out[0] = global_sum;

#else

    (void)ctx;

#endif
}


void routine_atomic_reduction(WorkContext *ctx) {
#if OPENMP_HAS_ATOMIC

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
        shared(in, size, chunksize, global_sum)

    for (i = 0; i < size; ++i) {
        #pragma omp atomic
        global_sum += in[i];
    }

    out[0] = global_sum;

#else

    (void)ctx;

#endif
}


void routine_barrier(WorkContext *ctx) {
#if OPENMP_HAS_BARRIER

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
        #pragma omp for
        for (i = 0; i < size; ++i) {
            out[i] = arithmetic_step(in[i]);
        }

        #pragma omp barrier

        #pragma omp for
        for (i = 0; i < size; ++i) {
            out[i] = arithmetic_step(out[i]);
        }
    }

#else

    (void)ctx;

#endif
}


void routine_nowait(WorkContext *ctx) {
#if OPENMP_HAS_NOWAIT

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
        #pragma omp for nowait
        for (i = 0; i < size; ++i) {
            out[i] = arithmetic_step(in[i]);
        }

        #pragma omp for
        for (i = 0; i < size; ++i) {
            out[i] = arithmetic_step(out[i]);
        }
    }

#else

    (void)ctx;

#endif
}


void routine_ordered(WorkContext *ctx) {
#if OPENMP_HAS_ORDERED

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
        ordered \
        default(none) \
        shared(in, out, size, chunksize)

    for (i = 0; i < size; ++i) {
        datatype val;

        val = arithmetic_step(in[i]);

        #pragma omp ordered
        {
            out[i] = val;
        }
    }

#else

    (void)ctx;

#endif
}


static void compute_task_recursive(
    datatype *in,
    datatype *out,
    size_t start,
    size_t end
) {
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
        {
            compute_task_recursive(
                in,
                out,
                start,
                mid
            );
        }

        #pragma omp task \
            shared(in, out) \
            if(end - start > 4096)
        {
            compute_task_recursive(
                in,
                out,
                mid,
                end
            );
        }

        #pragma omp taskwait
    }
}


void routine_task_divide_conquer(WorkContext *ctx) {
#if OPENMP_HAS_3_0

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

#else

    (void)ctx;

#endif
}


void routine_taskloop_scale(WorkContext *ctx) {
#if OPENMP_HAS_TASKLOOP

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

#else

    (void)ctx;

#endif
}


void routine_simd(WorkContext *ctx) {
#if OPENMP_HAS_SIMD

    datatype *in;
    datatype *out;

    size_t size;
    size_t i;

    if (ctx == NULL ||
        ctx->input == NULL ||
        ctx->output == NULL) {
        return;
    }

    in = ctx->input->data;
    out = ctx->output->data;

    size = ctx->input->size;

    #pragma omp simd
    for (i = 0; i < size; ++i) {
        datatype val;
        int k;

        val = in[i];

        for (k = 0; k < 8; ++k) {
            val = arithmetic_step(val);
        }

        out[i] = val;
    }

#else

    (void)ctx;

#endif
}


void routine_parallel_for_simd(WorkContext *ctx) {
#if OPENMP_HAS_SIMD

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

    #pragma omp parallel for simd \
        num_threads(threadnumber) \
        schedule(CHOSEN_SCHEDULE, chunksize) \
        default(none) \
        shared(in, out, size, chunksize)

    for (i = 0; i < size; ++i) {
        datatype val;
        int k;

        val = in[i];

        for (k = 0; k < 8; ++k) {
            val = arithmetic_step(val);
        }

        out[i] = val;
    }

#else

    (void)ctx;

#endif
}


void routine_masked(WorkContext *ctx) {
#if OPENMP_HAS_MASKED

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
        #pragma omp masked
        {
            for (i = 0; i < size; ++i) {
                out[i] = arithmetic_step(in[i]);
            }
        }
    }

#else

    (void)ctx;

#endif
}


void routine_loop(WorkContext *ctx) {
#if OPENMP_HAS_LOOP

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
        #pragma omp loop
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

#else

    (void)ctx;

#endif
}

static const MicroRoutine microroutines[] = {
    {
        "Parallel for Arithmetic scale",
        routine_micro_scale
    },

    {
        "Parallel for best cache",
        routine_matrix_row_best
    },

    {
        "Parallel for worst cache",
        routine_matrix_col_worst
    },

#if OPENMP_HAS_COLLAPSE
    {
        "Parallel for collapse",
        routine_matrix_collapse
    },
#endif

    {
        "Parallel for",
        routine_for
    },

    {
        "Parallel for reduction",
        routine_reduction_sum
    },

#if OPENMP_HAS_SCAN
    {
        "Parallel inclusive scan",
        routine_scan_inclusive
    },
#endif

#if OPENMP_HAS_SECTIONS
    {
        "Sections",
        routine_sections
    },
#endif

#if OPENMP_HAS_CRITICAL
    {
        "Critical reduction",
        routine_critical_reduction
    },
#endif

#if OPENMP_HAS_ATOMIC
    {
        "Atomic reduction",
        routine_atomic_reduction
    },
#endif

#if OPENMP_HAS_BARRIER
    {
        "Barrier",
        routine_barrier
    },
#endif

#if OPENMP_HAS_NOWAIT
    {
        "Nowait",
        routine_nowait
    },
#endif

#if OPENMP_HAS_ORDERED
    {
        "Ordered",
        routine_ordered
    },
#endif

#if OPENMP_HAS_3_0
    {
        "Task",
        routine_task_divide_conquer
    },
#endif

#if OPENMP_HAS_TASKLOOP
    {
        "Taskloop",
        routine_taskloop_scale
    },
#endif

#if OPENMP_HAS_SIMD
    {
        "SIMD",
        routine_simd
    },

    {
        "Parallel for SIMD",
        routine_parallel_for_simd
    },
#endif

#if OPENMP_HAS_MASKED
    {
        "Masked",
        routine_masked
    },
#endif

#if OPENMP_HAS_LOOP
    {
        "Loop",
        routine_loop
    },
#endif
};

const MicroRoutine *get_microroutines(void) {
    return microroutines;
}


size_t get_microroutines_count(void) {
    return sizeof(microroutines) / sizeof(microroutines[0]);
}
