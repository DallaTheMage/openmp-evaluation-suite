#!/usr/bin/env bash

#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Copyright (C) 2026 Lorenzo Dall'Ara
#
# This file is part of the OpenMP compiler-agnostic benchmark suite,
# developed as part of a Bachelor's thesis at the
# Alma Mater Studiorum - University of Bologna.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#

detect_default_compiler() {
    if command -v cc &>/dev/null; then
        # Resolve symlink if cc points to gcc/clang
        local real_cc
        real_cc=$(readlink -f "$(command -v cc)" 2>/dev/null || command -v cc)
        if [[ "$real_cc" == *clang* ]]; then
            echo "clang"
            return
        fi
    fi

    if command -v gcc &>/dev/null; then
        echo "gcc"
    elif command -v clang &>/dev/null; then
        echo "clang"
    else
        echo "gcc"
    fi
}

# Number of available logical CPUs (respects cgroup limits when possible).
detect_cores() {
    local n=""

    if [[ -r /sys/fs/cgroup/cpu.max ]]; then
        local quota period
        read -r quota period < /sys/fs/cgroup/cpu.max || true

        if [[ "${quota:-max}" != "max" && "${period:-0}" -gt 0 ]]; then
            n=$(( (quota + period - 1) / period ))
        fi

    elif [[ -r /sys/fs/cgroup/cpu/cpu.cfs_quota_us &&
            -r /sys/fs/cgroup/cpu/cpu.cfs_period_us ]]; then

        local quota period
        quota=$(< /sys/fs/cgroup/cpu/cpu.cfs_quota_us)
        period=$(< /sys/fs/cgroup/cpu/cpu.cfs_period_us)

        if (( quota > 0 && period > 0 )); then
            n=$(( (quota + period - 1) / period ))
        fi
    fi

    if [[ -z "$n" || "$n" -lt 1 ]]; then
        n=$(nproc 2>/dev/null ||
            getconf _NPROCESSORS_ONLN 2>/dev/null ||
            sysctl -n hw.ncpu 2>/dev/null ||
            echo 4)
    fi

    (( n < 1 )) && n=1

    printf '%s\n' "$n"
}

# Detect the largest data/unified cache at the highest cache level.
#
# Returns the cache size in bytes.
# Returns 0 when cache detection is unavailable.
detect_llc_bytes() {
    local cache_dir
    local level type size
    local cache_bytes unit

    local max_level=0
    local max_bytes=0

    for cache_dir in /sys/devices/system/cpu/cpu[0-9]*/cache/index*; do
        [[ -d "$cache_dir" ]] || continue

        [[ -r "$cache_dir/level" &&
           -r "$cache_dir/type" &&
           -r "$cache_dir/size" ]] || continue

        level=$(< "$cache_dir/level")
        type=$(< "$cache_dir/type")
        size=$(< "$cache_dir/size")

        # Ignore instruction-only caches.
        [[ "$type" == "Data" || "$type" == "Unified" ]] || continue

        [[ "$level" =~ ^[0-9]+$ ]] || continue

        # Convert the sysfs cache size to bytes.
        if [[ "$size" =~ ^([0-9]+)([KMG]?)$ ]]; then
            cache_bytes="${BASH_REMATCH[1]}"
            unit="${BASH_REMATCH[2]}"

            case "$unit" in
                K)
                    cache_bytes=$((cache_bytes * 1024))
                    ;;
                M)
                    cache_bytes=$((cache_bytes * 1024 * 1024))
                    ;;
                G)
                    cache_bytes=$((cache_bytes * 1024 * 1024 * 1024))
                    ;;
                "")
                    ;;
            esac
        else
            continue
        fi

        # Keep the highest cache level encountered.
        # For that level, keep the largest reported capacity.
        if (( level > max_level )); then
            max_level="$level"
            max_bytes="$cache_bytes"

        elif (( level == max_level && cache_bytes > max_bytes )); then
            max_bytes="$cache_bytes"
        fi
    done

    printf '%s\n' "$max_bytes"
}

# Available RAM in MiB; takes cgroup limits into account when present.
detect_mem_mib() {
    local mem_kib=""

    if [[ -r /proc/meminfo ]]; then
        mem_kib=$(awk '/^MemAvailable:/ {print $2; exit}' /proc/meminfo)
    fi

    if [[ -r /sys/fs/cgroup/memory.max ]]; then
        local limit usage cgroup_kib

        limit=$(< /sys/fs/cgroup/memory.max)
        usage=$(cat /sys/fs/cgroup/memory.current 2>/dev/null || echo 0)

        if [[ "$limit" =~ ^[0-9]+$ ]]; then
            cgroup_kib=$(( (limit - usage) / 1024 ))

            [[ -z "$mem_kib" || "$cgroup_kib" -lt "$mem_kib" ]] &&
                mem_kib="$cgroup_kib"
        fi

    elif [[ -r /sys/fs/cgroup/memory/memory.limit_in_bytes &&
            -r /sys/fs/cgroup/memory/memory.usage_in_bytes ]]; then

        local limit usage cgroup_kib

        limit=$(< /sys/fs/cgroup/memory/memory.limit_in_bytes)
        usage=$(cat /sys/fs/cgroup/memory/memory.usage_in_bytes 2>/dev/null || echo 0)

        if (( limit > 0 && limit < 9000000000000000000 )); then
            cgroup_kib=$(( (limit - usage) / 1024 ))

            [[ -z "$mem_kib" || "$cgroup_kib" -lt "$mem_kib" ]] &&
                mem_kib="$cgroup_kib"
        fi
    fi

    if [[ ! "$mem_kib" =~ ^[0-9]+$ || "$mem_kib" -lt 1 ]]; then
        mem_kib=4194304
    fi

    printf '%s\n' "$(( mem_kib / 1024 ))"
}
