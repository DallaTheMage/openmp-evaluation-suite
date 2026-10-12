#include "kernels/scan/native.h"
#include "config/openmp.h"
#include "profiling/Profiler.h"
#include "core/TestPlan.h"

#if OPENMP_HAS_NATIVE_SCAN
/* ============================================================================
 * VIEW_2D
 * ========================================================================== */
KernelStatus kernel_2d_native_scan(DataView *view, const TestCase *test_case, Profiler *profiler, PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t total       = view->meta.v2d.rows * view->meta.v2d.cols;
    double sum                 = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, total, chunk) \
        reduction(inscan, +:sum)
    {
        #pragma omp for schedule(static, chunk)
        for (uint64_t i = 0; i < total; ++i) {
            sum += pool[i];
            #pragma omp scan inclusive(sum)
            pool[i] = sum;
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* ============================================================================
 * VIEW_3D
 * ========================================================================== */
KernelStatus kernel_3d_native_scan(DataView *view, const TestCase *test_case, Profiler *profiler, PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t total       = view->meta.v3d.height * view->meta.v3d.cols * view->meta.v3d.depth;
    double sum                 = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, total, chunk) \
        reduction(inscan, +:sum)
    {
        #pragma omp for schedule(static, chunk)
        for (uint64_t i = 0; i < total; ++i) {
            sum += pool[i];
            #pragma omp scan inclusive(sum)
            pool[i] = sum;
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* ============================================================================
 * VIEW_AOS
 * ========================================================================== */

KernelStatus kernel_aos_native_scan(DataView *view, const TestCase *test_case, Profiler *profiler, PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t total       = view->meta.aos.num_structs * view->meta.aos.struct_size;
    double sum                 = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, total, chunk) \
        reduction(inscan, +:sum)
    {
        #pragma omp for schedule(static, chunk)
        for (uint64_t i = 0; i < total; ++i) {
            sum += pool[i];
            #pragma omp scan inclusive(sum)
            pool[i] = sum;
        }
    }
    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}

/* ============================================================================
 * VIEW_SOA
 * ========================================================================== */
KernelStatus kernel_soa_native_scan(DataView *view, const TestCase *test_case, Profiler *profiler, PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t total       = view->meta.soa.field_length * view->meta.soa.num_fields;
    double sum                 = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, total, chunk) \
        reduction(inscan, +:sum)
    {
        #pragma omp for schedule(static, chunk)
        for (uint64_t i = 0; i < total; ++i) {
            sum += pool[i];
            #pragma omp scan inclusive(sum)
            pool[i] = sum;
        }
    }
    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* ============================================================================
 * VIEW_AOSOA
 * ========================================================================== */
KernelStatus kernel_aosoa_native_scan(DataView *view, const TestCase *test_case, Profiler *profiler, PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t vl          = view->meta.aosoa.vector_length;
    if (vl == 0) {
        return KERNEL_STATUS_INVALID_ARGUMENT;
    }
    const uint64_t total = view->meta.aosoa.num_blocks * view->meta.aosoa.num_fields * vl;
    double sum           = 0.0;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel num_threads(num_threads) \
        default(none) shared(pool, total, chunk) \
        reduction(inscan, +:sum)
    {
        #pragma omp for schedule(static, chunk)
        for (uint64_t i = 0; i < total; ++i) {
            sum += pool[i];
            #pragma omp scan inclusive(sum)
            pool[i] = sum;
        }
    }
    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}
#endif /* OPENMP_HAS_NATIVE_SCAN */