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

    /* Configurazione tipi ed eventi perf per ciascun contatore */
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

    /* Apertura del primo contatore come Group Leader */
    hw->fds[0] = perf_counter_open_process(types[0], configs[0], -1);
    if (hw->fds[0] < 0) {
        return -1;
    }
    hw->leader_fd = hw->fds[0];

    /* Apertura dei contatori secondari vincolati al Group Leader */
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

    /* Reset e abilitazione atomica del gruppo tramite il leader_fd */
    ioctl(hw->leader_fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
    ioctl(hw->leader_fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
    return 0;
}

int hw_counters_stop(HWCounters *hw, HWCountersData *data) {
    if (!hw || !hw->is_initialized || !data) return -1;

    /* Disabilitazione atomica del gruppo */
    ioctl(hw->leader_fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);

    /* Lettura dei valori dai singoli file descriptor */
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
