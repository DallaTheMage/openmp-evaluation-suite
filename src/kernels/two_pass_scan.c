#include "kernels/scan.h"
#include "profiling/Profiler.h"

#include <stdlib.h>

#if OPENMP_HAS_TWO_PASS_SCAN


/* ============================================================================
 * INTERNAL TWO-PASS IMPLEMENTATION
 * ========================================================================== */

static void run_twopass_omp(double *restrict pool,
                            uint64_t total,
                            uint16_t num_threads,
                            const TestCase *test_case,
                            Profiler *profiler,
                            PerformanceMetric *metric) {

    if (!pool || total == 0) {
        return;
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
    double *offsets =
        calloc(num_threads, sizeof(double));

    if (!offsets) {
        return;
    }

    profiler_start(profiler);

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, total, offsets)
    {
        const int tid =
            omp_get_thread_num();

        const int nth =
            omp_get_num_threads();

        const uint64_t items_per_thread =
            total / (uint64_t)nth;

        const uint64_t remainder =
            total % (uint64_t)nth;

        const uint64_t start =
            (uint64_t)tid * items_per_thread +
            (uint64_t)(
                tid < (int)remainder
                    ? tid
                    : remainder
            );

        const uint64_t count =
            items_per_thread +
            (uint64_t)(
                tid < (int)remainder
                    ? 1
                    : 0
            );

        const uint64_t end =
            start + count;


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

            for (int t = 0; t < nth; ++t) {

                const double tmp =
                    offsets[t];

                offsets[t] = running;

                running += tmp;
            }
        }


        /*
         * ---------------------------------------------------------------
         * PASS 2
         * ---------------------------------------------------------------
         *
         * Ogni thread aggiunge al proprio blocco l'offset globale.
         */

        const double thread_offset =
            offsets[tid];

        if (thread_offset != 0.0) {

            for (uint64_t i = start; i < end; ++i) {
                pool[i] += thread_offset;
            }
        }
    }

    profiler_stop(profiler, metric);

    free(offsets);
}


/* ============================================================================
 * VIEW_2D
 * ========================================================================== */

void kernel_2d_two_pass_scan(DataView *view,
                             const TestCase *test_case,
                             Profiler *profiler,
                             PerformanceMetric *metric) {

    const uint64_t total =
        view->meta.v2d.rows *
        view->meta.v2d.cols;

    run_twopass_omp(
        view->buffer->pool,
        total,
        test_case->num_threads,
        test_case,
        profiler,
        metric
    );
}


/* ============================================================================
 * VIEW_3D
 * ========================================================================== */

void kernel_3d_two_pass_scan(DataView *view,
                             const TestCase *test_case,
                             Profiler *profiler,
                             PerformanceMetric *metric) {

    const uint64_t total =
        view->meta.v3d.rows *
        view->meta.v3d.cols *
        view->meta.v3d.depth;

    run_twopass_omp(
        view->buffer->pool,
        total,
        test_case->num_threads,
        test_case,
        profiler,
        metric
    );
}


/* ============================================================================
 * VIEW_AOS
 * ========================================================================== */

void kernel_aos_two_pass_scan(DataView *view,
                              const TestCase *test_case,
                              Profiler *profiler,
                              PerformanceMetric *metric) {

    const uint64_t total =
        view->meta.aos.num_elements *
        view->meta.aos.struct_size;

    run_twopass_omp(
        view->buffer->pool,
        total,
        test_case->num_threads,
        test_case,
        profiler,
        metric
    );
}


/* ============================================================================
 * VIEW_SOA
 * ========================================================================== */

void kernel_soa_two_pass_scan(DataView *view,
                              const TestCase *test_case,
                              Profiler *profiler,
                              PerformanceMetric *metric) {

    const uint64_t total =
        view->meta.soa.num_elements *
        view->meta.soa.num_fields;

    run_twopass_omp(
        view->buffer->pool,
        total,
        test_case->num_threads,
        test_case,
        profiler,
        metric
    );
}


/* ============================================================================
 * VIEW_AOSOA
 * ========================================================================== */

void kernel_aosoa_two_pass_scan(DataView *view,
                                const TestCase *test_case,
                                Profiler *profiler,
                                PerformanceMetric *metric) {

    const uint64_t vl =
        view->meta.aosoa.vector_length;

    if (vl == 0) {
        return;
    }

    const uint64_t total =
        (view->meta.aosoa.num_elements / vl) *
        view->meta.aosoa.num_fields *
        vl;

    run_twopass_omp(
        view->buffer->pool,
        total,
        test_case->num_threads,
        test_case,
        profiler,
        metric
    );
}


/* ============================================================================
 * VIEW_CSR
 * ========================================================================== */

void kernel_csr_two_pass_scan(DataView *view,
                              const TestCase *test_case,
                              Profiler *profiler,
                              PerformanceMetric *metric) {

    if (!view->meta.csr.row_ptr) {
        return;
    }

    const uint64_t total =
        view->meta.csr.row_ptr[
            view->meta.csr.nrows
        ];

    run_twopass_omp(
        view->buffer->pool,
        total,
        test_case->num_threads,
        test_case,
        profiler,
        metric
    );
}


#endif /* OPENMP_HAS_TWO_PASS_SCAN */
