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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file RAPLPower.h
 * @brief Intel RAPL energy measurement interface.
 *
 * This module provides a small interface for measuring energy consumption
 * through the Linux powercap/RAPL interface.
 *
 * The implementation measures the energy consumed between a matching
 * rapl_power_start() and rapl_power_stop() pair.
 *
 * RAPL support is platform-dependent. When the interface is unavailable,
 * initialization may report failure or mark the RAPL instance as
 * unsupported. The profiler can therefore continue to operate without
 * assuming that energy counters are always available.
 *
 * The interface intentionally exposes raw RAPL measurements separately from
 * PerformanceMetric. Conversion and integration with the complete profiler
 * metric set are handled by Profiler.
 */

#ifndef PROFILING_RAPL_POWER_H
#define PROFILING_RAPL_POWER_H

#include <stdbool.h>

/**
 * @brief Runtime state of the RAPL energy counters.
 *
 * The structure owns the file descriptors used to access the configured
 * RAPL energy domains and stores the energy readings captured at the
 * beginning of a measurement interval.
 */
typedef struct RaplPower {
    /** File descriptor for the package energy counter. */
    int fd_pkg;

    /** File descriptor for the DRAM energy counter. */
    int fd_dram;

    /** Package-domain energy scale factor. */
    double scale_pkg;

    /** DRAM-domain energy scale factor. */
    double scale_dram;

    /** Package energy reading captured at measurement start, in microjoules. */
    double start_energy_pkg_uj;

    /** DRAM energy reading captured at measurement start, in microjoules. */
    double start_energy_dram_uj;

    /** True when the required RAPL interface is available. */
    bool is_supported;
} RaplPower;

/**
 * @brief Raw energy measurements produced by a RAPL interval.
 *
 * Energy values are expressed in joules so that they can be consumed
 * directly by the profiler without exposing the underlying RAPL unit.
 */
typedef struct RaplPowerData {
    /** Package energy consumed during the measurement interval, in joules. */
    double energy_pkg_joules;

    /** DRAM energy consumed during the measurement interval, in joules. */
    double energy_dram_joules;
} RaplPowerData;

/**
 * @brief Initialize the RAPL energy measurement interface.
 *
 * The function discovers and opens the supported RAPL energy domains and
 * initializes the scale factors required to convert hardware readings.
 *
 * RAPL is optional for the benchmark suite. Therefore, lack of RAPL support
 * is represented through the return value and/or the RaplPower state rather
 * than being assumed to be a programming error by callers.
 *
 * @param rapl RAPL state to initialize.
 *
 * @return 0 on successful initialization, non-zero on failure.
 *
 * @pre rapl != NULL
 *
 * @post On success, rapl contains the state required by
 *       rapl_power_start() and rapl_power_stop().
 */
int rapl_power_init(RaplPower *rapl);

/**
 * @brief Capture the initial RAPL energy readings.
 *
 * This starts an energy measurement interval. The corresponding interval
 * must be terminated with rapl_power_stop().
 *
 * @param rapl Initialized RAPL state.
 *
 * @return 0 on success, non-zero on failure.
 *
 * @pre rapl != NULL
 * @pre rapl_power_init(rapl) completed successfully.
 */
int rapl_power_start(RaplPower *rapl);

/**
 * @brief Stop the RAPL measurement interval and compute energy consumption.
 *
 * @param rapl Initialized RAPL state.
 * @param data Output structure receiving the energy consumed during the
 *             measurement interval.
 *
 * @return 0 on success, non-zero on failure.
 *
 * @pre rapl != NULL
 * @pre data != NULL
 * @pre rapl_power_start(rapl) completed successfully.
 *
 * @post On success, data contains package and DRAM energy consumption
 *       expressed in joules.
 */
int rapl_power_stop(
    RaplPower *rapl,
    RaplPowerData *data
);

/**
 * @brief Release RAPL resources.
 *
 * All file descriptors owned by the RAPL state are closed.
 *
 * Passing NULL is allowed and has no effect.
 *
 * @param rapl RAPL state to clean up.
 */
void rapl_power_cleanup(RaplPower *rapl);

#endif /* PROFILING_RAPL_POWER_H */