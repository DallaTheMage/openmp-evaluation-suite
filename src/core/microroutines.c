#include <omp.h>
#include <stddef.h>
#include "core/context.h"
#include "core/microroutines.h"

/*
 * 1. Microbenchmark: Scale con intensità aritmetica bilanciata
 */
void routine_micro_scale(WorkContext *ctx) {
    double *in;
    double *out;
    size_t size;
    int threadnumber;
    int chunksize;
    size_t i;

    if (ctx == NULL || ctx->input == NULL) return;

    in = (double *)ctx->input->data;
    out = (double *)ctx->output->data;
    size = ctx->input->size;

    threadnumber = ctx->threadnumber;
    chunksize = ctx->chunksize;

    #pragma omp parallel for num_threads(threadnumber) schedule(CHOSEN_SCHEDULE, chunksize) default(none) shared(in, out, size, chunksize, threadnumber) private(i)
    for (i = 0; i < size; ++i) {
        double val = in[i];
        int k;
        /* Piccolo carico computazionale per evitare il collo di bottiglia puro della RAM */
        for (k = 0; k < 8; ++k) {
            val = val * 1.000001 + 0.000001;
        }
        out[i] = val;
    }
}

/*
 * 2. Matrice BEST: Accesso per riga (Cache-friendly)
 */
void routine_matrix_row_best(WorkContext *ctx) {
    double *in;
    double *out;
    size_t rows, cols;
    int threadnumber, chunksize;
    size_t r, c;

    if (ctx == NULL || ctx->input == NULL || ctx->output == NULL) return;

    in = (double *)ctx->input->data;
    out = (double *)ctx->output->data;
    rows = ctx->input->rows;
    cols = ctx->input->columns;
    threadnumber = ctx->threadnumber;
    chunksize = ctx->chunksize;

    #pragma omp parallel for num_threads(threadnumber) schedule(CHOSEN_SCHEDULE, chunksize) default(none) shared(in, out, rows, cols, threadnumber, chunksize) private(r, c)
    for (r = 0; r < rows; ++r) {
        for (c = 0; c < cols; ++c) {
            size_t idx = r * cols + c;
            double val = in[idx];
            int k;
            for (k = 0; k < 4; ++k) {
                val = val * 1.000001;
            }
            out[idx] = val;
        }
    }
}

/*
 * 3. Matrice WORST: Accesso per colonna (Cache-unfriendly / Stride-N)
 */
void routine_matrix_col_worst(WorkContext *ctx) {
    double *in;
    double *out;
    size_t rows, cols;
    int threadnumber, chunksize;
    size_t r, c;

    if (ctx == NULL || ctx->input == NULL || ctx->output == NULL) return;

    in = (double *)ctx->input->data;
    out = (double *)ctx->output->data;
    rows = ctx->input->rows;
    cols = ctx->input->columns;
    threadnumber = ctx->threadnumber;
    chunksize = ctx->chunksize;

    #pragma omp parallel for num_threads(threadnumber) schedule(CHOSEN_SCHEDULE, chunksize) default(none) shared(in, out, rows, cols, threadnumber, chunksize) private(r, c)
    for (c = 0; c < cols; ++c) {
        for (r = 0; r < rows; ++r) {
            size_t idx = r * cols + c;
            double val = in[idx];
            int k;
            for (k = 0; k < 4; ++k) {
                val = val * 1.000001;
            }
            out[idx] = val;
        }
    }
}

/*
 * 4. Reduction: Somma vettoriale parallela
 */
void routine_reduction_sum(WorkContext *ctx) {
    double *in;
    double *out;
    size_t size;
    int threadnumber, chunksize;
    size_t i;
    double sum = 0.0;

    if (ctx == NULL || ctx->input == NULL || ctx->output == NULL) return;

    in = (double *)ctx->input->data;
    out = (double *)ctx->output->data;
    size = ctx->input->size;
    threadnumber = ctx->threadnumber;
    chunksize = ctx->chunksize;

    #pragma omp parallel for num_threads(threadnumber) schedule(CHOSEN_SCHEDULE, chunksize) default(none) shared(in, size, threadnumber, chunksize) private(i) reduction(+:sum)
    for (i = 0; i < size; ++i) {
        sum += in[i];
    }

    out[0] = sum;
}

/*
 * 5. Scan Parallela nativa con OpenMP (inscan)
 */
