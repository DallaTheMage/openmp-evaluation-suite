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
 * @file BenchmarkValidation.h
 * @brief Compile-time switch and state for optional correctness checks.
 */

/* Optional correctness checks are disabled in normal benchmark builds. */
#ifndef KERNELS_COMMON_BENCHMARK_VALIDATION_H
#define KERNELS_COMMON_BENCHMARK_VALIDATION_H

#ifndef BENCH_VALIDATE_RESULTS
#define BENCH_VALIDATE_RESULTS 0
#endif

#if BENCH_VALIDATE_RESULTS
/** @brief Optional reduction checksum, updated outside measured intervals. */
extern volatile double benchmark_validation_reduction_sum;
#endif

#endif /* KERNELS_COMMON_BENCHMARK_VALIDATION_H */
