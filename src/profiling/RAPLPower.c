/* RAPLPower.c */
#include "profiling/RAPLPower.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define RAPL_PKG_PATH "/sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj"
#define RAPL_DRAM_PATH "/sys/class/powercap/intel-rapl/intel-rapl:0/intel-rapl:0:0/energy_uj"

static double read_energy_uj(int fd) {
    if (fd < 0) return 0.0;
    char buffer[32];
    lseek(fd, 0, SEEK_SET);
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) return 0.0;
    buffer[bytes_read] = '\0';
    return strtod(buffer, NULL);
}

int rapl_power_init(RaplPower *rapl) {
    if (!rapl) return -1;
    memset(rapl, 0, sizeof(RaplPower));

    rapl->fd_pkg = open(RAPL_PKG_PATH, O_RDONLY);
    if (rapl->fd_pkg < 0) {
        rapl->is_supported = false;
        return -1;
    }

    rapl->fd_dram = open(RAPL_DRAM_PATH, O_RDONLY);
    rapl->scale_pkg = 1e-6;
    rapl->scale_dram = 1e-6;
    rapl->is_supported = true;
    return 0;
}

int rapl_power_start(RaplPower *rapl) {
    if (!rapl || !rapl->is_supported) return -1;
    rapl->start_energy_pkg_uj  = read_energy_uj(rapl->fd_pkg);
    rapl->start_energy_dram_uj = (rapl->fd_dram >= 0) ? read_energy_uj(rapl->fd_dram) : 0.0;
    return 0;
}

int rapl_power_stop(RaplPower *rapl, RaplPowerData *data) {
    if (!rapl || !rapl->is_supported || !data) return -1;

    double stop_pkg  = read_energy_uj(rapl->fd_pkg);
    double stop_dram = (rapl->fd_dram >= 0) ? read_energy_uj(rapl->fd_dram) : 0.0;

    double delta_pkg  = (stop_pkg >= rapl->start_energy_pkg_uj)
                        ? (stop_pkg - rapl->start_energy_pkg_uj) : 0.0;
    double delta_dram = (stop_dram >= rapl->start_energy_dram_uj)
                        ? (stop_dram - rapl->start_energy_dram_uj) : 0.0;

    data->energy_pkg_joules  = delta_pkg * rapl->scale_pkg;
    data->energy_dram_joules = delta_dram * rapl->scale_dram;
    return 0;
}

void rapl_power_cleanup(RaplPower *rapl) {
    if (!rapl) return;
    if (rapl->fd_pkg >= 0) close(rapl->fd_pkg);
    if (rapl->fd_dram >= 0) close(rapl->fd_dram);
    rapl->is_supported = false;
}