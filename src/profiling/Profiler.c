#include "profiling/Profiler.h"
#include <omp.h>
#include <string.h>

int profiler_init(Profiler *p) {
    if (!p) return -1;

    memset(p, 0, sizeof(Profiler));

    if (hw_counters_init(&p->hw) != 0) {
        return -1;
    }

    if (rapl_power_init(&p->rapl) != 0) {
        hw_counters_cleanup(&p->hw);
        return -2;
    }

    p->is_running = false;
    return 0;
}

void profiler_start(Profiler *p) {
    if (!p || p->is_running) return;

    hw_counters_start(&p->hw);
    rapl_power_start(&p->rapl);

    p->start_wtime = omp_get_wtime();
    p->is_running = true;
}

void profiler_stop(Profiler *p, PerformanceMetric *metric) {
    double stop_wtime = omp_get_wtime();

    if (!p || !p->is_running || !metric) return;

    /* Struct di supporto per la lettura in blocco dei sottosistemi */
    HWCountersData hw_data = {0};
    RaplPowerData  rapl_data = {0};

    hw_counters_stop(&p->hw, &hw_data);
    rapl_power_stop(&p->rapl, &rapl_data);
    p->is_running = false;

    /* Misurazione del tempo */
    metric->wall_time_sec = stop_wtime - p->start_wtime;

    /* Assegnazione metriche grezze */
    metric->cycles       = hw_data.cycles;
    metric->instructions = hw_data.instructions;
    metric->l1d_misses   = hw_data.l1d_misses;
    metric->llc_misses   = hw_data.llc_misses;

    /* Calcolo metriche derivate */
    metric->ipc = (metric->cycles > 0)
        ? ((double)metric->instructions / (double)metric->cycles)
        : 0.0;

    metric->l1d_miss_ratio = (metric->instructions > 0)
        ? ((double)metric->l1d_misses / (double)metric->instructions)
        : 0.0;

    metric->llc_miss_ratio = (metric->instructions > 0)
        ? ((double)metric->llc_misses / (double)metric->instructions)
        : 0.0;

    /* Assegnazione consumo energetico */
    metric->energy_pkg_joules  = rapl_data.energy_pkg_joules;
    metric->energy_dram_joules = rapl_data.energy_dram_joules;
}

void profiler_cleanup(Profiler *p) {
    if (!p) return;

    hw_counters_cleanup(&p->hw);
    rapl_power_cleanup(&p->rapl);
    p->is_running = false;
}
