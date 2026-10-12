#include "kernels/scan/two_pass.h"
#include "config/openmp.h"
#include "profiling/Profiler.h"
#include "core/TestPlan.h"

#include <stdlib.h>
#include <omp.h>

#if OPENMP_HAS_TWO_PASS_SCAN
/* ============================================================================
 * INTERNAL TWO-PASS IMPLEMENTATION
 * ========================================================================== */
static KernelStatus run_twopass_omp(double *restrict pool, uint64_t total, uint16_t num_threads, Profiler *profiler, PerformanceMetric *metric) {
    if (pool == NULL || total == 0U || profiler == NULL || metric == NULL) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }
    if (num_threads == 0) {
        num_threads = 1;
    }
    /*
     * Allocazione volutamente fuori dalla regione misurata.
     *
     * Il costo della gestione degli offset non deve contaminare il
     * benchmark del costrutto/algoritmo di scan.
     */
    double *offsets = calloc(num_threads, sizeof(double));
    if (offsets == NULL) {
        return KERNEL_STATUS_ALLOCATION_FAILURE;
    }
    if (profiler_start(profiler) != 0) {
        free(offsets);
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, total, offsets)
    {
        const uint64_t tid              = (uint64_t)omp_get_thread_num();
        const uint64_t nth              = (uint64_t)omp_get_num_threads();
        const uint64_t items_per_thread = total / nth;
        const uint64_t remainder        = total % nth;
        const uint64_t start            = tid * items_per_thread + (tid < remainder ? tid : remainder);
        const uint64_t count            = items_per_thread + (tid < remainder ? UINT64_C(1) : UINT64_C(0));
        const uint64_t end              = start + count;

        /*
         * ---------------------------------------------------------------
         * PASS 1
         * ---------------------------------------------------------------
         *
         * Ogni thread esegue una scan inclusiva locale sul proprio
         * intervallo.
         */

        double acc = 0.0;
        for (uint64_t i = start; i < end; ++i) {
            acc += pool[i];
            pool[i] = acc;
        }
        offsets[tid] = acc;

        /*
         * Tutti i thread devono aver scritto offsets[] prima che
         * il thread singolo li utilizzi.
         */

        #pragma omp barrier
        /*
         * ---------------------------------------------------------------
         * OFFSET SCAN
         * ---------------------------------------------------------------
         *
         * Il thread 0 trasforma le somme locali in offset esclusivi.
         *
         * Esempio:
         *
         *   offsets = [10, 20, 30, 40]
         *
         * diventa:
         *
         *   offsets = [0, 10, 30, 60]
         */
        #pragma omp single
        {
            double running = 0.0;
            for (uint64_t t = 0U; t < nth; ++t) {
                const double tmp = offsets[t];
                offsets[t]       = running;
                running         += tmp;
            }
        }

        /*
         * ---------------------------------------------------------------
         * PASS 2
         * ---------------------------------------------------------------
         *
         * Ogni thread aggiunge al proprio blocco l'offset globale.
         */

        const double thread_offset = offsets[tid];
        if (thread_offset != 0.0) {
            for (uint64_t i = start; i < end; ++i) {
                pool[i] += thread_offset;
            }
        }
    }

    const int stop_status = profiler_stop(profiler, metric);
    free(offsets);
    if (stop_status != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}

/* ============================================================================
 * VIEW_2D
 * ========================================================================== */
KernelStatus kernel_2d_two_pass_scan(DataView *view, const TestCase *test_case, Profiler *profiler, PerformanceMetric *metric) {
    const uint64_t total = view->meta.v2d.rows * view->meta.v2d.cols;
    return run_twopass_omp(view->buffer->pool, total, test_case->num_threads, profiler, metric);
}

/* ============================================================================
 * VIEW_3D
 * ========================================================================== */
KernelStatus kernel_3d_two_pass_scan(DataView *view, const TestCase *test_case, Profiler *profiler, PerformanceMetric *metric) {
    const uint64_t total = view->meta.v3d.height * view->meta.v3d.cols * view->meta.v3d.depth;
    return run_twopass_omp(view->buffer->pool, total, test_case->num_threads, profiler, metric);
}

/* ============================================================================
 * VIEW_AOS
 * ========================================================================== */
KernelStatus kernel_aos_two_pass_scan(DataView *view, const TestCase *test_case, Profiler *profiler, PerformanceMetric *metric) {
    const uint64_t total = view->meta.aos.num_structs * view->meta.aos.struct_size;
    return run_twopass_omp(view->buffer->pool, total, test_case->num_threads, profiler, metric);
}

/* ============================================================================
 * VIEW_SOA
 * ========================================================================== */
KernelStatus kernel_soa_two_pass_scan(DataView *view, const TestCase *test_case, Profiler *profiler, PerformanceMetric *metric) {
    const uint64_t total = view->meta.soa.field_length * view->meta.soa.num_fields;
    return run_twopass_omp(view->buffer->pool, total, test_case->num_threads, profiler, metric);
}


/* ============================================================================
 * VIEW_AOSOA
 * ========================================================================== */
KernelStatus kernel_aosoa_two_pass_scan(DataView *view, const TestCase *test_case, Profiler *profiler, PerformanceMetric *metric) {
    const uint64_t vl = view->meta.aosoa.vector_length;
    if (vl == 0) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }
    const uint64_t total = view->meta.aosoa.num_blocks * view->meta.aosoa.num_fields * vl;
    return run_twopass_omp(view->buffer->pool, total, test_case->num_threads, profiler, metric);
}
#endif /* OPENMP_HAS_TWO_PASS_SCAN */
