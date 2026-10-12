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
 * @file HWCounters.c
 * @brief Hardware performance-counter lifecycle and sampling.
 */

#include "profiling/HWCounters.h"
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

int hw_counters_init(HWCounters *hw) {
    if (!hw) return -1;
    memset(hw, 0, sizeof(HWCounters));

    for (int i = 0; i < HW_PERF_COUNT_MAX; i++) {
        hw->fds[i] = -1;
    }
    hw->leader_fd = -1;

    /* Configure the perf event type and event identifier for each counter. */
    uint32_t types[HW_PERF_COUNT_MAX] = {
        PERF_TYPE_HARDWARE,
        PERF_TYPE_HARDWARE,
        PERF_TYPE_HW_CACHE,
        PERF_TYPE_HW_CACHE
    };

    uint64_t configs[HW_PERF_COUNT_MAX] = {
        PERF_COUNT_HW_CPU_CYCLES,
        PERF_COUNT_HW_INSTRUCTIONS,
        PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
        PERF_COUNT_HW_CACHE_LL  | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)
    };

    /* Open the first counter as the event-group leader. */
    hw->fds[0] = perf_counter_open_process(types[0], configs[0], -1);
    if (hw->fds[0] < 0) {
        return -1;
    }
    hw->leader_fd = hw->fds[0];

    /* Open secondary counters as members of the leader event group. */
    for (int i = 1; i < HW_PERF_COUNT_MAX; i++) {
        hw->fds[i] = perf_counter_open_process(types[i], configs[i], hw->leader_fd);
        if (hw->fds[i] < 0) {
            hw_counters_cleanup(hw);
            return -1;
        }
    }

    hw->is_initialized = true;
    return 0;
}

int hw_counters_start(HWCounters *hw) {
    if (!hw || !hw->is_initialized) return -1;

    /* Reset and enable the event group through leader_fd. */
    ioctl(hw->leader_fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
    ioctl(hw->leader_fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
    return 0;
}

int hw_counters_stop(HWCounters *hw, HWCountersData *data) {
    if (!hw || !hw->is_initialized || !data) return -1;

    /* Disable the event group. */
    ioctl(hw->leader_fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);

    /* Read counter values from the individual file descriptors. */
    data->cycles       = 0;
    data->instructions = 0;
    data->l1d_misses   = 0;
    data->llc_misses   = 0;

    read(hw->fds[HW_PERF_CYCLES],       &data->cycles,       sizeof(uint64_t));
    read(hw->fds[HW_PERF_INSTRUCTIONS], &data->instructions, sizeof(uint64_t));
    read(hw->fds[HW_PERF_L1D_MISSES],   &data->l1d_misses,   sizeof(uint64_t));
    read(hw->fds[HW_PERF_LLC_MISSES],   &data->llc_misses,   sizeof(uint64_t));

    return 0;
}

void hw_counters_cleanup(HWCounters *hw) {
    if (!hw) return;

    for (int i = 0; i < HW_PERF_COUNT_MAX; i++) {
        if (hw->fds[i] >= 0) {
            perf_counter_close(hw->fds[i]);
            hw->fds[i] = -1;
        }
    }
    hw->leader_fd = -1;
    hw->is_initialized = false;
}