/*
 * Copyright (C) 2026
 *
 * This file is part of the OpenMP compiler-agnostic benchmark suite.
 *
 * The OpenMP Benchmark Suite is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * The OpenMP Benchmark Suite is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the OpenMP Benchmark Suite. If not, see
 * <https://www.gnu.org/licenses/>.
 */

/**
 * @file ViewShapeFns.c
 * @brief Shape computation and validation for DataView layouts.
 *
 * Shape computation is performed during benchmark setup. No function in
 * this module allocates memory, performs I/O or belongs in a benchmark
 * measurement region.
 */

#include "data/views/ViewShapeFns.h"

#include <stdint.h>


/* ========================================================================= */
/* --- 1D ------------------------------------------------------------------ */
/* ========================================================================= */

void shape_1d_recompute(DataView *view)
{
    if (view == NULL) {
        return;
    }

    view->meta.v1d.length = view->window.size;
}


bool shape_1d_validate(const DataView *view)
{
    if (view == NULL || view->type != VIEW_1D) {
        return false;
    }

    return view->meta.v1d.length == view->window.size;
}


/* ========================================================================= */
/* --- 2D ------------------------------------------------------------------ */
/* ========================================================================= */

void shape_2d_recompute(DataView *view)
{
    uint64_t cols;

    if (view == NULL) {
        return;
    }

    cols = view->meta.v2d.cols;

    if (cols == 0U) {
        view->meta.v2d.rows = 0U;
        return;
    }

    view->meta.v2d.rows = view->window.size / cols;
}


bool shape_2d_validate(const DataView *view)
{
    uint64_t rows;
    uint64_t cols;

    if (view == NULL || view->type != VIEW_2D) {
        return false;
    }

    rows = view->meta.v2d.rows;
    cols = view->meta.v2d.cols;

    if (cols == 0U) {
        return false;
    }

    /*
     * The shape represents complete rows only. The unused remainder,
     * if any, is intentionally outside the logical 2D shape.
     */
    return rows == view->window.size / cols;
}


/* ========================================================================= */
/* --- 3D ------------------------------------------------------------------ */
/* ========================================================================= */

void shape_3d_recompute(DataView *view)
{
    uint64_t depth;
    uint64_t cols;
    uint64_t plane_size;

    if (view == NULL) {
        return;
    }

    depth = view->meta.v3d.depth;
    cols = view->meta.v3d.cols;

    if (depth == 0U ||
        cols == 0U ||
        depth > UINT64_MAX / cols) {

        view->meta.v3d.height = 0U;
        return;
    }

    plane_size = depth * cols;
    view->meta.v3d.height =
        view->window.size / plane_size;
}


bool shape_3d_validate(const DataView *view)
{
    uint64_t height;
    uint64_t depth;
    uint64_t cols;
    uint64_t plane_size;

    if (view == NULL || view->type != VIEW_3D) {
        return false;
    }

    height = view->meta.v3d.height;
    depth = view->meta.v3d.depth;
    cols = view->meta.v3d.cols;

    if (depth == 0U ||
        cols == 0U ||
        depth > UINT64_MAX / cols) {

        return false;
    }

    plane_size = depth * cols;

    return height == view->window.size / plane_size;
}


/* ========================================================================= */
/* --- AoS ----------------------------------------------------------------- */
/* ========================================================================= */

void shape_aos_recompute(DataView *view)
{
    uint64_t struct_size;

    if (view == NULL) {
        return;
    }

    struct_size = view->meta.aos.struct_size;

    if (struct_size == 0U) {
        view->meta.aos.num_structs = 0U;
        return;
    }

    view->meta.aos.num_structs =
        view->window.size / struct_size;
}


bool shape_aos_validate(const DataView *view)
{
    uint64_t struct_size;

    if (view == NULL || view->type != VIEW_AOS) {
        return false;
    }

    struct_size = view->meta.aos.struct_size;

    if (struct_size == 0U) {
        return false;
    }

    return view->meta.aos.num_structs ==
           view->window.size / struct_size;
}


/* ========================================================================= */
/* --- SoA ----------------------------------------------------------------- */
/* ========================================================================= */

void shape_soa_recompute(DataView *view)
{
    uint64_t num_fields;

    if (view == NULL) {
        return;
    }

    num_fields = view->meta.soa.num_fields;

    if (num_fields == 0U) {
        view->meta.soa.field_length = 0U;
        return;
    }

    view->meta.soa.field_length =
        view->window.size / num_fields;
}


bool shape_soa_validate(const DataView *view)
{
    uint64_t num_fields;

    if (view == NULL || view->type != VIEW_SOA) {
        return false;
    }

    num_fields = view->meta.soa.num_fields;

    if (num_fields == 0U) {
        return false;
    }

    return view->meta.soa.field_length ==
           view->window.size / num_fields;
}


/* ========================================================================= */
/* --- AoSoA ---------------------------------------------------------------- */
/* ========================================================================= */

void shape_aosoa_recompute(DataView *view)
{
    uint64_t num_fields;
    uint64_t vector_length;
    uint64_t block_size;

    if (view == NULL) {
        return;
    }

    num_fields = view->meta.aosoa.num_fields;
    vector_length = view->meta.aosoa.vector_length;

    if (num_fields == 0U ||
        vector_length == 0U ||
        num_fields > UINT64_MAX / vector_length) {

        view->meta.aosoa.num_blocks = 0U;
        return;
    }

    block_size = num_fields * vector_length;

    view->meta.aosoa.num_blocks =
        view->window.size / block_size;
}


bool shape_aosoa_validate(const DataView *view)
{
    uint64_t num_fields;
    uint64_t vector_length;
    uint64_t block_size;

    if (view == NULL || view->type != VIEW_AOSOA) {
        return false;
    }

    num_fields = view->meta.aosoa.num_fields;
    vector_length = view->meta.aosoa.vector_length;

    if (num_fields == 0U ||
        vector_length == 0U ||
        num_fields > UINT64_MAX / vector_length) {

        return false;
    }

    block_size = num_fields * vector_length;

    return view->meta.aosoa.num_blocks ==
           view->window.size / block_size;
}