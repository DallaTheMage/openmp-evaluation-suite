#!/usr/bin/env bash
set -Eeuo pipefail

# Usage: ./autotune.sh [output.conf]
OUTPUT_FILE="${1:-autotune.conf}"

# Detect system default compiler (fallback to gcc if not found)
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

# ---------------------------------------------------------------------------
# System detection
# ---------------------------------------------------------------------------

LOGICAL_CORES=$(detect_cores)
AVAILABLE_MEM_MIB=$(detect_mem_mib)
DEFAULT_COMPILER=$(detect_default_compiler)
DETECTED_LLC_SIZE_BYTES=$(detect_llc_bytes)

# ---------------------------------------------------------------------------
# Problem-size selection
# ---------------------------------------------------------------------------

# Use a conservative data budget (25% of currently available RAM).
# The estimate reserves 64 bytes per logical element for the data arrays used
# by the benchmark. Derive all workload sizes from this same budget.
BUDGET_BYTES=$(( AVAILABLE_MEM_MIB * 1024 * 1024 / 4 ))
BUDGET_ELEMENTS=$(( BUDGET_BYTES / 64 ))

if (( BUDGET_ELEMENTS < 1 )); then
    printf '[ERROR] Available memory is too low for the minimum workload.\n' >&2
    exit 1
fi

# Find the largest representable power-of-two element count that fits the
# budget. Bash arithmetic is signed, so do not shift into bit 63 or beyond.
MAX_PROBLEM_SIZE=0
for (( exp=1; exp<=60; exp++ )); do
    if (( (1 << exp) <= BUDGET_ELEMENTS )); then
        MAX_PROBLEM_SIZE=$exp
    else
        break
    fi
done

# ---------------------------------------------------------------------------
# Thread-list selection
# ---------------------------------------------------------------------------

THREAD_LIST="1"
threads=1

while (( threads * 2 < LOGICAL_CORES )); do
    threads=$(( threads * 2 ))
    THREAD_LIST+=",${threads}"
done

if (( LOGICAL_CORES > 1 )); then
    THREAD_LIST+=",${LOGICAL_CORES}"
fi

# ---------------------------------------------------------------------------
# Per-thread workload
# ---------------------------------------------------------------------------

# Keep proportional workloads valid for the largest configured thread count:
# SIZE_PER_THREAD * LOGICAL_CORES must fit in the original buffer and budget.
# Choose the largest power of two that satisfies this constraint; there is no
# arbitrary minimum or fixed initial workload size.
per_thread_limit=$(( (1 << MAX_PROBLEM_SIZE) / LOGICAL_CORES ))
if (( per_thread_limit < 1 )); then
    printf '[ERROR] Memory budget cannot accommodate one element per thread.\n' >&2
    exit 1
fi

SIZE_PER_THREAD=1
for (( exp=1; exp<=62; exp++ )); do
    if (( (1 << exp) <= per_thread_limit )); then
        SIZE_PER_THREAD=$(( 1 << exp ))
    else
        break
    fi
done

# ---------------------------------------------------------------------------
# Window-size selection
# ---------------------------------------------------------------------------

WINDOW_MAX=$MAX_PROBLEM_SIZE
WINDOW_MIN=16

# For small-memory systems, start at the largest valid exponent instead of
# emitting windows larger than the configured problem buffer.
(( WINDOW_MAX < WINDOW_MIN )) && WINDOW_MIN="$WINDOW_MAX"

WINDOWS_LOG2_LIST=""

for (( exp=WINDOW_MIN; exp<=WINDOW_MAX; exp+=2 )); do
    [[ -n "$WINDOWS_LOG2_LIST" ]] && WINDOWS_LOG2_LIST+=","
    WINDOWS_LOG2_LIST+="${exp}U"
done

[[ -n "$WINDOWS_LOG2_LIST" ]] || WINDOWS_LOG2_LIST="${WINDOW_MAX}U"

# ---------------------------------------------------------------------------
# Generate configuration file
# ---------------------------------------------------------------------------

mkdir -p -- "$(dirname -- "$OUTPUT_FILE")"

cat > "$OUTPUT_FILE" <<CONFIG
# =============================================================================
# Configuration automatically generated by autotune.sh
# Host: $(hostname 2>/dev/null || echo unknown)
# Available logical CPUs: ${LOGICAL_CORES}
# Estimated available RAM: ${AVAILABLE_MEM_MIB} MiB
# Conservative data budget: approx 25% of available RAM (64 bytes/element)
# =============================================================================

COMPILERNAME=${DEFAULT_COMPILER}
THREAD_LIST=${THREAD_LIST}
PROBLEM_LOG2_SIZE=${MAX_PROBLEM_SIZE}U
SIZE_PER_THREAD=${SIZE_PER_THREAD}ULL
MEMORY_ALIGNMENT=64U

# Detected last-level data/unified cache size, in bytes.
# Zero means cache detection was unavailable.
DETECTED_LLC_SIZE_BYTES=${DETECTED_LLC_SIZE_BYTES}ULL

# OpenMP: 1=static, 2=dynamic, 3=guided
CHOSEN_SCHEDULE_ID=1U
CHUNK_SIZE_LIST=64U

RUN_FULL_SCALE=1
RUN_PROPORTIONAL=1
WARMUP_REPS=3U
WORK_REPS=5U
SLOWDOWN_FACTOR=0U
WINDOWS_LOG2_LIST=${WINDOWS_LOG2_LIST}
GENERATION_SEED=12345ULL
CONFIG

# ---------------------------------------------------------------------------
# Report selected configuration
# ---------------------------------------------------------------------------

echo "[+] Generated configuration: $OUTPUT_FILE"
echo "    Compiler: ${DEFAULT_COMPILER}"
echo "    Logical cores: ${LOGICAL_CORES}"
echo "    Estimated available RAM: ${AVAILABLE_MEM_MIB} MiB"

if (( DETECTED_LLC_SIZE_BYTES > 0 )); then
    echo "    Detected LLC: $((DETECTED_LLC_SIZE_BYTES / 1024 / 1024)) MiB"
    echo "    Auxiliary buffer target: $(((DETECTED_LLC_SIZE_BYTES * 2) / 1024 / 1024)) MiB (2x LLC)"
else
    echo "    Detected LLC: unavailable"
    echo "    Auxiliary buffer target: 0 elements"
fi

echo "    Main size: 2^${MAX_PROBLEM_SIZE} elements"
echo "    Tested threads: ${THREAD_LIST}"