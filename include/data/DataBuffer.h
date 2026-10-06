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
 * @file DataBuffer.h
 * @brief Owning contiguous storage for benchmark data.
 *
 * DataBuffer owns a contiguous pool of double-precision elements.
 *
 * The buffer is deliberately independent from the global Configuration
 * object. Its creation requires only the information that is actually
 * relevant to the allocation:
 *
 * - number of elements;
 * - required alignment.
 *
 * This keeps the data layer independent from the benchmark configuration
 * layer and avoids unnecessary coupling between components.
 *
 * DataView objects may reference a DataBuffer, but do not own it.
 */

#ifndef DATA_BUFFER_H
#define DATA_BUFFER_H

#include <stddef.h>


/* ========================================================================= */
/* DataBuffer                                                                */
/* ========================================================================= */

/**
 * @brief Owning contiguous buffer of double-precision elements.
 *
 * The memory referenced by pool is owned by the DataBuffer and remains valid
 * until destroy_data_buffer() is called.
 *
 * DataBuffer is not copy-owning: copying the structure itself does not create
 * an independent allocation.
 */
typedef struct DataBuffer {
    /**
     * @brief Pointer to the first element of the allocated pool.
     *
     * The pointer is NULL when no storage is allocated.
     */
    double *pool;

    /**
     * @brief Number of double-precision elements in the pool.
     */
    size_t element_count;

    /**
     * @brief Alignment of the allocated pool in bytes.
     */
    size_t alignment;
} DataBuffer;


/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/**
 * @brief Allocate and initialize a data buffer.
 *
 * The allocation contains exactly element_count double-precision elements
 * and is aligned according to the requested alignment.
 *
 * A zero element_count is allowed and results in a valid DataBuffer object
 * with no allocated element storage.
 *
 * @param element_count Number of double elements to allocate.
 * @param alignment Required alignment in bytes.
 *
 * @return Pointer to a newly allocated DataBuffer, or NULL if allocation
 *         fails or the requested allocation parameters are invalid.
 */
DataBuffer *create_data_buffer(
    size_t element_count,
    size_t alignment
);


/**
 * @brief Destroy a data buffer and release its owned storage.
 *
 * Passing NULL is allowed and has no effect.
 *
 * @param buffer Buffer to destroy.
 */
void destroy_data_buffer(
    DataBuffer *buffer
);


#endif /* DATA_BUFFER_H */
