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
 * @file HWCounters.h
 * @brief Hardware performance-counter measurement interface.
 *
 * This module manages the CPU hardware performance counters used by the
 * benchmark profiler.
 *
 * The counters are grouped into a single measurement set:
 *
 *     cycles
 *     instructions
 *     L1 data-cache misses
 *     last-level-cache misses
 *
 * The implementation is responsible for opening, starting, stopping and
 * closing the underlying perf_event counters.
 *
 * HWCounters does not own the final PerformanceMetric object. It produces
 * raw counter values through HWCountersData, which are subsequently
 * consumed by the profiler.
 *
 * This interface is Linux-specific because the implementation relies on
 * perf_event_open().
 */

#ifndef PROFILING_HW_COUNTERS_H
#define PROFILING_HW_COUNTERS_H

#include <stdbool.h>
#include <stdint.h>

#include "profiling/PerfEventWrapper.h"

/**
 * @brief Hardware performance events monitored by the benchmark.
 *
 * The enumeration order is part of the internal ABI of HWCounters because
 * it corresponds to the indices used by HWCounters::fds.
 */
typedef enum HWPerfEventType {
    /** Retired CPU cycle count. */
    HW_PERF_CYCLES = 0,

    /** Retired instruction count. */
    HW_PERF_INSTRUCTIONS,

    /** L1 data-cache miss count. */
    HW_PERF_L1D_MISSES,

    /** Last-level-cache miss count. */
    HW_PERF_LLC_MISSES,

    /** Number of supported hardware events. */
    HW_PERF_COUNT_MAX
} HWPerfEventType;

/**
 * @brief Runtime state of the hardware performance counters.
 *
 * The structure owns the file descriptors associated with the configured
 * hardware events.
 */
typedef struct HWCounters {
    /** File descriptors indexed by HWPerfEventType. */
    int fds[HW_PERF_COUNT_MAX];

    /**
     * File descriptor of the group leader.
     *
     * The remaining counters are attached to this group.
     */
    int leader_fd;

    /** True after successful counter initialization. */
    bool is_initialized;
} HWCounters;

/**
 * @brief Raw values collected from the hardware counters.
 *
 * Values are kept separate from PerformanceMetric because this structure
 * represents raw hardware observations rather than derived metrics.
 */
typedef struct HWCountersData {
    /** Retired CPU cycles. */
    uint64_t cycles;

    /** Retired instructions. */
    uint64_t instructions;

    /** L1 data-cache misses. */
    uint64_t l1d_misses;

    /** Last-level-cache misses. */
    uint64_t llc_misses;
} HWCountersData;

/**
 * @brief Initialize the hardware performance counters.
 *
 * The function creates the configured perf-event group and prepares it
 * for measurements.
 *
 * @param hw Hardware-counter state to initialize.
 *
 * @return 0 on success, non-zero on failure.
 *
 * @pre hw != NULL
 *
 * @post On success, hw->is_initialized is true.
 * @post On success, the counter descriptors are ready for
 *       hw_counters_start().
 */
int hw_counters_init(HWCounters *hw);

/**
 * @brief Start all hardware performance counters.
 *
 * @param hw Initialized hardware-counter state.
 *
 * @return 0 on success, non-zero on failure.
 *
 * @pre hw != NULL
 * @pre hw_counters_init(hw) completed successfully.
 *
 * @post On success, all configured counters are active.
 */
int hw_counters_start(HWCounters *hw);

/**
 * @brief Stop the counters and read their raw values.
 *
 * @param hw Initialized hardware-counter state.
 * @param data Output structure receiving the counter values.
 *
 * @return 0 on success, non-zero on failure.
 *
 * @pre hw != NULL
 * @pre data != NULL
 * @pre hw_counters_init(hw) completed successfully.
 *
 * @post On success, data contains the values measured by the counters.
 */
int hw_counters_stop(
    HWCounters *hw,
    HWCountersData *data
);

/**
 * @brief Release all hardware-counter resources.
 *
 * All open counter file descriptors are closed.
 *
 * Passing NULL is allowed and has no effect.
 *
 * @param hw Hardware-counter state to clean up.
 */
void hw_counters_cleanup(HWCounters *hw);

#endif /* PROFILING_HW_COUNTERS_H */
