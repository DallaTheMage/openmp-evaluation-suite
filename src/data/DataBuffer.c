/*
 * Copyright (C) 2026
 *
 * This file is part of the OpenMP compiler-agnostic benchmark suite.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file DataBuffer.c
 * @brief Owning contiguous storage for benchmark data.
 *
 * This implementation deliberately depends only on the DataBuffer contract.
 * The data layer must remain independent from the global benchmark
 * Configuration object.
 *
 * The allocation uses POSIX posix_memalign() to provide explicit alignment,
 * which is important for vectorized and memory-bandwidth-sensitive kernels.
 */

#define _POSIX_C_SOURCE 200112L

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "data/DataBuffer.h"


/* ========================================================================= */
/* --- Internal helpers --------------------------------------------------- */
/* ========================================================================= */

/**
 * @brief Check whether an alignment value is valid for posix_memalign().
 *
 * POSIX requires the alignment to be:
 * - a power of two;
 * - a multiple of sizeof(void *).
 *
 * @param alignment Requested alignment in bytes.
 *
 * @return Non-zero if the alignment is valid, zero otherwise.
 */
static int is_valid_alignment(size_t alignment)
{
    if (alignment < sizeof(void *)) {
        return 0;
    }

    /*
     * A power of two has exactly one bit set.
     */
    if ((alignment & (alignment - 1U)) != 0U) {
        return 0;
    }

    return 1;
}


/* ========================================================================= */
/* --- Lifecycle ---------------------------------------------------------- */
/* ========================================================================= */

DataBuffer *create_data_buffer(
    size_t element_count,
    size_t alignment
)
{
    DataBuffer *buffer;
    size_t allocation_size;

    if (!is_valid_alignment(alignment)) {
        return NULL;
    }

    /*
     * Protect the multiplication below from wrapping around size_t.
     */
    if (element_count > SIZE_MAX / sizeof(double)) {
        return NULL;
    }

    buffer = malloc(sizeof(*buffer));

    if (buffer == NULL) {
        return NULL;
    }

    buffer->pool = NULL;
    buffer->element_count = element_count;
    buffer->alignment = alignment;

    /*
     * A zero-sized buffer is a valid object according to the public
     * contract, but there is no storage to allocate.
     */
    if (element_count == 0U) {
        return buffer;
    }

    allocation_size = element_count * sizeof(double);

    if (posix_memalign(
            (void **)&buffer->pool,
            alignment,
            allocation_size) != 0) {
        free(buffer);
        return NULL;
    }

    return buffer;
}


void destroy_data_buffer(
    DataBuffer *buffer
)
{
    if (buffer == NULL) {
        return;
    }

    free(buffer->pool);
    free(buffer);
}