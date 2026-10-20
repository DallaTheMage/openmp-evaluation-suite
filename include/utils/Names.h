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
 * @file Names.h
 * @brief Stable human-readable names for views and benchmark kernels.
 */

#ifndef UTILS_NAMES_H
#define UTILS_NAMES_H

#include "data/views/ViewType.h"
#include "kernels/common/KernelType.h"

/**
 * @brief Return a human-readable name for a view type.
 * @param type View type.
 * @return Static string (never free).
 */
const char *utils_view_type_name(ViewType type);
/**
 * @brief Return a kernel name appropriate for the requested output context.
 * @param type Kernel type.
 * @return Static string (never free).
 */
const char *utils_kernel_type_name(KernelType type);

#endif /* UTILS_NAMES_H */
