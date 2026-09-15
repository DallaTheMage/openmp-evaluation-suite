#!/usr/bin/env bash

CONF_FILE="autotune.conf"
CLI_PARAMS=()

# -----------------------------------------------------------------------------
# 1. PARSING DEGLI ARGOMENTI DA RIGA DI COMANDO
# -----------------------------------------------------------------------------
# Riconosce se il primo parametro è un file .conf o un'assegnazione KEY=VALUE
for arg in "$@"; do
    if [[ "$arg" == *.conf ]] && [ -f "$arg" ]; then
        CONF_FILE="$arg"
    elif [[ "$arg" == *=* ]]; then
        CLI_PARAMS+=("$arg")
    fi
done

# Map associativa per gestire la sovrascrittura delle macro (richiede Bash 4+)
declare -A MACROS

# -----------------------------------------------------------------------------
# 2. CARICAMENTO MACRO DAL FILE .CONF
# -----------------------------------------------------------------------------
if [ -f "$CONF_FILE" ]; then
    echo "[+] Caricamento configurazione da: $CONF_FILE"
    while IFS='=' read -r key value || [ -n "$key" ]; do
        key=$(echo "$key" | xargs)
        value=$(echo "$value" | xargs)

        # Salta commenti e righe vuote
        if [[ -z "$key" ]] || [[ "$key" =~ ^# ]]; then
            continue
        fi

        MACROS["$key"]="$value"
    done < "$CONF_FILE"
else
    echo "[!] Nessun file '$CONF_FILE' trovato. Utilizzo solo i parametri passati."
fi

# -----------------------------------------------------------------------------
# 3. SOVRASCRITTURA CON I PARAMETRI DA RIGA DI COMANDO (CLI)
# -----------------------------------------------------------------------------
for param in "${CLI_PARAMS[@]}"; do
    key="${param%%=*}"
    value="${param#*=}"

    key=$(echo "$key" | xargs)
    value=$(echo "$value" | xargs)

    echo "[->] Override riga di comando: $key = $value"
    MACROS["$key"]="$value"
done

# -----------------------------------------------------------------------------
# 4. COSTRUZIONE CFLAGS E COMPILATORE
# -----------------------------------------------------------------------------
EXTRA_CFLAGS=""
SELECTED_CC=""

for key in "${!MACROS[@]}"; do
    value="${MACROS[$key]}"

    if [ "$key" = "COMPILERNAME" ]; then
        SELECTED_CC="$value"
        continue
    fi

    EXTRA_CFLAGS="$EXTRA_CFLAGS -D${key}=${value}"
done

# Priorità Compilatore: Variable CC d'ambiente > COMPILERNAME da CLI/conf > gcc
CC_COMPILER="${CC:-${SELECTED_CC:-gcc}}"

# -----------------------------------------------------------------------------
# 5. ESECUZIONE MAKE
# -----------------------------------------------------------------------------
echo "[+] Compilatore selezionato: $CC_COMPILER"
echo "[+] Macro precompilatore finali: $EXTRA_CFLAGS"

# Invocazione del Makefile con le CFLAGS aggiornate[cite: 23]
make CC="$CC_COMPILER" CFLAGS="-g -Wall -Wextra -Wpedantic -std=c99 -Iinclude -Isrc/microroutines -fopenmp -MMD -MP $EXTRA_CFLAGS"

if [ $? -eq 0 ]; then
    echo "[+] Compilazione completata! Eseguibile: build/$CC_COMPILER/main"
else
    echo "[-] Errore durante la compilazione."
    exit 1
fi