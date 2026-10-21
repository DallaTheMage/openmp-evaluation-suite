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
 * @file Profiler.c
 * @brief Performance measurement coordination for benchmark executions.
 */


#include <omp.h>
#include <string.h>
#include <math.h>
#include "profiling/Profiler.h"
#include "config/params.h"


int profiler_init(Profiler *p) {
    #if ENABLE_HW_COUNTERS
    int hw_status;
    #endif
    int rapl_status;
    if (p == NULL) {
        return -1;
    }
    memset(p, 0, sizeof(*p));

    #if ENABLE_HW_COUNTERS
    hw_status = hw_counters_init(&p->hw);
    if (hw_status != 0) {
        hw_counters_cleanup(&p->hw);
    }
    #endif

    rapl_status = rapl_power_init(&p->rapl);
    if (rapl_status != 0) {
        rapl_power_cleanup(&p->rapl);
    }

    p->is_running = false;
    return 0;
}


int profiler_start(Profiler *p) {
    if (p == NULL || p->is_running) {
        return -1;
    }

    p->start_wtime = omp_get_wtime();

    if (p->hw.is_initialized && hw_counters_start(&p->hw) != 0) {
        hw_counters_cleanup(&p->hw);
    }

    if (p->rapl.is_supported && rapl_power_start(&p->rapl) != 0) {
        rapl_power_cleanup(&p->rapl);
    }

    p->is_running = true;
    return 0;
}


int profiler_stop(Profiler *p, PerformanceMetric *metric) {
    const double stop_wtime = omp_get_wtime();
    bool hw_valid = false;
    bool pkg_valid = false;
    bool dram_valid = false;
    HWCountersData hw_data  = {0};
    RaplPowerData rapl_data = {0};

    if (p == NULL || !p->is_running || metric == NULL) {
        return -1;
    }

    if (p->hw.is_initialized) {
        if (hw_counters_stop(&p->hw, &hw_data) != 0) {
            hw_counters_cleanup(&p->hw);
        } else {
            hw_valid = true;
        }
    }

    if (p->rapl.is_supported) {
        if (rapl_power_stop(&p->rapl, &rapl_data) != 0) {
            rapl_power_cleanup(&p->rapl);
        } else {
            pkg_valid = true;
            dram_valid = (p->rapl.fd_dram >= 0) && isfinite(rapl_data.energy_dram_joules);
        }
    }

    p->is_running = false;

    memset(metric, 0, sizeof(*metric));
    metric->wall_time_sec = stop_wtime - p->start_wtime;
    metric->cycles        = hw_data.cycles;
    metric->instructions  = hw_data.instructions;
    metric->l1d_misses    = hw_data.l1d_misses;
    metric->llc_misses    = hw_data.llc_misses;

    metric->ipc = (metric->cycles > 0U)
        ? (double)metric->instructions / (double)metric->cycles
        : 0.0;

    metric->l1d_miss_ratio = (metric->instructions > 0U)
        ? (double)metric->l1d_misses / (double)metric->instructions
        : 0.0;

    metric->llc_miss_ratio = (metric->instructions > 0U)
        ? (double)metric->llc_misses / (double)metric->instructions
        : 0.0;

    metric->energy_dram_joules = dram_valid ? rapl_data.energy_dram_joules : NAN;
    metric->energy_pkg_joules  = pkg_valid ? rapl_data.energy_pkg_joules : NAN;
    metric->hw_counters_valid  = hw_valid;
    metric->energy_pkg_valid   = pkg_valid;
    metric->energy_dram_valid  = dram_valid;
    if (!hw_valid) {
        metric->ipc            = NAN;
        metric->l1d_miss_ratio = NAN;
        metric->llc_miss_ratio = NAN;
    }
    return 0;
}


void profiler_cleanup(Profiler *p) {
    if (p == NULL) {
        return;
    }

    hw_counters_cleanup(&p->hw);
    rapl_power_cleanup(&p->rapl);
    p->is_running = false;
}
