#ifndef HW_COUNTERS_H
#define HW_COUNTERS_H

#include <stdint.h>
#include <stdbool.h>
#include "profiling/PerfEventWrapper.h"

/* Enum degli eventi Hardware monitorati */
typedef enum HWPerfEventType {
    HW_PERF_CYCLES = 0,
    HW_PERF_INSTRUCTIONS,
    HW_PERF_L1D_MISSES,
    HW_PERF_LLC_MISSES,
    HW_PERF_COUNT_MAX
} HWPerfEventType;

/* Struttura contenente i File Descriptor dei contatori CPU */
typedef struct HWCounters {
    int fds[HW_PERF_COUNT_MAX];
    int leader_fd;
    bool is_initialized;
} HWCounters;

/* Dati grezzi dei contatori hardware */
typedef struct HWCountersData {
    uint64_t cycles;
    uint64_t instructions;
    uint64_t l1d_misses;
    uint64_t llc_misses;
} HWCountersData;

/* Inizializzazione, gestione ciclo di vita e lettura */
int hw_counters_init(HWCounters *hw);
int hw_counters_start(HWCounters *hw);
int hw_counters_stop(HWCounters *hw, HWCountersData *data);
void hw_counters_cleanup(HWCounters *hw);

#endif /* HW_COUNTERS_H */