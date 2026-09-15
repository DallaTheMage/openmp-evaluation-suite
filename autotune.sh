#!/usr/bin/env bash

OUTPUT_FILE="autotune.conf"

# -----------------------------------------------------------------------------
# 1. RILEVAMENTO HARDWARE AUTOMATICO (Auto-tuning)
# -----------------------------------------------------------------------------
# Thread logici disponibili
LOGICAL_CORES=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# RAM disponibile in KiB (con fallback)
if [ -f /proc/meminfo ]; then
    MEM_FREE_KB=$(awk '/MemAvailable/ {print $2}' /proc/meminfo)
    : "${MEM_FREE_KB:=$(awk '/MemFree/ {print $2}' /proc/meminfo)}"
else
    MEM_FREE_KB=4096000
fi

# Calcola MAX_PROBLEM_SIZE basandosi sul 50% della RAM libera
# Oggetti: double (8 byte). 2^N * 8 <= Bytes disponibili
RAM_TARGET_BYTES=$(( (MEM_FREE_KB * 1024) / 2 ))
MAX_PROBLEM_SIZE=20 # Fallback default 2^20 (8MB)

for exp in $(seq 10 32); do
    BYTES_NEEDED=$(( (1 << exp) * 8 ))
    if [ "$BYTES_NEEDED" -le "$RAM_TARGET_BYTES" ]; then
        MAX_PROBLEM_SIZE=$exp
    else
        break
    fi
done

# -----------------------------------------------------------------------------
# 2. GENERAZIONE DEL FILE DI CONFIGURAZIONE (.conf)
# -----------------------------------------------------------------------------
cat << EOF > "$OUTPUT_FILE"
# =============================================================================
# AUTO-GENERATED TUNE CONFIGURATION
# System: $(hostname 2>/dev/null || echo "Unknown")
# Logic Cores: ${LOGICAL_CORES} | Auto Max Problem Size: 2^${MAX_PROBLEM_SIZE}
# =============================================================================

# --- THREAD CONFIGURATION ---
CUSTOM_THREAD_NUMBERS=${LOGICAL_CORES}

# --- PROBLEM SIZES (2^N) ---
MAX_PROBLEM_SIZE=${MAX_PROBLEM_SIZE}
WORK_SIZE_SLICES=3

# --- CHUNK SIZES & SCHEDULE ---
CHOSEN_SCHEDULE=static
DEFAULT_CHUNKSIZE=32

# --- BENCHMARK ITERATIONS ---
WARMUP_REPS=1
WORK_REPS=5
SLOWDOWN_REPS=0

# --- ENGINE CONFIGURATION ---
RNG_TYPE=2
LOGGING_ENABLED=1
FILETYPE=1
COMPILERNAME=gcc
EOF

echo "[+] Configurazione generata con successo in: $OUTPUT_FILE"
echo "    - Thread rilevati: $LOGICAL_CORES"
echo "    - Max Problem Size: 2^$MAX_PROBLEM_SIZE ($(( (1 << MAX_PROBLEM_SIZE) * 8 / 1024 / 1024 )) MB)"