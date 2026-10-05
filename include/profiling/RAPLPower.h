/* RAPLPower.h */
#ifndef RAPL_POWER_H
#define RAPL_POWER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct RaplPower {
    int    fd_pkg;
    int    fd_dram;
    double scale_pkg;
    double scale_dram;
    double start_energy_pkg_uj;
    double start_energy_dram_uj;
    bool   is_supported;
} RaplPower;

typedef struct RaplPowerData {
    double energy_pkg_joules;
    double energy_dram_joules;
} RaplPowerData;

int  rapl_power_init(RaplPower *rapl);
int  rapl_power_start(RaplPower *rapl);
int  rapl_power_stop(RaplPower *rapl, RaplPowerData *data);
void rapl_power_cleanup(RaplPower *rapl);

#endif /* RAPL_POWER_H */