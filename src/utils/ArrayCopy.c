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
 * @file ArrayCopy.c
 * @brief Implementation of checked array allocation and copying.
 */

#include "utils/ArrayCopy.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void *utils_array_copy(const void *source, size_t count, size_t element_size) {
    if (source == NULL || count == 0U || element_size == 0U ||
        count > SIZE_MAX / element_size) {
        return NULL;
    }
    void *data = malloc(count * element_size);
    if (data != NULL) {
        memcpy(data, source, count * element_size);
    }
    return data;
}
