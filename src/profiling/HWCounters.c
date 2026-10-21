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
 * @file HWCounters.c
 * @brief Linux perf_event counters aggregated across enumerated process threads.
 *
 * Implementation details are kept separate from the public API contracts
 * documented in the corresponding header.
 *
 * @note Counters are opened for threads visible in /proc/self/task during initialization.
 */

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include "profiling/HWCounters.h"

static const uint32_t types[HW_PERF_COUNT_MAX] = {
    PERF_TYPE_HARDWARE, PERF_TYPE_HARDWARE,
    PERF_TYPE_HW_CACHE, PERF_TYPE_HW_CACHE
};

static const uint64_t configs[HW_PERF_COUNT_MAX] = {
    PERF_COUNT_HW_CPU_CYCLES,
    PERF_COUNT_HW_INSTRUCTIONS,
    PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) |
        (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
    PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_READ << 8) |
        (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)
};

void hw_counters_cleanup(HWCounters *hw) {
    if (!hw) {
        return;
    }
    for (size_t t = 0; t < hw->thread_count; t++) {
        for (int i = 0; i < HW_PERF_COUNT_MAX; i++) {
            perf_counter_close(hw->threads[t].fds[i]);
        }
    }
    free(hw->threads);
    hw->threads = NULL;
    hw->thread_count = 0;
    hw->is_initialized = false;
}

int hw_counters_init(HWCounters *hw) {
    if (!hw) {
        return -1;
    }
    memset(hw, 0, sizeof(*hw));
    DIR *dir = opendir("/proc/self/task");
    if (!dir) {
        return -1;
    }
    struct dirent *entry;
    size_t capacity = 0;
    while ((entry = readdir(dir)) != NULL) {
        char *end;
        errno = 0;
        long tid = strtol(entry->d_name, &end, 10);
        if (errno || (*end != '\0') || (tid <= 0) || (tid > INT_MAX)){
            continue;
        }
        if (hw->thread_count == capacity) {
            if (capacity > (SIZE_MAX / (2 * sizeof(*hw->threads)))) {
                closedir(dir);
                hw_counters_cleanup(hw);
                return -1;
            }
            size_t next = capacity ? (capacity * 2) : 8;
            HWThreadCounters *tmp = realloc(hw->threads, next * sizeof(*tmp));
            if (!tmp) {
                closedir(dir);
                hw_counters_cleanup(hw);
                return -1;
            }
            hw->threads = tmp;
            capacity = next;
        }
        HWThreadCounters *th = &hw->threads[hw->thread_count++];
        for (int i = 0; i < HW_PERF_COUNT_MAX; i++){
            th->fds[i] = -1;
        }
        th->leader_fd = -1;
        for (int i = 0; i < HW_PERF_COUNT_MAX; i++) {
            th->fds[i] = perf_counter_open_thread(types[i], configs[i], (pid_t)tid, th->leader_fd);
            if (th->fds[i] < 0) {
                closedir(dir);
                hw_counters_cleanup(hw);
                return -1;
            }
            if (i == 0) {
                th->leader_fd = th->fds[i];
            }
        }
    }
    closedir(dir);
    if (!hw->thread_count) {
        hw_counters_cleanup(hw);
        return -1;
    }
    hw->is_initialized = true;
    return 0;
}

int hw_counters_start(HWCounters *hw) {
    if (!hw || !hw->is_initialized) {
        return -1;
    }
    size_t started = 0;
    for (size_t t = 0; t < hw->thread_count; t++) {
        int fd = hw->threads[t].leader_fd;
        if (ioctl(fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP) == -1 ||
            ioctl(fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP) == -1) {
            (void)ioctl(fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
            for (size_t j = 0; j < started; j++) {
                (void)ioctl(hw->threads[j].leader_fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
            }
            return -1;
        }
        started++;
    }
    return 0;
}

typedef struct PerfRead {
    uint64_t value, time_enabled, time_running;
} PerfRead;

int hw_counters_stop(HWCounters *hw, HWCountersData *data) {
    if (!hw || !hw->is_initialized || !data) {
        return -1;
    }
    memset(data, 0, sizeof(*data));
    for (size_t t = 0; t < hw->thread_count; t++)
        if (ioctl(hw->threads[t].leader_fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP) == -1) {
            return -1;
        }
    uint64_t totals[HW_PERF_COUNT_MAX] = {0};
    for (size_t t = 0; t < hw->thread_count; t++) {
        for (int i = 0; i < HW_PERF_COUNT_MAX; i++) {
            PerfRead r;
            if (read(hw->threads[t].fds[i], &r, sizeof(r)) != (ssize_t)sizeof(r)
                || r.time_running == 0) {
                return -1;
            }
            long double scaled = (long double)r.value *
                (long double)r.time_enabled / (long double)r.time_running;
            if ((scaled < 0) || scaled > ((long double)UINT64_MAX - totals[i])) {
                return -1;
            }
            totals[i] += (uint64_t)(scaled + 0.5L);
        }
    }
    data->cycles       = totals[HW_PERF_CYCLES];
    data->instructions = totals[HW_PERF_INSTRUCTIONS];
    data->l1d_misses   = totals[HW_PERF_L1D_MISSES];
    data->llc_misses   = totals[HW_PERF_LLC_MISSES];
    return 0;
}
