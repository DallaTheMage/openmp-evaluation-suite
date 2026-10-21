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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file RAPLPower.c
 * @brief Linux powercap RAPL package and optional DRAM energy readings.
 *
 * Implementation details are kept separate from the public API contracts
 * documented in the corresponding header.

 * @note Wraparound correction assumes at most one counter wrap per interval.
 *       RAPL measurements are restricted to package-0 and optional DRAM.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <dirent.h>
#include <limits.h>
#include <linux/limits.h>

#include "profiling/RAPLPower.h"

static int read_value(int fd, double *value) {
    char buffer[64], *end;
    ssize_t n;
    double parsed;
    if ((fd < 0)|| (value == NULL) || (lseek(fd, 0, SEEK_SET) < 0)) {
        return -1;
    }
    n = read(fd, buffer, sizeof(buffer) - 1);
    if (n <= 0) {
        return -1;
    }
    buffer[n] = '\0';
    errno = 0;
    parsed = strtod(buffer, &end);
    if (errno != 0 || end == buffer || !isfinite(parsed) || parsed < 0) {
        return -1;
    }
    while (*end == ' ' || *end == '\n' || *end == '\t') {
        end++;
    }
    if (*end != '\0') {
        return -1;
    }
    *value = parsed;
    return 0;
}

static int read_path(const char *path, double *value) {
    int fd = open(path, O_RDONLY);
    int status;
    if (fd < 0) {
        return -1;
    }
    status = read_value(fd, value);
    close(fd);
    return status;
}

static int find_named_domain(
    const char *parent,
    const char *expected,
    char *path,
    size_t length) {

    DIR *directory = opendir(parent);
    struct dirent *entry;
    int found = -1;
    if (!directory) {
        return -1;
    }
    while ((entry = readdir(directory)) != NULL && found != 0) {
        char name_path[PATH_MAX];
        char domain_name[80];
        int fd, out_len;
        ssize_t n;
        out_len = snprintf(name_path, sizeof(name_path), "%s/%s/name", parent, entry->d_name);
        if ((strncmp(entry->d_name, "intel-rapl:", 11) == 0) &&
            (out_len < (int)sizeof(name_path))) {
            fd = open(name_path, O_RDONLY);
            if (fd >= 0) {
                n = read(fd, domain_name, sizeof(domain_name) - 1);
                close(fd);
                if (n > 0) {
                    domain_name[n] = '\0';
                    domain_name[strcspn(domain_name, "\r\n")] = '\0';
                    out_len = snprintf(path, length, "%s/%s", parent, entry->d_name);
                    if (strcmp(domain_name, expected) == 0 && out_len < (int)length) {
                        found = 0;
                    }
                }
            }
        }
    }
    closedir(directory);
    return found;
}

static int open_domain(const char *directory, double *maximum) {
    char filename[PATH_MAX];
    int fd, out_len;
    out_len = snprintf(filename, sizeof(filename), "%s/max_energy_range_uj", directory);
    if (out_len >= (int)sizeof(filename) ||
        read_path(filename, maximum) != 0 || *maximum <= 0) {
        return -1;
    }
    out_len = snprintf(filename, sizeof(filename), "%s/energy_uj", directory);
    if (out_len >= (int)sizeof(filename)) {
        return -1;
    }
    fd = open(filename, O_RDONLY);
    return fd;
}

int rapl_power_init(RaplPower *rapl) {
    char pkg[PATH_MAX], dram[PATH_MAX];
    int found_nd;
    if (!rapl) {
        return -1;
    }
    memset(rapl, 0, sizeof(*rapl));
    rapl->fd_pkg = rapl->fd_dram = -1;
    found_nd = find_named_domain(RAPL_ROOT, "package-0", pkg, sizeof(pkg));
    if (found_nd != 0) {
        return -1;
    }
    rapl->fd_pkg = open_domain(pkg, &rapl->max_energy_pkg_uj);
    if (rapl->fd_pkg < 0) {
        return -1;
    }
    found_nd = find_named_domain(pkg, "dram", dram, sizeof(dram));
    if (found_nd == 0) {
        rapl->fd_dram = open_domain(dram, &rapl->max_energy_dram_uj);
    }
    rapl->scale_pkg = rapl->scale_dram = 1e-6;
    rapl->is_supported = true;
    return 0;
}

int rapl_power_start(RaplPower *rapl) {
    if (!rapl || !rapl->is_supported) {
        return -1;
    }
    if (read_value(rapl->fd_pkg, &rapl->start_energy_pkg_uj) != 0) {
        return -1;
    }
    if (rapl->fd_dram >= 0 && read_value(rapl->fd_dram, &rapl->start_energy_dram_uj) != 0) {
        close(rapl->fd_dram);
        rapl->fd_dram = -1;
    }
    return 0;
}

static inline double energy_delta(double start, double stop, double maximum) {
    return (stop >= start) ? (stop - start) : (maximum - start + stop);
}

int rapl_power_stop(RaplPower *rapl, RaplPowerData *data) {
    double stop_pkg, stop_dram, delta;
    if (!rapl || !rapl->is_supported || !data) {
        return -1;
    }
    if (read_value(rapl->fd_pkg, &stop_pkg) != 0) {
        return -1;
    }
    delta = energy_delta(rapl->start_energy_pkg_uj, stop_pkg, rapl->max_energy_pkg_uj);
    data->energy_pkg_joules = delta * rapl->scale_pkg;
    data->energy_dram_joules = NAN;
    if (rapl->fd_dram >= 0) {
        if (read_value(rapl->fd_dram, &stop_dram) == 0) {
            delta = energy_delta(rapl->start_energy_dram_uj, stop_dram, rapl->max_energy_dram_uj);
            data->energy_dram_joules = delta * rapl->scale_dram;
        } else {
            close(rapl->fd_dram);
            rapl->fd_dram = -1;
        }
    }
    return 0;
}

void rapl_power_cleanup(RaplPower *rapl) {
    if (!rapl) {
        return;
    }
    if (rapl->fd_pkg >= 0) {
        close(rapl->fd_pkg);
    }
    if (rapl->fd_dram >= 0) {
        close(rapl->fd_dram);
    }
    rapl->fd_pkg  = -1;
    rapl->fd_dram = -1;
    rapl->is_supported = false;
}
