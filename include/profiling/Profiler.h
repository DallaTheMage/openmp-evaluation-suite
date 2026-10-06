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
 * @file Profiler.h
 * @brief Performance measurement interface for benchmark kernels.
 *
 * The profiler coordinates wall-clock timing, hardware performance counters
 * and RAPL energy measurements for a single benchmark execution.
 *
 * A Profiler instance follows the lifecycle:
 *
 *     profiler_init()
 *          |
 *          v
 *     profiler_start()
 *          |
 *          v
 *     profiler_stop()
 *          |
 *          v
 *     profiler_cleanup()
 *
 * The benchmark kernel receives a Profiler pointer so that measurement can
 * start and stop as close as possible to the OpenMP region being evaluated.
 *
 * The profiler owns its hardware-counter and RAPL state, but does not own
 * the PerformanceMetric object supplied to profiler_stop().
 *
 * This interface is intentionally independent of the benchmark plan and
 * kernel registry.
 */

#ifndef PROFILING_PROFILER_H
#define PROFILING_PROFILER_H

#include <stdbool.h>
#include <stdint.h>

#include "profiling/HWCounters.h"
#include "profiling/RAPLPower.h"

/**
 * @brief Performance measurements produced by one benchmark execution.
 *
 * The structure contains raw measurements collected from the wall-clock
 * timer, hardware performance counters and RAPL, together with metrics
 * derived from those measurements.
 *
 * All energy values are expressed in joules.
 * Wall-clock time is expressed in seconds.
 */
typedef struct PerformanceMetric {
    /** Effective wall-clock execution time in seconds. */
    double wall_time_sec;

    /** CPU cycle count. */
    uint64_t cycles;

    /** Retired instruction count. */
    uint64_t instructions;

    /** L1 data-cache miss count. */
    uint64_t l1d_misses;

    /** Last-level-cache miss count. */
    uint64_t llc_misses;

    /** Instructions per cycle. */
    double ipc;

    /** L1 data-cache miss ratio. */
    double l1d_miss_ratio;

    /** Last-level-cache miss ratio. */
    double llc_miss_ratio;

    /** Package energy consumed during the measurement, in joules. */
    double energy_pkg_joules;

    /** DRAM energy consumed during the measurement, in joules. */
    double energy_dram_joules;
} PerformanceMetric;

/**
 * @brief Runtime state of a performance profiler.
 *
 * A Profiler owns the state required to coordinate hardware-counter,
 * RAPL and wall-clock measurements.
 *
 * The structure must be initialized with profiler_init() before use.
 * profiler_start() and profiler_stop() define one measurement interval.
 */
typedef struct Profiler {
    /** Hardware performance-counter state. */
    HWCounters hw;

    /** RAPL energy-measurement state. */
    RaplPower rapl;

    /** Wall-clock timestamp captured by profiler_start(). */
    double start_wtime;

    /** True while a measurement interval is active. */
    bool is_running;
} Profiler;

/**
 * @brief Initialize a performance profiler.
 *
 * This function initializes the hardware-counter and RAPL backends and
 * prepares the profiler for measurement.
 *
 * @param p Profiler instance to initialize.
 *
 * @return 0 on success, non-zero on failure.
 *
 * @pre p != NULL
 *
 * @post On success, the profiler can be passed to profiler_start().
 * @post The profiler is not running after successful initialization.
 */
int profiler_init(Profiler *p);

/**
 * @brief Start one performance measurement interval.
 *
 * This function starts wall-clock, hardware-counter and energy
 * measurements as close as possible to the benchmark region.
 *
 * @param p Initialized profiler.
 *
 * @return 0 on success, non-zero on failure.
 *
 * @pre p != NULL
 * @pre profiler_init(p) has completed successfully.
 * @pre p->is_running == false
 *
 * @post On success, p->is_running is true.
 */
int profiler_start(Profiler *p);

/**
 * @brief Stop the active measurement and collect its metrics.
 *
 * This function stops all active measurement backends and stores the
 * resulting measurements and derived metrics in the supplied output
 * structure.
 *
 * @param p Initialized and running profiler.
 * @param metric Output structure receiving the measurement results.
 *
 * @return 0 on success, non-zero on failure.
 *
 * @pre p != NULL
 * @pre metric != NULL
 * @pre p->is_running == true
 *
 * @post On successful completion, p->is_running is false.
 * @post metric contains the measurements for the completed interval.
 */
int profiler_stop(
    Profiler *p,
    PerformanceMetric *metric
);

/**
 * @brief Release all resources owned by a profiler.
 *
 * Cleanup is safe to call after initialization and may be used to release
 * partially initialized backend state after an initialization failure.
 *
 * @param p Profiler instance to clean up.
 *
 * @note Passing NULL has no effect.
 */
void profiler_cleanup(Profiler *p);

#endif /* PROFILING_PROFILER_H */