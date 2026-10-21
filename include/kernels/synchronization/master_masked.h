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
 * @file master_masked.h
 * @brief OpenMP master and masked kernel declarations.
 */

#ifndef KERNELS_SYNCHRONIZATION_MASTER_MASKED_H
#define KERNELS_SYNCHRONIZATION_MASTER_MASKED_H

#include "kernels/common/Kernel.h"

/**
 * @brief Execute the 1D master benchmark.
 *
 * @param view Non-owning data view processed by the kernel.
 * @param test_case Immutable execution parameters for this run.
 * @param profiler Initialized profiler used for the measurement interval.
 * @param metric Output structure receiving the measured performance data.
 *
 * @pre All pointer arguments must be non-NULL.
 * @pre The test case must be valid for the supplied view.
 * @return KERNEL_STATUS_OK on success, or a KernelStatus error.
 */
KernelStatus kernel_1d_master(DataView *view, const struct TestCase *test_case, struct Profiler *profiler, struct PerformanceMetric *metric);

/**
 * @brief Execute the 2D master benchmark.
 *
 * @param view Non-owning data view processed by the kernel.
 * @param test_case Immutable execution parameters for this run.
 * @param profiler Initialized profiler used for the measurement interval.
 * @param metric Output structure receiving the measured performance data.
 *
 * @pre All pointer arguments must be non-NULL.
 * @pre The test case must be valid for the supplied view.
 * @return KERNEL_STATUS_OK on success, or a KernelStatus error.
 */
KernelStatus kernel_2d_master(DataView *view, const struct TestCase *test_case, struct Profiler *profiler, struct PerformanceMetric *metric);
/**
 * @brief Execute the 1D masked benchmark.
 *
 * @param view Non-owning data view processed by the kernel.
 * @param test_case Immutable execution parameters for this run.
 * @param profiler Initialized profiler used for the measurement interval.
 * @param metric Output structure receiving the measured performance data.
 *
 * @pre All pointer arguments must be non-NULL.
 * @pre The test case must be valid for the supplied view.
 * @return KERNEL_STATUS_OK on success, or a KernelStatus error.
 */
KernelStatus kernel_1d_masked(DataView *view, const struct TestCase *test_case, struct Profiler *profiler, struct PerformanceMetric *metric);

/**
 * @brief Execute the 2D masked benchmark.
 *
 * @param view Non-owning data view processed by the kernel.
 * @param test_case Immutable execution parameters for this run.
 * @param profiler Initialized profiler used for the measurement interval.
 * @param metric Output structure receiving the measured performance data.
 *
 * @pre All pointer arguments must be non-NULL.
 * @pre The test case must be valid for the supplied view.
 * @return KERNEL_STATUS_OK on success, or a KernelStatus error.
 */
KernelStatus kernel_2d_masked(DataView *view, const struct TestCase *test_case, struct Profiler *profiler, struct PerformanceMetric *metric);

#endif /* KERNELS_SYNCHRONIZATION_MASTER_MASKED_H */
