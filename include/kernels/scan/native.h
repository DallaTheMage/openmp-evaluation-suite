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
 * @file native.h
 * @brief Native OpenMP scan kernel declarations.
 */

#ifndef KERNELS_SCAN_NATIVE_H
#define KERNELS_SCAN_NATIVE_H

#include "kernels/common/Kernel.h"

/**
 * @brief Execute the 2D native scan benchmark.
 *
 * @param view Non-owning data view processed by the kernel.
 * @param test_case Immutable execution parameters for this run.
 * @param profiler Initialized profiler used for the measurement interval.
 * @param metric Output structure receiving the measured performance data.
 *
 * @pre All pointer arguments must be non-NULL.
 * @pre The test case must be valid for the supplied view.
 */
void kernel_2d_native_scan(DataView *, const struct TestCase *, struct Profiler *, struct PerformanceMetric *);
/**
 * @brief Execute the 3D native scan benchmark.
 *
 * @param view Non-owning data view processed by the kernel.
 * @param test_case Immutable execution parameters for this run.
 * @param profiler Initialized profiler used for the measurement interval.
 * @param metric Output structure receiving the measured performance data.
 *
 * @pre All pointer arguments must be non-NULL.
 * @pre The test case must be valid for the supplied view.
 */
void kernel_3d_native_scan(DataView *, const struct TestCase *, struct Profiler *, struct PerformanceMetric *);
/**
 * @brief Execute the AOS native scan benchmark.
 *
 * @param view Non-owning data view processed by the kernel.
 * @param test_case Immutable execution parameters for this run.
 * @param profiler Initialized profiler used for the measurement interval.
 * @param metric Output structure receiving the measured performance data.
 *
 * @pre All pointer arguments must be non-NULL.
 * @pre The test case must be valid for the supplied view.
 */
void kernel_aos_native_scan(DataView *, const struct TestCase *, struct Profiler *, struct PerformanceMetric *);
/**
 * @brief Execute the SOA native scan benchmark.
 *
 * @param view Non-owning data view processed by the kernel.
 * @param test_case Immutable execution parameters for this run.
 * @param profiler Initialized profiler used for the measurement interval.
 * @param metric Output structure receiving the measured performance data.
 *
 * @pre All pointer arguments must be non-NULL.
 * @pre The test case must be valid for the supplied view.
 */
void kernel_soa_native_scan(DataView *, const struct TestCase *, struct Profiler *, struct PerformanceMetric *);
/**
 * @brief Execute the AOSOA native scan benchmark.
 *
 * @param view Non-owning data view processed by the kernel.
 * @param test_case Immutable execution parameters for this run.
 * @param profiler Initialized profiler used for the measurement interval.
 * @param metric Output structure receiving the measured performance data.
 *
 * @pre All pointer arguments must be non-NULL.
 * @pre The test case must be valid for the supplied view.
 */
void kernel_aosoa_native_scan(DataView *, const struct TestCase *, struct Profiler *, struct PerformanceMetric *);

#endif /* KERNELS_SCAN_NATIVE_H */
