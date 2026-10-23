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
 * @file MemoryManager.c
 * @brief Implementation of benchmark buffer ownership and data restoration.
 */

#include <stddef.h>
#include <stdlib.h>
#include <omp.h>

#include "data/Generator.h"
#include "data/MemoryManager.h"

#define MEMORY_MANAGER_PARALLEL_THRESHOLD_ELEMENTS ((size_t)262144U)
#define MEMORY_MANAGER_MAX_COPY_THREADS 8

struct MemoryManager {
    DataBuffer *original;
    DataBuffer *working;
};

/**
 * @brief Determine a bounded thread count for memory preparation.
 *
 * @return A positive thread count no greater than the configured internal cap.
 */
static int memory_manager_copy_thread_count(void) {
    int available = omp_get_num_procs();

    if (available < 1) {
        available = 1;
    }
    if (available > MEMORY_MANAGER_MAX_COPY_THREADS) {
        available = MEMORY_MANAGER_MAX_COPY_THREADS;
    }

    return available;
}

MemoryManager *memory_manager_create(
    size_t original_element_count,
    size_t alignment,
    uint64_t seed) {
    MemoryManager *manager;

    manager = (MemoryManager *)calloc(1U, sizeof(*manager));
    if (manager == NULL) {
        return NULL;
    }

    manager->original = create_data_buffer(
        original_element_count,
        alignment
    );

    if (manager->original == NULL) {
        free(manager);
        return NULL;
    }

    generator_fill_pool(
        seed,
        manager->original->pool,
        manager->original->element_count
    );

    return manager;
}

DataBuffer *memory_manager_original_buffer(MemoryManager *manager) {
    return manager != NULL ? manager->original : NULL;
}

DataBuffer *memory_manager_work_buffer(MemoryManager *manager) {
    return manager != NULL ? manager->working : NULL;
}

int memory_manager_reserve_work(
    MemoryManager *manager,
    size_t required_elements) {
    DataBuffer *replacement;

    if (manager == NULL ||
        manager->original == NULL ||
        required_elements > manager->original->element_count) {
        return -1;
    }

    if (manager->working != NULL &&
        manager->working->element_count >= required_elements) {
        return 0;
    }

    replacement = create_data_buffer(
        required_elements,
        manager->original->alignment
    );

    if (replacement == NULL) {
        return -1;
    }

    destroy_data_buffer(manager->working);
    manager->working = replacement;

    return 0;
}

int memory_manager_restore_window(
    MemoryManager *manager,
    uint64_t offset,
    size_t element_count) {
    size_t i;
    size_t source_offset;

    if (manager == NULL ||
        manager->original == NULL ||
        manager->working == NULL ||
        offset > (uint64_t)SIZE_MAX) {
        return -1;
    }

    source_offset = (size_t)offset;

    if (source_offset > manager->original->element_count ||
        element_count > (manager->original->element_count - source_offset) ||
        element_count > manager->working->element_count) {
        return -1;
    }

    if (element_count < MEMORY_MANAGER_PARALLEL_THRESHOLD_ELEMENTS) {
        for (i = 0U; i < element_count; ++i) {
            manager->working->pool[i] = manager->original->pool[source_offset + i];
        }
    } else {
        const int copy_threads = memory_manager_copy_thread_count();
        const size_t count = element_count;

        #pragma omp parallel for schedule(static) num_threads(copy_threads)
        for (i = 0U; i < count; ++i) {
            manager->working->pool[i] = manager->original->pool[source_offset + i];
        }
    }
    return 0;
}

int memory_manager_restore(
    MemoryManager *manager,
    size_t element_count) {
    return memory_manager_restore_window(
        manager,
        UINT64_C(0),
        element_count
    );
}


void memory_manager_destroy(MemoryManager *manager) {
    if (manager != NULL) {
        destroy_data_buffer(manager->working);
        destroy_data_buffer(manager->original);
        free(manager);
    }
    return;
}