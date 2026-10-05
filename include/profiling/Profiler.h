#ifndef PROFILER_H
#define PROFILER_H

#include <stdint.h>
#include <stdbool.h>
#include "profiling/HWCounters.h"
#include "profiling/RAPLPower.h"

/* Risultati hardware della singola esecuzione */
typedef struct PerformanceMetric {
    /* Tempo effettivo di esecuzione (Wall-Clock) */
    double   wall_time_sec;

    /* Metriche Hardware CPU */
    uint64_t cycles;
    uint64_t instructions;
    uint64_t l1d_misses;
    uint64_t llc_misses;

    /* Metriche derivate */
    double   ipc;
    double   l1d_miss_ratio;
    double   llc_miss_ratio; /* Miss ratio per la Last Level Cache (L3) */

    /* Consumo Energetico (RAPL) */
    double   energy_pkg_joules;
    double   energy_dram_joules;
} PerformanceMetric;

/* Stato interno del Profiler */
typedef struct Profiler {
    HWCounters hw;
    RaplPower  rapl;
    double     start_wtime;
    bool       is_running;
} Profiler;

/* Interfaccia per i Benchmark */
int  profiler_init(Profiler *p);
void profiler_start(Profiler *p);
void profiler_stop(Profiler *p, PerformanceMetric *metric);
void profiler_cleanup(Profiler *p);

#endif /* PROFILER_H */