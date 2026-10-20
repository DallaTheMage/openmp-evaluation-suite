/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (C) 2026 Lorenzo Dall'Ara
 *
 * This file is part of the OpenMP compiler-agnostic benchmark suite,
 * developed as part of a Bachelor's thesis at the
 * Alma Mater Studiorum - University of Bologna.
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
 * @file CacheEviction.h
 * @brief Public interface for controlled cache-residency perturbation.
 *
 * The buffer is traversed at a configurable stride to perturb cache residency.
 * The operation does not guarantee eviction of any specific cache line.
 */

#ifndef DATA_CACHE_EVICTION_H
#define DATA_CACHE_EVICTION_H

#include <stddef.h>
#include <stdint.h>

/** @brief Buffer and traversal state used for cache perturbation. */
typedef struct CacheEviction {
    unsigned char *buffer;
    size_t size_bytes;
    size_t stride_bytes;
    uint64_t checksum;
} CacheEviction;

/**
 * @brief Allocate and initialize the cache-perturbation buffer.
 * @param ctx Context to initialize.
 * @param llc_bytes Detected last-level cache size in bytes.
 * @param multiplier Buffer size relative to the LLC size.
 * @param stride_bytes Distance between consecutive buffer accesses.
 * @return 0 on success, -1 on failure.
 */
int cache_eviction_init(
    CacheEviction *ctx,
    size_t llc_bytes,
    size_t multiplier,
    size_t stride_bytes
);

/**
 * @brief Traverse the buffer to perturb cache residency.
 * @param ctx Initialized cache-eviction context.
 * @note Complete eviction of cached data is not guaranteed.
 */
void cache_eviction_run(CacheEviction *ctx);

/** @brief Release allocated resources.
 * @param ctx Context to destroy.
 */
void cache_eviction_destroy(CacheEviction *ctx);

#endif