#include "kernels/scan.h"
#include "profiling/Profiler.h"
#include <stdlib.h>

/* Helper statico per eseguire il Two-Pass Scan mantenendo l'allocazione FUORI dalla misurazione */
#if OPENMP_HAS_TWO_PASS_SCAN
static inline void run_twopass_omp(double *restrict pool, uint64_t total, uint16_t num_threads, LaunchConfig *config) {
    if (total == 0 || !pool) return;
    if (num_threads == 0) num_threads = 1;

    double *offsets = (double *)calloc(num_threads, sizeof(double));
    if (!offsets) return;

    if (config->is_measuring) profiler_start(config->profiler);

    #pragma omp parallel num_threads(num_threads) default(none) shared(pool, total, offsets)
    {
        int tid = omp_get_thread_num();
        int nth = omp_get_num_threads();

        uint64_t items_per_thread = total / nth;
        uint64_t remainder        = total % nth;

        uint64_t start = (uint64_t)tid * items_per_thread + (uint64_t)(tid < (int)remainder ? tid : remainder);
        uint64_t count = items_per_thread + (tid < (int)remainder ? 1 : 0);
        uint64_t end   = start + count;

        /* Pass 1: Scansione inclusiva locale + accumulo somma parziale */
        double acc = 0.0;
        for (uint64_t i = start; i < end; ++i) {
            acc += pool[i];
            pool[i] = acc;
        }
        offsets[tid] = acc;

        #pragma omp barrier

        /* Thread 0 calcola la scansione esclusiva degli offset dei thread */
        #pragma omp single
        {
            double running = 0.0;
            for (int t = 0; t < nth; ++t) {
                double tmp = offsets[t];
                offsets[t] = running;
                running += tmp;
            }
        }

        /* Pass 2: Aggiunta dell'offset del thread al blocco locale */
        double thread_offset = offsets[tid];
        if (thread_offset != 0.0) {
            for (uint64_t i = start; i < end; ++i) {
                pool[i] += thread_offset;
            }
        }
    }

    if (config->is_measuring) profiler_stop(config->profiler, &config->last_metric);

    free(offsets);
}
#endif

/* =========================================================================
 * 1. NATIVE OPENMP 5.0 INSCAN (#pragma omp scan)
 * ========================================================================= */
#if OPENMP_HAS_NATIVE_SCAN

