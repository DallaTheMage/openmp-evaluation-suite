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
 * @file MemoryManager.h
 * @brief Owns and prepares the benchmark's original and working data buffers.
 *
 * The memory manager owns the immutable, deterministically generated source
 * buffer and a reusable working buffer. The working buffer grows on demand
 * and is never shrunk during the manager's lifetime.
 */

#ifndef DATA_MEMORY_MANAGER_H
#define DATA_MEMORY_MANAGER_H

#include <stddef.h>
#include <stdint.h>

#include "data/DataBuffer.h"

/** @brief Opaque owner of benchmark data buffers. */
typedef struct MemoryManager MemoryManager;

/**
 * @brief Create the memory manager and initialize the original data buffer.
 *
 * The original buffer is filled exactly once using the existing deterministic
 * generator. Working storage is allocated later, after the test plan reveals
 * the largest workload required by a suite.
 *
 * @param original_element_count Number of elements in the original dataset.
 * @param alignment Required buffer alignment in bytes.
 * @param seed Seed passed unchanged to the data generator.
 * @return A new manager, or NULL if allocation or initialization setup fails.
 */
MemoryManager *memory_manager_create(size_t original_element_count, size_t alignment, uint64_t seed);

/**
 * @brief Get the original, immutable data buffer.
 *
 * The returned pointer is borrowed and remains owned by the manager. Callers
 * must not modify its contents or destroy the buffer.
 *
 * @param manager Memory manager.
 * @return Borrowed original buffer, or NULL if manager is NULL.
 */
DataBuffer *memory_manager_original_buffer(MemoryManager *manager);

/**
 * @brief Get the current working buffer.
 *
 * The returned pointer is borrowed and remains owned by the manager. It may
 * become invalid if a later call to memory_manager_reserve_work() grows the
 * working buffer; consumers should reacquire it after reserving capacity.
 *
 * @param manager Memory manager.
 * @return Borrowed working buffer, or NULL if no working buffer exists.
 */
DataBuffer *memory_manager_work_buffer(MemoryManager *manager);

/**
 * @brief Ensure the working buffer can hold at least the requested elements.
 *
 * Capacity only grows; an existing buffer is retained when it is already
 * large enough. A failed growth leaves the current buffer intact.
 *
 * @param manager Memory manager.
 * @param required_elements Minimum required capacity in elements.
 * @return 0 on success, non-zero on invalid input or allocation failure.
 */
int memory_manager_reserve_work(MemoryManager *manager, size_t required_elements);

/**
 * @brief Restore the requested prefix of the working buffer.
 *
 * Small copies use a sequential loop. Larger copies use a static-scheduled
 * OpenMP loop with a preparation-specific thread count independent of the
 * benchmark test's thread count.
 *
 * @param manager Memory manager.
 * @param element_count Number of elements to restore.
 * @return 0 on success, non-zero if the request exceeds buffer capacity.
 */
int memory_manager_restore(MemoryManager *manager, size_t element_count);

/**
 * @brief Restore a logical window into the beginning of the working buffer.
 *
 * The requested range is copied from the original buffer at the specified
 * offset to the beginning of the working buffer.
 *
 * The copy is performed outside the kernel profiling interval.
 *
 * @param manager Memory manager.
 * @param offset Starting element in the original buffer.
 * @param element_count Number of elements to restore.
 *
 * @return 0 on success, non-zero if the requested window exceeds the
 *         original buffer or the working buffer capacity.
 */
int memory_manager_restore_window(MemoryManager *manager, uint64_t offset, size_t element_count);

/**
 * @brief Destroy the manager and all buffers it owns.
 *
 * @param manager Manager to destroy.
 */
void memory_manager_destroy(MemoryManager *manager);

#endif /* DATA_MEMORY_MANAGER_H */