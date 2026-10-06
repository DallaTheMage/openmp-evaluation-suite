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
 * @file PerfEventWrapper.h
 * @brief Low-level wrapper around the Linux perf_event_open system call.
 *
 * This header provides the minimal interface required by the hardware
 * performance-counter subsystem.
 *
 * The wrapper deliberately does not contain event-selection logic,
 * counter-group management, measurement policy, or metric computation.
 * Those responsibilities belong to HWCounters.
 *
 * The implementation is Linux-specific because it relies on the
 * perf_event_open system call and the corresponding kernel interface.
 *
 * The benchmark suite itself follows C99 for its source code, while this
 * profiling backend necessarily depends on Linux-specific system interfaces.
 *
 * @warning This interface is intended for the profiling backend only.
 *          It must not be used directly by benchmark kernels.
 */

#ifndef PROFILING_PERF_EVENT_WRAPPER_H
#define PROFILING_PERF_EVENT_WRAPPER_H

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>


/**
 * @brief Invoke the Linux perf_event_open system call.
 *
 * This helper provides the raw system-call interface used by the
 * higher-level hardware-counter implementation.
 *
 * The function is kept as a small inline wrapper so that no additional
 * abstraction or dispatch overhead is introduced in the profiling path.
 *
 * @param hw_event
 *     Pointer to the perf_event_attr structure describing the event.
 *
 * @param pid
 *     Process or thread identifier to which the event is attached.
 *
 * @param cpu
 *     CPU on which the event is monitored, or -1 when the event is
 *     attached to the specified process/thread independently of a
 *     particular CPU.
 *
 * @param group_fd
 *     File descriptor of the event-group leader, or -1 when creating
 *     a new group.
 *
 * @param flags
 *     perf_event_open flags.
 *
 * @return
 *     A non-negative file descriptor on success.
 *     A negative value on failure.
 *
 * @pre hw_event != NULL.
 */
static inline int sys_perf_event_open(
    struct perf_event_attr *hw_event,
    pid_t                   pid,
    int                     cpu,
    int                     group_fd,
    unsigned long           flags
)
{
    return (int)syscall(
        __NR_perf_event_open,
        hw_event,
        pid,
        cpu,
        group_fd,
        flags
    );
}


/**
 * @brief Descriptor and configuration of a perf event.
 *
 * This structure represents the minimal state associated with one
 * perf_event counter.
 *
 * The descriptor is owned by the component that creates the counter.
 * This type does not perform resource management by itself.
 */
typedef struct PerfCounterFD {
    int      fd;
    uint64_t type;
    uint64_t config;
} PerfCounterFD;


/**
 * @brief Open a perf event for the current process.
 *
 * The event is opened using the Linux perf_event_open interface and can
 * subsequently be controlled by the hardware-counter subsystem.
 *
 * The function does not select or interpret hardware events. The caller
 * is responsible for providing a valid perf event type and configuration.
 *
 * @param type
 *     Linux perf event type, for example PERF_TYPE_HARDWARE or
 *     PERF_TYPE_HW_CACHE.
 *
 * @param config
 *     Event-specific configuration value.
 *
 * @param group_fd
 *     File descriptor of the group leader, or -1 to create a new group.
 *
 * @return
 *     A non-negative file descriptor on success.
 *     A negative value on failure.
 *
 * @note
 *     The implementation may configure the event so that measurements
 *     include the OpenMP worker threads created by the benchmark process.
 *     The exact event semantics are determined by the implementation and
 *     Linux perf subsystem.
 */
int perf_counter_open_process(
    uint32_t type,
    uint64_t config,
    int      group_fd
);


/**
 * @brief Close a perf event file descriptor.
 *
 * Closing an invalid descriptor is ignored by this interface.
 *
 * @param fd
 *     File descriptor returned by perf_counter_open_process().
 *
 * @note
 *     This function is NULL-independent because it operates directly
 *     on a file descriptor.
 */
void perf_counter_close(int fd);


#endif /* PROFILING_PERF_EVENT_WRAPPER_H */