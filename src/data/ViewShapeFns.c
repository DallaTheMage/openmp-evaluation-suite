#include "data/views/ViewShapeFns.h"
#include "data/DataView.h"

#include <stdint.h>
#include <stdbool.h>


/* ========================================================================= */
/* --- 1D Shape ------------------------------------------------------------ */
/* ========================================================================= */

void shape_1d_recompute(DataView *view)
{
    /*
     * 1D has no derived shape parameters.
     *
     * The window itself already represents the valid 1D extent.
     */
    (void)view;
}

bool shape_1d_validate(const DataView *view)
{
    /*
     * A 1D view is always valid as long as the DataView itself exists.
     */
    (void)view;
    return true;
}


/* ========================================================================= */
/* --- 2D Shape ------------------------------------------------------------ */
/* ========================================================================= */

void shape_2d_recompute(DataView *view)
{
    uint64_t cols = view->meta.v2d.cols;

    if (cols == 0) {
        view->meta.v2d.rows = 0;
        return;
    }

    /*
     * V1 policy:
     *
     *     cols = fixed/configured dimension
     *     rows = derived from the current window
     *
     * The remainder, if any, is intentionally unused.
     */
    view->meta.v2d.rows =
        view->window.size / cols;
}

bool shape_2d_validate(const DataView *view)
{
    uint64_t cols = view->meta.v2d.cols;
    uint64_t rows = view->meta.v2d.rows;

    if (cols == 0)
        return false;

    /*
     * Use division instead of:
     *
     *     rows * cols <= window.size
     *
     * to avoid possible unsigned multiplication overflow.
     */
    return rows <= view->window.size / cols;
}


/* ========================================================================= */
/* --- 3D Shape ------------------------------------------------------------ */
/* ========================================================================= */

void shape_3d_recompute(DataView *view)
{
    uint64_t depth = view->meta.v3d.depth;
    uint64_t cols  = view->meta.v3d.cols;

    if (depth == 0 ||
        cols == 0 ||
        depth > UINT64_MAX / cols) {

        view->meta.v3d.height = 0;
        return;
    }

    uint64_t plane_size = depth * cols;

    /*
     * V1 policy:
     *
     *     depth  = fixed/configured dimension
     *     cols   = fixed/configured dimension
     *     height = derived from the current window
     *
     * Any remainder is intentionally unused.
     */
    view->meta.v3d.height =
        view->window.size / plane_size;
}

bool shape_3d_validate(const DataView *view)
{
    uint64_t depth  = view->meta.v3d.depth;
    uint64_t cols   = view->meta.v3d.cols;
    uint64_t height = view->meta.v3d.height;

    if (depth == 0 ||
        cols == 0 ||
        depth > UINT64_MAX / cols) {

        return false;
    }

    uint64_t plane_size = depth * cols;

    /*
     * Avoid:
     *
     *     height * plane_size <= window.size
     *
     * because the multiplication may overflow.
     */
    return height <= view->window.size / plane_size;
}


/* --- CSR Shape --- */

void shape_csr_recompute(DataView *view)
{
    /*
     * V1 CSR convention:
     *
     *     window.size == logical nnz
     *
     * Therefore the logical CSR size does not need to be derived
     * from another quantity: it is already represented directly
     * by the current window.
     *
     * The actual CSR topology (row_ptr / col_ind) is generated
     * separately by CSRTopology.
     */
    (void)view;
}

bool shape_csr_validate(const DataView *view)
{
    /*
     * A CSR view is valid if its logical nnz does not exceed
     * the current window.
     *
     * Since window.size itself represents nnz in V1, this is
     * normally guaranteed by construction.
     */
    return view != NULL;
}


/* ========================================================================= */
/* --- AoS Shape ----------------------------------------------------------- */
/* ========================================================================= */

void shape_aos_recompute(DataView *view)
{
    uint64_t struct_size = view->meta.aos.struct_size;

    if (struct_size == 0) {
        view->meta.aos.num_structs = 0;
        return;
    }

    /*
     * V1 policy:
     *
     *     struct_size = fixed/configured
     *     num_structs = derived from the current window
     *
     * Any incomplete trailing struct is intentionally unused.
     */
    view->meta.aos.num_structs =
        view->window.size / struct_size;
}

bool shape_aos_validate(const DataView *view)
{
    uint64_t struct_size = view->meta.aos.struct_size;
    uint64_t num_structs = view->meta.aos.num_structs;

    if (struct_size == 0)
        return false;

    /*
     * Avoid multiplication overflow.
     */
    return num_structs <=
           view->window.size / struct_size;
}


/* ========================================================================= */
/* --- SoA Shape ----------------------------------------------------------- */
/* ========================================================================= */

void shape_soa_recompute(DataView *view)
{
    uint64_t num_fields = view->meta.soa.num_fields;

    if (num_fields == 0) {
        view->meta.soa.field_len = 0;
        return;
    }

    /*
     * V1 policy:
     *
     *     num_fields = fixed/configured
     *     field_len  = derived from the current window
     *
     * The window is conceptually divided equally among all fields.
     * Any remainder is intentionally unused.
     */
    view->meta.soa.field_len =
        view->window.size / num_fields;
}

bool shape_soa_validate(const DataView *view)
{
    uint64_t num_fields = view->meta.soa.num_fields;
    uint64_t field_len  = view->meta.soa.field_len;

    if (num_fields == 0)
        return false;

    /*
     * Avoid multiplication overflow.
     */
    return field_len <=
           view->window.size / num_fields;
}


/* ========================================================================= */
/* --- AoSoA Shape --------------------------------------------------------- */
/* ========================================================================= */

void shape_aosoa_recompute(DataView *view)
{
    uint64_t num_fields    = view->meta.aosoa.num_fields;
    uint64_t vector_length = view->meta.aosoa.vector_length;

    if (num_fields == 0 ||
        vector_length == 0 ||
        num_fields > UINT64_MAX / vector_length) {

        view->meta.aosoa.num_blocks = 0;
        return;
    }

    uint64_t block_size =
        num_fields * vector_length;

    /*
     * V1 policy:
     *
     *     num_fields    = fixed/configured
     *     vector_length = fixed/configured
     *     num_blocks    = derived from the current window
     *
     * Any incomplete trailing block is intentionally unused.
     */
    view->meta.aosoa.num_blocks =
        view->window.size / block_size;
}

bool shape_aosoa_validate(const DataView *view)
{
    uint64_t num_fields =
        view->meta.aosoa.num_fields;

    uint64_t vector_length =
        view->meta.aosoa.vector_length;

    uint64_t num_blocks =
        view->meta.aosoa.num_blocks;

    if (num_fields == 0 ||
        vector_length == 0 ||
        num_fields > UINT64_MAX / vector_length) {

        return false;
    }

    uint64_t block_size =
        num_fields * vector_length;

    /*
     * Avoid:
     *
     *     num_blocks * block_size <= window.size
     *
     * because the multiplication may overflow.
     */
    return num_blocks <=
           view->window.size / block_size;
}
