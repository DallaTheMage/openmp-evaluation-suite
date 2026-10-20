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
 * @file BenchmarkRunner.h
 * @brief Internal entry point for running one benchmark suite.
 */

#ifndef CORE_INTERNAL_BENCHMARK_RUNNER_H
#define CORE_INTERNAL_BENCHMARK_RUNNER_H

#include "core/Configuration.h"
#include "core/ResultsWriter.h"
#include "data/MemoryManager.h"

/**
 * @brief Run one configured benchmark suite and write its results.
 * @param memory_manager Manager owning the benchmark work memory.
 * @param config Configuration controlling the benchmark run.
 * @param test_name Name used to label the suite output.
 * @param scaling_mode Scaling mode used for result serialization.
 * @param proportional Whether to generate a proportional test plan.
 * @return Zero on success; nonzero if setup, execution or output fails.
 */
int run_benchmark_suite(MemoryManager *memory_manager,
    const Configuration *config, const char *test_name,
    ScalingMode scaling_mode, int proportional);

#endif
