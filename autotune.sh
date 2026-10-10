#!/usr/bin/env bash
set -Eeuo pipefail

# Uso: ./autotune.sh [output.conf]
OUTPUT_FILE="${1:-autotune.conf}"

# Numero di CPU logiche disponibili (rispetta, quando possibile, i limiti cgroup).
detect_cores() {
    local n=""
    if [[ -r /sys/fs/cgroup/cpu.max ]]; then
        local quota period
        read -r quota period < /sys/fs/cgroup/cpu.max || true
        if [[ "${quota:-max}" != "max" && "${period:-0}" -gt 0 ]]; then
            n=$(( (quota + period - 1) / period ))
        fi
    elif [[ -r /sys/fs/cgroup/cpu/cpu.cfs_quota_us && -r /sys/fs/cgroup/cpu/cpu.cfs_period_us ]]; then
        local quota period
        quota=$(< /sys/fs/cgroup/cpu/cpu.cfs_quota_us)
        period=$(< /sys/fs/cgroup/cpu/cpu.cfs_period_us)
        if (( quota > 0 && period > 0 )); then n=$(( (quota + period - 1) / period )); fi
    fi
    if [[ -z "$n" || "$n" -lt 1 ]]; then
        n=$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    fi
    (( n < 1 )) && n=1
    printf '%s\n' "$n"
}

# RAM disponibile in MiB; tiene conto del limite cgroup quando presente.
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
            [[ -z "$mem_kib" || "$cgroup_kib" -lt "$mem_kib" ]] && mem_kib="$cgroup_kib"
        fi
    elif [[ -r /sys/fs/cgroup/memory/memory.limit_in_bytes ]]; then
        local limit usage cgroup_kib
        limit=$(< /sys/fs/cgroup/memory/memory.limit_in_bytes)
        usage=$(cat /sys/fs/cgroup/memory/memory.usage_in_bytes 2>/dev/null || echo 0)
        if (( limit > 0 && limit < 9000000000000000000 )); then
            cgroup_kib=$(( (limit - usage) / 1024 ))
            [[ -z "$mem_kib" || "$cgroup_kib" -lt "$mem_kib" ]] && mem_kib="$cgroup_kib"
        fi
    fi
    if [[ ! "$mem_kib" =~ ^[0-9]+$ || "$mem_kib" -lt 1 ]]; then mem_kib=4194304; fi
    printf '%s\n' "$(( mem_kib / 1024 ))"
}

LOGICAL_CORES=$(detect_cores)
AVAILABLE_MEM_MIB=$(detect_mem_mib)

# Budget circa 25% della RAM disponibile; stima fino a 8 array double (64 byte/elemento).
# Il limite massimo evita workload iniziali eccessivi; aumentarlo manualmente se opportuno.
BUDGET_BYTES=$(( AVAILABLE_MEM_MIB * 1024 * 1024 / 4 ))
MAX_PROBLEM_SIZE=20
for (( exp=20; exp<=30; exp++ )); do
    bytes_needed=$(( (1 << exp) * 64 ))
    if (( bytes_needed <= BUDGET_BYTES )); then MAX_PROBLEM_SIZE=$exp; else break; fi
done

THREAD_LIST="1"
threads=1
while (( threads * 2 < LOGICAL_CORES )); do
    threads=$(( threads * 2 ))
    THREAD_LIST+=",${threads}"
done
if (( LOGICAL_CORES > 1 )); then THREAD_LIST+=",${LOGICAL_CORES}"; fi

SIZE_PER_THREAD=1048576
if (( LOGICAL_CORES > 1 )); then
    per_thread_budget=$(( BUDGET_BYTES / LOGICAL_CORES / 64 ))
    while (( SIZE_PER_THREAD > per_thread_budget && SIZE_PER_THREAD > 65536 )); do
        SIZE_PER_THREAD=$(( SIZE_PER_THREAD / 2 ))
    done
fi

WINDOW_MAX=$MAX_PROBLEM_SIZE
(( WINDOW_MAX > 24 )) && WINDOW_MAX=24
WINDOW_MIN=16
(( WINDOW_MAX < WINDOW_MIN )) && WINDOW_MIN="$WINDOW_MAX"
WINDOWS_LOG2_LIST=""
for (( exp=WINDOW_MIN; exp<=WINDOW_MAX; exp+=2 )); do
    [[ -n "$WINDOWS_LOG2_LIST" ]] && WINDOWS_LOG2_LIST+="," 
    WINDOWS_LOG2_LIST+="${exp}U"
done
[[ -n "$WINDOWS_LOG2_LIST" ]] || WINDOWS_LOG2_LIST="${WINDOW_MAX}U"

mkdir -p -- "$(dirname -- "$OUTPUT_FILE")"
cat > "$OUTPUT_FILE" <<CONFIG
# =============================================================================
# Configurazione generata automaticamente da autotune.sh
# Host: $(hostname 2>/dev/null || echo unknown)
# CPU logiche disponibili: ${LOGICAL_CORES}
# RAM disponibile stimata: ${AVAILABLE_MEM_MIB} MiB
# Budget prudente per i dati: circa il 25% della RAM disponibile
# =============================================================================

COMPILERNAME=gcc
THREAD_LIST=${THREAD_LIST}
PROBLEM_LOG2_SIZE=${MAX_PROBLEM_SIZE}U
SIZE_PER_THREAD=${SIZE_PER_THREAD}ULL
MEMORY_ALIGNMENT=64U

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

echo "[+] Configurazione generata: $OUTPUT_FILE"
echo "    CPU logiche: $LOGICAL_CORES"
echo "    RAM disponibile stimata: ${AVAILABLE_MEM_MIB} MiB"
echo "    Dimensione principale: 2^${MAX_PROBLEM_SIZE} elementi"
echo "    Thread testati: ${THREAD_LIST}"
