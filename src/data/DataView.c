#include "data/DataView.h"
#include "data/views/ViewShapeFns.h"
#include <assert.h>

static void view_compute_meta(DataView *view) {
    if (!view || view->window.size == 0) return;

    switch (view->type) {
        case VIEW_1D:    shape_1d_recompute(view); break;
        case VIEW_2D:    shape_2d_recompute(view); break;
        case VIEW_3D:    shape_3d_recompute(view); break;
        case VIEW_AOS:   shape_aos_recompute(view); break;
        case VIEW_SOA:   shape_soa_recompute(view); break;
        case VIEW_AOSOA: shape_aosoa_recompute(view); break;
        case VIEW_CSR:   shape_csr_recompute(view); break;
        default: break;
    }
}

DataView create_data_view(
    DataBuffer *buffer,
    ViewType type,
    const ViewShapeConfig *shape_cfg,
    uint64_t offset,
    uint64_t size
) {
    assert(buffer != NULL);
    assert(offset + size <= buffer->size);

    DataView view = {0};
    view.buffer = buffer;
    view.type   = type;
    view.window.offset = offset;
    view.window.size   = size;

    if (shape_cfg) {
        switch (type) {
            case VIEW_2D:
                view.meta.v2d.cols = shape_cfg->v2d.cols;
                break;
            case VIEW_3D:
                view.meta.v3d.depth = shape_cfg->v3d.depth;
                view.meta.v3d.cols  = shape_cfg->v3d.cols;
                break;
            case VIEW_AOS:
                view.meta.aos.struct_size = shape_cfg->aos.struct_size;
                break;
            case VIEW_SOA:
                view.meta.soa.num_fields = shape_cfg->soa.num_fields;
                break;
            case VIEW_AOSOA:
                view.meta.aosoa.vector_length = shape_cfg->aosoa.vector_length;
                view.meta.aosoa.num_fields    = shape_cfg->aosoa.num_fields;
                break;
            default:
                break;
        }
    }

    view_compute_meta(&view);
    return view;
}

DataView create_data_view_csr(
    DataBuffer *buffer,
    uint64_t offset,
    uint64_t size,
    uint64_t nrows,
    uint64_t ncols,
    const uint64_t *row_ptr,
    const uint64_t *col_ind
) {
    assert(buffer != NULL);
    assert(offset + size <= buffer->size);

    DataView view = {0};
    view.buffer = buffer;
    view.type   = VIEW_CSR;
    view.window.offset = offset;
    view.window.size   = size;

    view.meta.csr.nrows   = nrows;
    view.meta.csr.ncols   = ncols;
    view.meta.csr.row_ptr = row_ptr;
    view.meta.csr.col_ind = col_ind;

    return view;
}