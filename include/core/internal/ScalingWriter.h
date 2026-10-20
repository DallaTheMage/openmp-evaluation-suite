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
 * @file ScalingWriter.h
 * @brief Internal writers for strong and weak scaling CSV output.
 */

#ifndef CORE_INTERNAL_SCALING_WRITER_H
#define CORE_INTERNAL_SCALING_WRITER_H

#include <stdio.h>
#include <stddef.h>
#include "core/Analyzer.h"
#include "core/TestPlan.h"

/**
 * @brief Write strong-scaling comparisons to a CSV stream.
 * @param file Open output stream.
 * @param plan Test plan supplying case metadata.
 * @param aggregated Aggregated benchmark samples.
 * @param aggregated_count Number of entries in aggregated.
 * @return Writer status (zero on success).
 */
int write_scaling_strong(FILE *file, const TestPlan *plan,
    const AggregatedSample *aggregated, size_t aggregated_count);
/**
 * @brief Write weak-scaling comparisons to a CSV stream.
 * @param file Open output stream.
 * @param plan Test plan supplying case metadata.
 * @param aggregated Aggregated benchmark samples.
 * @param aggregated_count Number of entries in aggregated.
 * @return Writer status (zero on success).
 */
int write_scaling_weak(FILE *file, const TestPlan *plan,
    const AggregatedSample *aggregated, size_t aggregated_count);

#endif
