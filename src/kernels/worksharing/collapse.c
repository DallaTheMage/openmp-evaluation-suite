#include "kernels/worksharing/collapse.h"
#include "config/openmp.h"
#include "profiling/Profiler.h"
#include "core/TestPlan.h"

#if OPENMP_HAS_3_0
/* ============================================================================
- VIEW_2D
- ========================================================================== */
KernelStatus kernel_2d_collapse(DataView *view, const TestCase *test_case, Profiler *profiler, PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t rows        = view->meta.v2d.rows;
    const uint64_t cols        = view->meta.v2d.cols;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel for \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        collapse(2)
    for (uint64_t r = 0; r < rows; ++r) {
        for (uint64_t c = 0; c < cols; ++c) {
            const uint64_t offset = r * cols + c;
            pool[offset] = pool[offset] * 2.0 + 1.0;
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}


/* ============================================================================
- VIEW_3D
- ========================================================================== */
KernelStatus kernel_3d_collapse(DataView *view, const TestCase *test_case, Profiler *profiler, PerformanceMetric *metric) {
    const uint16_t num_threads = test_case->num_threads;
    const uint32_t chunk       = test_case->chunk_size;
    double *restrict pool      = view->buffer->pool;
    const uint64_t rows        = view->meta.v3d.height;
    const uint64_t cols        = view->meta.v3d.cols;
    const uint64_t depth       = view->meta.v3d.depth;
    if (profiler_start(profiler) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }

    #pragma omp parallel for \
        num_threads(num_threads) \
        schedule(CHOSEN_SCHEDULE, chunk) \
        collapse(3)
    for (uint64_t r = 0; r < rows; ++r) {
        for (uint64_t c = 0; c < cols; ++c) {
            for (uint64_t d = 0; d < depth; ++d) {
                const uint64_t offset = (r * cols + c) * depth + d;
                pool[offset] = pool[offset] * 2.0 + 1.0;
            }
        }
    }

    if (profiler_stop(profiler, metric) != 0) {
        return KERNEL_STATUS_PROFILER_FAILURE;
    }
    return KERNEL_STATUS_OK;
}
#endif /* OPENMP_HAS_3_0 */