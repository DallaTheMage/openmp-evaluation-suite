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
 * the PerformanceMetric object supplied to profiler_stop(). Hardware-counter
 * and energy backends are optional: wall-clock timing remains available when
 * a backend is unsupported or inaccessible.
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
    double   wall_time_sec;      /* Effective wall-clock execution time in seconds. */
    uint64_t cycles;             /* CPU cycle count. */
    uint64_t instructions;       /* Retired instruction count. */
    uint64_t l1d_misses;         /* L1 data-cache miss count. */
    uint64_t llc_misses;         /* Last-level-cache miss count. */
    double   ipc;                /* Instructions per cycle. */
    double   l1d_miss_ratio;     /* L1 data-cache miss ratio. */
    double   llc_miss_ratio;     /* Last-level-cache miss ratio. */
    double   energy_pkg_joules;  /* Package energy consumed during the measurement, in joules. */
    double   energy_dram_joules; /* DRAM energy consumed during the measurement, in joules. */
    bool hw_counters_valid;       /* False when perf counters are unavailable. */
    bool energy_pkg_valid;       /* False when package energy was not collected. */
    bool energy_dram_valid;      /* False when DRAM energy was not collected. */
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
    HWCounters hw;          /* Hardware performance-counter state. */
    RaplPower  rapl;        /* RAPL energy-measurement state. */
    double     start_wtime; /* Wall-clock timestamp captured by profiler_start(). */
    bool       is_running;  /* True while a measurement interval is active. */
} Profiler;

/**
 * @brief Initialize a performance profiler.
 *
 * This function initializes the optional hardware-counter and RAPL backends
 * and prepares the profiler for measurement. Backend initialization failures
 * do not prevent wall-clock measurements from being collected.
 *
 * @param p Profiler instance to initialize.
 *
 * @return 0 when the profiler state is initialized, non-zero only when
 *         the input pointer is invalid.
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
 * This function starts wall-clock measurement and, when available, the
 * hardware-counter and energy backends as close as possible to the benchmark
 * region.
 *
 * @param p Initialized profiler.
 *
 * @return 0 on success, non-zero when the profiler is NULL or already
 *         running.
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
 * structure. Unavailable optional backends produce NaN floating-point metrics and false availability flags; raw integer counters remain zero.
 *
 * @param p Initialized and running profiler.
 * @param metric Output structure receiving the measurement results.
 *
 * @return 0 on success, non-zero when the profiler state or output pointer
 *         is invalid.
 *
 * @pre p != NULL
 * @pre metric != NULL
 * @pre p->is_running == true
 *
 * @post On successful completion, p->is_running is false.
 * @post metric contains the measurements for the completed interval.
 */
int profiler_stop(Profiler *p, PerformanceMetric *metric);

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