void kernel_native_scan_run(DataView *view, LaunchConfig *config) {
    if (!view || !view->buffer || !config) return;

    const uint16_t num_threads = config->threadnumber;
    const uint32_t chunk       = config->chunksize;
    const bool measure         = config->is_measuring;
    double *restrict pool      = view->buffer->pool;

    switch (view->type) {
        case VIEW_1D: {
            const uint64_t dim0 = view->meta.v1d.dim0;

            if (measure) profiler_start(config->profiler);
            double sum = 0.0;
            #pragma omp parallel num_threads(num_threads) default(none) \
                    shared(pool, dim0, chunk) reduction(inscan, +:sum)
            {
                #pragma omp for schedule(static, chunk)
                for (uint64_t i = 0; i < dim0; ++i) {
                    sum += pool[i];
                    #pragma omp scan inclusive(sum)
                    pool[i] = sum;
                }
            }
            if (measure) profiler_stop(config->profiler, &config->last_metric);
            break;
        }
        case VIEW_2D: {
            const uint64_t total = view->meta.v2d.rows * view->meta.v2d.cols;

            if (measure) profiler_start(config->profiler);
            double sum = 0.0;
            #pragma omp parallel num_threads(num_threads) default(none) \
                    shared(pool, total, chunk) reduction(inscan, +:sum)
            {
                #pragma omp for schedule(static, chunk)
                for (uint64_t i = 0; i < total; ++i) {
                    sum += pool[i];
                    #pragma omp scan inclusive(sum)
                    pool[i] = sum;
                }
            }
            if (measure) profiler_stop(config->profiler, &config->last_metric);
            break;
        }
        case VIEW_3D: {
            const uint64_t total = view->meta.v3d.rows * view->meta.v3d.cols * view->meta.v3d.depth;

            if (measure) profiler_start(config->profiler);
            double sum = 0.0;
            #pragma omp parallel num_threads(num_threads) default(none) \
                    shared(pool, total, chunk) reduction(inscan, +:sum)
            {
                #pragma omp for schedule(static, chunk)
                for (uint64_t i = 0; i < total; ++i) {
                    sum += pool[i];
                    #pragma omp scan inclusive(sum)
                    pool[i] = sum;
                }
            }
            if (measure) profiler_stop(config->profiler, &config->last_metric);
            break;
        }
        case VIEW_CSR: {
            if (!view->meta.csr.row_ptr) break;
            const uint64_t total = view->meta.csr.row_ptr[view->meta.csr.nrows];

            if (measure) profiler_start(config->profiler);
            double sum = 0.0;
            #pragma omp parallel num_threads(num_threads) default(none) \
                    shared(pool, total, chunk) reduction(inscan, +:sum)
            {
                #pragma omp for schedule(static, chunk)
                for (uint64_t i = 0; i < total; ++i) {
                    sum += pool[i];
                    #pragma omp scan inclusive(sum)
                    pool[i] = sum;
                }
            }
            if (measure) profiler_stop(config->profiler, &config->last_metric);
            break;
        }
        case VIEW_AOS: {
            const uint64_t total = view->meta.aos.num_elements * view->meta.aos.struct_size;

            if (measure) profiler_start(config->profiler);
            double sum = 0.0;
            #pragma omp parallel num_threads(num_threads) default(none) \
                    shared(pool, total, chunk) reduction(inscan, +:sum)
            {
                #pragma omp for schedule(static, chunk)
                for (uint64_t i = 0; i < total; ++i) {
                    sum += pool[i];
                    #pragma omp scan inclusive(sum)
                    pool[i] = sum;
                }
            }
            if (measure) profiler_stop(config->profiler, &config->last_metric);
            break;
        }
        case VIEW_SOA: {
            const uint64_t total = view->meta.soa.num_elements * view->meta.soa.num_fields;

            if (measure) profiler_start(config->profiler);
            double sum = 0.0;
            #pragma omp parallel num_threads(num_threads) default(none) \
                    shared(pool, total, chunk) reduction(inscan, +:sum)
            {
                #pragma omp for schedule(static, chunk)
                for (uint64_t i = 0; i < total; ++i) {
                    sum += pool[i];
                    #pragma omp scan inclusive(sum)
                    pool[i] = sum;
                }
            }
            if (measure) profiler_stop(config->profiler, &config->last_metric);
            break;
        }
        case VIEW_AOSOA: {
            const uint64_t vl = view->meta.aosoa.vector_length;
            if (vl == 0) break;
            const uint64_t total = (view->meta.aosoa.num_elements / vl) * view->meta.aosoa.num_fields * vl;

            if (measure) profiler_start(config->profiler);
            double sum = 0.0;
            #pragma omp parallel num_threads(num_threads) default(none) \
                    shared(pool, total, chunk) reduction(inscan, +:sum)
            {
                #pragma omp for schedule(static, chunk)
                for (uint64_t i = 0; i < total; ++i) {
                    sum += pool[i];
                    #pragma omp scan inclusive(sum)
                    pool[i] = sum;
                }
            }
            if (measure) profiler_stop(config->profiler, &config->last_metric);
            break;
        }
        default: break;
    }
}

#endif /* OPENMP_HAS_NATIVE_SCAN */


/* =========================================================================
 * 2. CUSTOM TWO-PASS PARALLEL SCAN (OpenMP 2.0+)
 * ========================================================================= */
#if OPENMP_HAS_TWO_PASS_SCAN

void kernel_twopass_scan_run(DataView *view, LaunchConfig *config) {
    if (!view || !view->buffer || !config) return;

    const uint16_t num_threads = config->threadnumber;
    double *restrict pool      = view->buffer->pool;

    switch (view->type) {
        case VIEW_1D: {
            const uint64_t total = view->meta.v1d.dim0;
            run_twopass_omp(pool, total, num_threads, config);
            break;
        }
        case VIEW_2D: {
            const uint64_t total = view->meta.v2d.rows * view->meta.v2d.cols;
            run_twopass_omp(pool, total, num_threads, config);
            break;
        }
        case VIEW_3D: {
            const uint64_t total = view->meta.v3d.rows * view->meta.v3d.cols * view->meta.v3d.depth;
            run_twopass_omp(pool, total, num_threads, config);
            break;
        }
        case VIEW_CSR: {
            if (!view->meta.csr.row_ptr) break;
            const uint64_t total = view->meta.csr.row_ptr[view->meta.csr.nrows];
            run_twopass_omp(pool, total, num_threads, config);
            break;
        }
        case VIEW_AOS: {
            const uint64_t total = view->meta.aos.num_elements * view->meta.aos.struct_size;
            run_twopass_omp(pool, total, num_threads, config);
            break;
        }
        case VIEW_SOA: {
            const uint64_t total = view->meta.soa.num_elements * view->meta.soa.num_fields;
            run_twopass_omp(pool, total, num_threads, config);
            break;
        }
        case VIEW_AOSOA: {
            const uint64_t vl = view->meta.aosoa.vector_length;
            if (vl == 0) break;
            const uint64_t total = (view->meta.aosoa.num_elements / vl) * view->meta.aosoa.num_fields * vl;
            run_twopass_omp(pool, total, num_threads, config);
            break;
        }
        default: break;
    }
}

#endif /* OPENMP_HAS_TWO_PASS_SCAN */