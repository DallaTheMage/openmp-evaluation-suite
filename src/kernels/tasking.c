#include "kernels/tasking.h"
#include "profiling/Profiler.h"

#if OPENMP_HAS_3_0

void kernel_tasking_run(DataView *view, LaunchConfig *config) {
    if (!ctx || !ctx->data) return;

    const uint16_t num_threads = ctx->current_point.threadnumber;
    const uint32_t chunk       = ctx->current_point.chunksize;
    double *restrict pool      = ctx->data->buffer->pool;
    const bool measure         = ctx->is_measuring;

    switch (ctx->data->type) {
        case VIEW_1D: {
            const uint64_t dim0 = ctx->data->meta.v1d.dim0;
            const uint32_t task_size = (chunk > 0) ? chunk : 64;

            if (measure) profiler_start(ctx->profiler);
            #pragma omp parallel num_threads(num_threads) default(none) shared(pool, dim0, task_size)
            {
                #pragma omp single
                {
                    for (uint64_t i = 0; i < dim0; i += task_size) {
                        uint64_t end = (i + task_size < dim0) ? (i + task_size) : dim0;
                        #pragma omp task firstprivate(i, end) shared(pool)
                        {
                            for (uint64_t k = i; k < end; ++k) {
                                pool[k] = pool[k] * 2.0 + 1.0;
                            }
                        }
                    }
                }
            }
            if (measure) profiler_stop(ctx->profiler, &ctx->last_metric);
            break;
        }
        case VIEW_2D: {
            const uint64_t rows = ctx->data->meta.v2d.rows;
            const uint64_t cols = ctx->data->meta.v2d.cols;

            if (measure) profiler_start(ctx->profiler);
            #pragma omp parallel num_threads(num_threads) default(none) shared(pool, rows, cols)
            {
                #pragma omp single
                {
                    for (uint64_t r = 0; r < rows; ++r) {
                        #pragma omp task firstprivate(r) shared(pool, cols)
                        {
                            uint64_t row_offset = r * cols;
                            for (uint64_t c = 0; c < cols; ++c) {
                                pool[row_offset + c] = pool[row_offset + c] * 2.0 + 1.0;
                            }
                        }
                    }
                }
            }
            if (measure) profiler_stop(ctx->profiler, &ctx->last_metric);
            break;
        }
        case VIEW_3D: {
            const uint64_t rows  = ctx->data->meta.v3d.rows;
            const uint64_t cols  = ctx->data->meta.v3d.cols;
            const uint64_t depth = ctx->data->meta.v3d.depth;

            if (measure) profiler_start(ctx->profiler);
            #pragma omp parallel num_threads(num_threads) default(none) shared(pool, rows, cols, depth)
            {
                #pragma omp single
                {
                    for (uint64_t r = 0; r < rows; ++r) {
                        #pragma omp task firstprivate(r) shared(pool, cols, depth)
                        {
                            for (uint64_t c = 0; c < cols; ++c) {
                                uint64_t offset = (r * cols + c) * depth;
                                for (uint64_t d = 0; d < depth; ++d) {
                                    pool[offset + d] = pool[offset + d] * 2.0 + 1.0;
                                }
                            }
                        }
                    }
                }
            }
            if (measure) profiler_stop(ctx->profiler, &ctx->last_metric);
            break;
        }
        case VIEW_CSR: {
            const uint64_t nrows             = ctx->data->meta.csr.nrows;
            const uint64_t *restrict row_ptr = ctx->data->meta.csr.row_ptr;
            const uint64_t *restrict col_ind = ctx->data->meta.csr.col_ind;
            const double   *restrict val     = ctx->data->buffer->pool;
            double         *restrict y       = ctx->data->buffer->pool;

            if (measure) profiler_start(ctx->profiler);
            #pragma omp parallel num_threads(num_threads) default(none) shared(nrows, row_ptr, col_ind, val, y)
            {
                #pragma omp single
                {
                    for (uint64_t r = 0; r < nrows; ++r) {
                        #pragma omp task firstprivate(r) shared(row_ptr, col_ind, val, y)
                        {
                            double sum = 0.0;
                            uint64_t start = row_ptr[r];
                            uint64_t end   = row_ptr[r + 1];
                            for (uint64_t idx = start; idx < end; ++idx) {
                                sum += val[idx] * val[col_ind[idx]];
                            }
                            y[r] = sum;
                        }
                    }
                }
            }
            if (measure) profiler_stop(ctx->profiler, &ctx->last_metric);
            break;
        }
        case VIEW_AOS: {
            const uint64_t num_elems   = ctx->data->meta.aos.num_elements;
            const uint64_t struct_size = ctx->data->meta.aos.struct_size;
            const uint32_t task_size   = (chunk > 0) ? chunk : 64;

            if (measure) profiler_start(ctx->profiler);
            #pragma omp parallel num_threads(num_threads) default(none) shared(pool, num_elems, struct_size, task_size)
            {
                #pragma omp single
                {
                    for (uint64_t i = 0; i < num_elems; i += task_size) {
                        uint64_t end = (i + task_size < num_elems) ? (i + task_size) : num_elems;
                        #pragma omp task firstprivate(i, end) shared(pool, struct_size)
                        {
                            for (uint64_t k = i; k < end; ++k) {
                                uint64_t base = k * struct_size;
                                for (uint64_t f = 0; f < struct_size; ++f) {
                                    pool[base + f] = pool[base + f] * 2.0 + 1.0;
                                }
                            }
                        }
                    }
                }
            }
            if (measure) profiler_stop(ctx->profiler, &ctx->last_metric);
            break;
        }
        case VIEW_SOA: {
            const uint64_t num_elems  = ctx->data->meta.soa.num_elements;
            const uint64_t num_fields = ctx->data->meta.soa.num_fields;

            if (measure) profiler_start(ctx->profiler);
            #pragma omp parallel num_threads(num_threads) default(none) shared(pool, num_elems, num_fields)
            {
                #pragma omp single
                {
                    for (uint64_t f = 0; f < num_fields; ++f) {
                        #pragma omp task firstprivate(f) shared(pool, num_elems)
                        {
                            uint64_t field_offset = f * num_elems;
                            for (uint64_t i = 0; i < num_elems; ++i) {
                                pool[field_offset + i] = pool[field_offset + i] * 2.0 + 1.0;
                            }
                        }
                    }
                }
            }
            if (measure) profiler_stop(ctx->profiler, &ctx->last_metric);
            break;
        }
        case VIEW_AOSOA: {
            const uint64_t num_elems  = ctx->data->meta.aosoa.num_elements;
            const uint64_t num_fields = ctx->data->meta.aosoa.num_fields;
            const uint64_t vl         = ctx->data->meta.aosoa.vector_length;
            const uint64_t num_tiles  = (vl > 0) ? (num_elems / vl) : 0;
            const uint64_t tile_size  = num_fields * vl;

            if (measure) profiler_start(ctx->profiler);
            #pragma omp parallel num_threads(num_threads) default(none) shared(pool, num_tiles, num_fields, vl, tile_size)
            {
                #pragma omp single
                {
                    for (uint64_t t = 0; t < num_tiles; ++t) {
                        #pragma omp task firstprivate(t) shared(pool, num_fields, vl, tile_size)
                        {
                            uint64_t tile_base = t * tile_size;
                            for (uint64_t f = 0; f < num_fields; ++f) {
                                uint64_t field_base = tile_base + (f * vl);
                                for (uint64_t v = 0; v < vl; ++v) {
                                    pool[field_base + v] = pool[field_base + v] * 2.0 + 1.0;
                                }
                            }
                        }
                    }
                }
            }
            if (measure) profiler_stop(ctx->profiler, &ctx->last_metric);
            break;
        }
        default: break;
    }
}

#endif /* OPENMP_HAS_3_0 */