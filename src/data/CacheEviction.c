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
 * @file CacheEviction.c
 * @brief Allocation, traversal, and cleanup of a cache-perturbation buffer.
 *
 * Implementation details are kept separate from the public API contracts
 * documented in the corresponding header.
 */

#include "data/CacheEviction.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int cache_eviction_init(
    CacheEviction *ctx,
    size_t llc_bytes,
    size_t multiplier,
    size_t stride_bytes
)
{
    size_t i, size_bytes;

    if (ctx == NULL ||
        llc_bytes == 0U ||
        multiplier == 0U ||
        stride_bytes == 0U ||
        llc_bytes > SIZE_MAX / multiplier) {
        return -1;
    }

    memset(ctx, 0, sizeof(*ctx));
    size_bytes  = llc_bytes * multiplier;
    ctx->buffer = (unsigned char *)malloc(size_bytes);

    if (ctx->buffer == NULL) {
        return -1;
    }

    ctx->size_bytes   = size_bytes;
    ctx->stride_bytes = stride_bytes;

    for (i = 0U; i < size_bytes; ++i) {
        ctx->buffer[i] =
            (unsigned char)(((i * 13U) + 7U) & 255U);
    }
    return 0;
}

void cache_eviction_run(CacheEviction *ctx) {
    size_t i;
    uint64_t sum = 0U;

    if (ctx == NULL ||
        ctx->buffer == NULL ||
        ctx->stride_bytes == 0U) {
        return;
    }

    for (i = 0U; i < ctx->size_bytes; i += ctx->stride_bytes) {
        sum += ctx->buffer[i];
    }

    ctx->checksum = sum;
}

void cache_eviction_destroy(CacheEviction *ctx)
{
    if (ctx == NULL) {
        return;
    }
    free(ctx->buffer);
    memset(ctx, 0, sizeof(*ctx));
}