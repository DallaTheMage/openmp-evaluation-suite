#ifndef PERF_EVENT_WRAPPER_H
#define PERF_EVENT_WRAPPER_H

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>

/* Wrapper diretto per la syscall perf_event_open (assente in glibc) */
static inline int sys_perf_event_open(struct perf_event_attr *hw_event,
                                      pid_t pid,
                                      int cpu,
                                      int group_fd,
                                      unsigned long flags) {
    return (int)syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

/* Gestore generico di un contatore perf_event */
typedef struct PerfCounterFD {
    int fd;
    uint64_t type;
    uint64_t config;
} PerfCounterFD;

/**
 * Apre un contatore perf per il processo corrente e i suoi thread figli (OpenMP).
 *
 * @param type     Tipo di evento (es. PERF_TYPE_HARDWARE, PERF_TYPE_HW_CACHE)
 * @param config   Configurazione specifica dell'evento
 * @param group_fd File descriptor del leader di gruppo (-1 se questo è il leader)
 * @return File descriptor del contatore aperto, oppure < 0 in caso di errore.
 */
int perf_counter_open_process(uint32_t type, uint64_t config, int group_fd);

/**
 * Chiude in sicurezza un file descriptor di un contatore perf.
 */
void perf_counter_close(int fd);

#endif /* PERF_EVENT_WRAPPER_H */