void routine_scan_inclusive(WorkContext *ctx) {
    double *in;
    double *out;
    size_t size;
    int threadnumber;
    size_t i;
    double running_sum;

    if (ctx == NULL || ctx->input == NULL || ctx->output == NULL) return;

    in = (double *)ctx->input->data;
    out = (double *)ctx->output->data;
    size = ctx->input->size;
    threadnumber = ctx->threadnumber;
    running_sum = 0.0;
    /*
     * Nota: Quando si usa reduction(inscan, ...), non si può specificare
     * una schedule personalizzata (es. dynamic/guided), OpenMP richiede
     * l'uso dello schedule di default (static).
     */
    #pragma omp parallel for num_threads(threadnumber) default(none) shared(in, out, size, threadnumber) private(i) reduction(inscan, +:running_sum)
    for (i = 0; i < size; ++i) {
        running_sum += in[i];
        #pragma omp scan inclusive(running_sum)
        out[i] = running_sum;
    }
}

/*
 * 6. Tasking: Visita ricorsiva/divide-et-impera con task paralleli
 */
static void compute_task_recursive(double *in, double *out, size_t start, size_t end) {
    size_t threshold = 1024; /* Soglia per passare al calcolo sequenziale */

    if (end - start <= threshold) {
        size_t i;
        for (i = start; i < end; ++i) {
            double val = in[i];
            int k;
            for (k = 0; k < 4; ++k) {
                val = val * 1.000001 + 0.000001;
            }
            out[i] = val;
        }
    } else {
        size_t mid = start + (end - start) / 2;

        #pragma omp task shared(in, out) if(end - start > 4096)
        compute_task_recursive(in, out, start, mid);

        #pragma omp task shared(in, out) if(end - start > 4096)
        compute_task_recursive(in, out, mid, end);

        #pragma omp taskwait
    }
}

void routine_task_divide_conquer(WorkContext *ctx) {
    double *in;
    double *out;
    size_t size;
    int threadnumber;

    if (ctx == NULL || ctx->input == NULL || ctx->output == NULL) return;

    in = (double *)ctx->input->data;
    out = (double *)ctx->output->data;
    size = ctx->input->size;
    threadnumber = ctx->threadnumber;

    #pragma omp parallel num_threads(threadnumber) default(none) shared(in, out, size)
    {
        #pragma omp single
        {
            compute_task_recursive(in, out, 0, size);
        }
    }
}

/*
 * 7. Synchronization: Riduzione manuale con #pragma omp atomic (Alternative approach)
 */
void routine_atomic_reduction(WorkContext *ctx) {
    double *in;
    double *out;
    size_t size;
    int threadnumber, chunksize;
    size_t i;
    double global_sum = 0.0;

    if (ctx == NULL || ctx->input == NULL || ctx->output == NULL) return;

    in = (double *)ctx->input->data;
    out = (double *)ctx->output->data;
    size = ctx->input->size;
    threadnumber = ctx->threadnumber;
    chunksize = ctx->chunksize;

    #pragma omp parallel for num_threads(threadnumber) schedule(CHOSEN_SCHEDULE, chunksize) default(none) shared(in, size, threadnumber, chunksize, global_sum) private(i)
    for (i = 0; i < size; ++i) {
        #pragma omp atomic
        global_sum += in[i];
    }

    out[0] = global_sum;
}

/*
 * 8. Taskloop: Parallelizzazione di un ciclo tramite task (OpenMP 4.5+)
 */
void routine_taskloop_scale(WorkContext *ctx) {
    double *in;
    double *out;
    size_t size;
    int threadnumber;
    size_t i;

    if (ctx == NULL || ctx->input == NULL || ctx->output == NULL) return;

    in = (double *)ctx->input->data;
    out = (double *)ctx->output->data;
    size = ctx->input->size;
    threadnumber = ctx->threadnumber;

    #pragma omp parallel num_threads(threadnumber) default(none) shared(in, out, size)
    {
        #pragma omp single
        {
            #pragma omp taskloop grainsize(512) shared(in, out, size) private(i)
            for (i = 0; i < size; ++i) {
                double val = in[i];
                int k;
                for (k = 0; k < 8; ++k) {
                    val = val * 1.000001 + 0.000001;
                }
                out[i] = val;
            }
        }
    }
}