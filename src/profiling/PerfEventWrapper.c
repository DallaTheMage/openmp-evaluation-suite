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
 * @file PerfEventWrapper.c
 * @brief Low-level Linux perf-event wrappers.
 */

#include "profiling/PerfEventWrapper.h"
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

int sys_perf_event_open(
    struct perf_event_attr *hw_event,
    pid_t                   pid,
    int                     cpu,
    int                     group_fd,
    unsigned long           flags
)
{
    return (int) syscall(
        __NR_perf_event_open,
        hw_event,
        pid,
        cpu,
        group_fd,
        flags
    );
}

int perf_counter_open_process(uint32_t type, uint64_t config, int group_fd) {
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(struct perf_event_attr));

    pe.type = type;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = config;

    /* Initially disabled; hw_counters_start() enables the counter explicitly. */
    pe.disabled = 1;

    /*
     * OPENMP / MULTITHREAD TRACKING:
     * inherit = 1 allows child threads created by OpenMP parallel regions to
     * inherit this counter configuration and contribute measurements through
     * the inherited file descriptor.
     */
    pe.inherit = 1;

    /* Exclude kernel and hypervisor execution to measure user-space benchmark code only. */
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    /*
     * pid = 0: Monitor the current process and inheriting child threads.
     * cpu = -1: Monitor on any CPU on which the threads are scheduled.
     */
    return sys_perf_event_open(&pe, 0, -1, group_fd, 0);
}

void perf_counter_close(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}

