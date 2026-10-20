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
 * @file ArrayCopy.h
 * @brief Checked allocation and copying of fixed-size array elements.
 */

#ifndef UTILS_ARRAY_COPY_H
#define UTILS_ARRAY_COPY_H

#include <stddef.h>

/**
 * @brief Allocate and copy an array, checking for size overflow.
 * @param source Source array.
 * @param count Number of elements to copy.
 * @param element_size Size of each element in bytes.
 * @return Allocated copy, or NULL for invalid input or allocation failure.
 * @note The caller owns the returned memory.
 */
void *utils_array_copy(const void *source, size_t count, size_t element_size);

#endif /* UTILS_ARRAY_COPY_H */
