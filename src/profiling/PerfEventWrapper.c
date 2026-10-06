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

    /* Disabilitato all'avvio: verrà abilitato esplicitamente da hw_counters_start() */
    pe.disabled = 1;

    /*
     * TRACCIAMENTO OPENMP / MULTI-THREAD:
     * inherit = 1 fa sì che tutti i thread figli creati da OpenMP (#pragma omp parallel)
     * ereditino automaticamente questo contatore e accumulino le metriche nello stesso FD.
     */
    pe.inherit = 1;

    /* Esclude il kernel e l'hypervisor per misurare solo lo spazio utente (il codice benchmark) */
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    /*
     * pid = 0: Monitora il processo corrente (e i suoi figli grazie a inherit = 1)
     * cpu = -1: Monitora su qualsiasi CPU su cui vengono schedulati i thread
     */
    return sys_perf_event_open(&pe, 0, -1, group_fd, 0);
}

void perf_counter_close(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}
