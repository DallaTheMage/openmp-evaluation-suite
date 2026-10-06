#include "profiling/PerfEventWrapper.h"
#include <string.h>
#include <unistd.h>

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
