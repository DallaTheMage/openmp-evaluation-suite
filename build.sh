#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_CONF="${SCRIPT_DIR}/default.conf"
CONF_FILE="$DEFAULT_CONF"
CLI_PARAMS=()

usage() {
    cat <<'HELP'
Uso: ./build.sh [file.conf] [KEY=VALUE ...] [opzioni]

  ./build.sh                         usa default.conf
  ./build.sh autotune.conf           usa la configurazione specificata
  ./build.sh --config autotune.conf  equivalente
  ./build.sh WORK_REPS=10            override di una macro
  ./build.sh autotune.conf WORK_REPS=10

Opzioni:
  -c, --config FILE   file di configurazione da usare
  -h, --help          mostra questo messaggio

Gli override KEY=VALUE prevalgono sui valori del file. La variabile d'ambiente
CC prevale su COMPILERNAME nel file di configurazione.
HELP
}

while (($#)); do
    case "$1" in
        -h|--help) usage; exit 0 ;;
        -c|--config)
            [[ $# -ge 2 ]] || { echo "[-] Manca il file dopo $1" >&2; exit 2; }
            CONF_FILE="$2"; shift 2 ;;
        *.conf) CONF_FILE="$1"; shift ;;
        *=*) CLI_PARAMS+=("$1"); shift ;;
        *) echo "[-] Argomento non riconosciuto: $1" >&2; usage >&2; exit 2 ;;
    esac
done

# Per il default usa quello accanto allo script; per gli altri file prova anche la directory corrente.
if [[ ! -f "$CONF_FILE" && -f "${SCRIPT_DIR}/${CONF_FILE}" ]]; then CONF_FILE="${SCRIPT_DIR}/${CONF_FILE}"; fi
if [[ ! -f "$CONF_FILE" ]]; then
    echo "[-] File di configurazione non trovato: $CONF_FILE" >&2
    exit 2
fi

declare -A MACROS=()
SELECTED_CC=""
trim() {
    local value="$1"
    value="${value#"${value%%[!$' \t\r\n']*}"}"
    value="${value%"${value##*[!$' \t\r\n']}"}"
    printf '%s' "$value"
}

echo "[+] Caricamento configurazione: $CONF_FILE"
while IFS= read -r line || [[ -n "$line" ]]; do
    line="${line%%$'\r'}"
    line="$(trim "$line")"
    [[ -z "$line" || "$line" == \#* ]] && continue
    [[ "$line" == *=* ]] || { echo "[-] Riga non valida in $CONF_FILE: $line" >&2; exit 2; }
    key="$(trim "${line%%=*}")"
    value="$(trim "${line#*=}")"
    [[ "$key" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]] || { echo "[-] Nome parametro non valido: $key" >&2; exit 2; }
    [[ -n "$value" ]] || { echo "[-] Valore vuoto per $key" >&2; exit 2; }
    if [[ "$key" == COMPILERNAME ]]; then SELECTED_CC="$value"; else MACROS["$key"]="$value"; fi
done < "$CONF_FILE"

for param in "${CLI_PARAMS[@]}"; do
    key="$(trim "${param%%=*}")"
    value="$(trim "${param#*=}")"
    [[ "$key" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]] || { echo "[-] Nome parametro non valido: $key" >&2; exit 2; }
    [[ -n "$value" ]] || { echo "[-] Valore vuoto per $key" >&2; exit 2; }
    if [[ "$key" == COMPILERNAME ]]; then SELECTED_CC="$value"; else MACROS["$key"]="$value"; fi
    echo "[->] Override CLI: $key=$value"
done

# Compatibilità con vecchie configurazioni che usavano CHOSEN_SCHEDULE=static/dynamic/guided.
if [[ -n "${MACROS[CHOSEN_SCHEDULE]:-}" ]]; then
    case "${MACROS[CHOSEN_SCHEDULE]}" in
        static) MACROS[CHOSEN_SCHEDULE_ID]=1U ;;
        dynamic) MACROS[CHOSEN_SCHEDULE_ID]=2U ;;
        guided) MACROS[CHOSEN_SCHEDULE_ID]=3U ;;
        *) echo "[-] CHOSEN_SCHEDULE deve essere static, dynamic o guided" >&2; exit 2 ;;
    esac
    unset 'MACROS[CHOSEN_SCHEDULE]'
fi

CC_COMPILER="${CC:-${SELECTED_CC:-gcc}}"
CFLAGS_ARRAY=(-g -Wall -Wextra -Wpedantic -std=c99 -D_GNU_SOURCE -Iinclude -Isrc/microroutines -fopenmp -MMD -MP)
for key in "${!MACROS[@]}"; do
    value="${MACROS[$key]}"
    [[ "$value" != *$'\n'* ]] || { echo "[-] Valore multilinea non ammesso per $key" >&2; exit 2; }
    CFLAGS_ARRAY+=("-D${key}=${value}")
done
CFLAGS_VALUE=""
for flag in "${CFLAGS_ARRAY[@]}"; do CFLAGS_VALUE+="${CFLAGS_VALUE:+ }${flag}"; done

echo "[+] Compilatore: $CC_COMPILER"
echo "[+] Configurazione: $CONF_FILE"
echo "[+] Macro: ${#MACROS[@]}"
make -C "$SCRIPT_DIR" CC="$CC_COMPILER" CFLAGS="$CFLAGS_VALUE"
echo "[+] Compilazione completata: build/$(basename -- "$CC_COMPILER")/main"
