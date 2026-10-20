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
 * @file loop.h
 * @brief OpenMP worksharing-loop kernel declarations.
 */

#ifndef KERNELS_WORKSHARING_LOOP_H
#define KERNELS_WORKSHARING_LOOP_H

#include "kernels/common/Kernel.h"

/**
 * @brief Execute the 2D loop benchmark.
 *
 * @param view Non-owning data view processed by the kernel.
 * @param test_case Immutable execution parameters for this run.
 * @param profiler Initialized profiler used for the measurement interval.
 * @param metric Output structure receiving the measured performance data.
 *
 * @pre All pointer arguments must be non-NULL.
 * @pre The test case must be valid for the supplied view.
 */
KernelStatus kernel_2d_loop(DataView *view, const struct TestCase *test_case, struct Profiler *profiler, struct PerformanceMetric *metric);
/**
 * @brief Execute the 3D loop benchmark.
 *
 * @param view Non-owning data view processed by the kernel.
 * @param test_case Immutable execution parameters for this run.
 * @param profiler Initialized profiler used for the measurement interval.
 * @param metric Output structure receiving the measured performance data.
 *
 * @pre All pointer arguments must be non-NULL.
 * @pre The test case must be valid for the supplied view.
 */
KernelStatus kernel_3d_loop(DataView *view, const struct TestCase *test_case, struct Profiler *profiler, struct PerformanceMetric *metric);
/**
 * @brief Execute the AOS loop benchmark.
 *
 * @param view Non-owning data view processed by the kernel.
 * @param test_case Immutable execution parameters for this run.
 * @param profiler Initialized profiler used for the measurement interval.
 * @param metric Output structure receiving the measured performance data.
 *
 * @pre All pointer arguments must be non-NULL.
 * @pre The test case must be valid for the supplied view.
 */
KernelStatus kernel_aos_loop(DataView *view, const struct TestCase *test_case, struct Profiler *profiler, struct PerformanceMetric *metric);
/**
 * @brief Execute the SOA loop benchmark.
 *
 * @param view Non-owning data view processed by the kernel.
 * @param test_case Immutable execution parameters for this run.
 * @param profiler Initialized profiler used for the measurement interval.
 * @param metric Output structure receiving the measured performance data.
 *
 * @pre All pointer arguments must be non-NULL.
 * @pre The test case must be valid for the supplied view.
 */
KernelStatus kernel_soa_loop(DataView *view, const struct TestCase *test_case, struct Profiler *profiler, struct PerformanceMetric *metric);
/**
 * @brief Execute the AOSOA loop benchmark.
 *
 * @param view Non-owning data view processed by the kernel.
 * @param test_case Immutable execution parameters for this run.
 * @param profiler Initialized profiler used for the measurement interval.
 * @param metric Output structure receiving the measured performance data.
 *
 * @pre All pointer arguments must be non-NULL.
 * @pre The test case must be valid for the supplied view.
 */
KernelStatus kernel_aosoa_loop(DataView *view, const struct TestCase *test_case, struct Profiler *profiler, struct PerformanceMetric *metric);

KernelStatus kernel_soa_loop_inner(DataView *view, const struct TestCase *test_case, struct Profiler *profiler, struct PerformanceMetric *metric);

#endif /* KERNELS_WORKSHARING_LOOP_H */
