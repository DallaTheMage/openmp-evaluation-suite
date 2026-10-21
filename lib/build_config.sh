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

# Loads configuration and validates cache settings.
load_build_configuration() {
# ------------------------------------------------------------
# Parse command-line arguments
# ------------------------------------------------------------

while (($#)); do
    case "$1" in
        -h|--help)
            usage
            exit 0
            ;;

        -c|--config)
            [[ $# -ge 2 ]] || error "Missing configuration file after $1"

            CONF_FILE="$2"
            shift 2
            ;;

        *.conf)
            CONF_FILE="$1"
            shift
            ;;

        *=*)
            CLI_PARAMS+=("$1")
            shift
            ;;

        *)
            printf '[ERROR] Unrecognized argument: %s\n\n' "$1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

# ------------------------------------------------------------
# Locate configuration file
# ------------------------------------------------------------

if [[ ! -f "$CONF_FILE" && -f "${SCRIPT_DIR}/${CONF_FILE}" ]]; then
    CONF_FILE="${SCRIPT_DIR}/${CONF_FILE}"
fi

[[ -f "$CONF_FILE" ]] ||
    error "Configuration file not found: $CONF_FILE"

# ------------------------------------------------------------
# Load configuration
# ------------------------------------------------------------

MACROS=()

SELECTED_CC=""

info "Loading configuration: $CONF_FILE"

while IFS= read -r line || [[ -n "$line" ]]; do

    # Remove Windows carriage returns.
    line="${line%%$'\r'}"

    line="$(trim "$line")"

    # Skip blank lines and comments.
    [[ -z "$line" || "$line" == \#* ]] && continue

    [[ "$line" == *=* ]] ||
        error "Invalid configuration line in $CONF_FILE: $line"

    key="$(trim "${line%%=*}")"
    value="$(trim "${line#*=}")"

    [[ "$key" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]] ||
        error "Invalid parameter name: $key"

    [[ -n "$value" ]] ||
        error "Empty value for parameter: $key"

    if [[ "$key" == "COMPILERNAME" ]]; then
        SELECTED_CC="$value"
    else
        MACROS["$key"]="$value"
    fi

done < "$CONF_FILE"

# ------------------------------------------------------------
# Apply command-line overrides
# ------------------------------------------------------------

for param in "${CLI_PARAMS[@]}"; do

    key="$(trim "${param%%=*}")"
    value="$(trim "${param#*=}")"

    [[ "$key" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]] ||
        error "Invalid parameter name in command line: $key"

    [[ -n "$value" ]] ||
        error "Empty command-line value for parameter: $key"

    if [[ "$key" == "COMPILERNAME" ]]; then
        SELECTED_CC="$value"
    else
        MACROS["$key"]="$value"
    fi

    info "Command-line override: $key=$value"

done

# SIZE_PER_THREAD is derived in params.h; never define it directly.
if [[ -v 'MACROS[SIZE_PER_THREAD]' ]]; then
    error "SIZE_PER_THREAD is derived; configure SIZE_PER_THREAD_LOG2 instead"
fi

# Verify unsigned C integer exponents and workload capacity.
validate_log2() {
    local name="$1" value="$2" digits
    [[ "$value" =~ ^([0-9]+)(U|UL|ULL)?$ ]] ||
        error "$name must be an unsigned decimal integer (optional C suffix)"
    digits="${BASH_REMATCH[1]}"
    digits="${digits#"${digits%%[!0]*}"}"
    [[ -n "$digits" ]] || digits=0
    (( ${#digits} <= 2 )) || error "$name must be between 0 and 62"
    (( 10#$digits <= 62 )) || error "$name must be between 0 and 62"
    LOG2_VALUE=$((10#$digits))
}

if [[ -v 'MACROS[SIZE_PER_THREAD_LOG2]' ]]; then
    validate_log2 SIZE_PER_THREAD_LOG2 "${MACROS[SIZE_PER_THREAD_LOG2]}"
    thread_exp=$LOG2_VALUE
else
    # Current params.h default, used only if the configuration omits the value.
    thread_exp=19
fi

if [[ -v 'MACROS[PROBLEM_LOG2_SIZE]' ]]; then
    validate_log2 PROBLEM_LOG2_SIZE "${MACROS[PROBLEM_LOG2_SIZE]}"
    problem_exp=$LOG2_VALUE
    if [[ -v 'MACROS[THREAD_LIST]' && "${MACROS[RUN_PROPORTIONAL]:-1}" != 0 ]]; then
        (( thread_exp <= problem_exp )) || error "Per-thread workload exceeds problem size"
        list="${MACROS[THREAD_LIST]}"
        [[ "$list" != ,* && "$list" != *, && "$list" != *,,* ]] || error "Malformed THREAD_LIST"
        IFS=',' read -r -a counts <<< "$list"
        max_threads=0
        for entry in "${counts[@]}"; do
            [[ "$entry" =~ ^[0-9]+$ ]] || error "Invalid thread count: $entry"
            digits="${entry#"${entry%%[!0]*}"}"
            [[ -n "$digits" ]] || digits=0
            (( ${#digits} <= 15 )) || error "Thread count is too large: $entry"
            count=$((10#$digits))
            (( count > 0 )) || error "Thread count must be positive"
            if (( count > max_threads )); then max_threads=$count; fi
        done
        capacity_exp=$((problem_exp - thread_exp))
        if (( capacity_exp < 63 && max_threads > (1 << capacity_exp) )); then
            error "Proportional workload exceeds problem buffer: 2^$thread_exp x $max_threads > 2^$problem_exp"
        fi
    fi
fi

# ------------------------------------------------------------
# Backward compatibility:
# CHOSEN_SCHEDULE=static/dynamic/guided
# ------------------------------------------------------------

if [[ -n "${MACROS[CHOSEN_SCHEDULE]:-}" ]]; then

    case "${MACROS[CHOSEN_SCHEDULE]}" in
        static)
            MACROS[CHOSEN_SCHEDULE_ID]=1U
            ;;

        dynamic)
            MACROS[CHOSEN_SCHEDULE_ID]=2U
            ;;

        guided)
            MACROS[CHOSEN_SCHEDULE_ID]=3U
            ;;

        *)
            error "CHOSEN_SCHEDULE must be static, dynamic or guided"
            ;;
    esac

    unset 'MACROS[CHOSEN_SCHEDULE]'

fi

# ------------------------------------------------------------
# Cache eviction configuration
# ------------------------------------------------------------

# Default values preserve the original benchmark behavior.
MACROS[CACHE_EVICTION_ENABLED]="${MACROS[CACHE_EVICTION_ENABLED]:-0U}"
MACROS[CACHE_EVICTION_MULTIPLIER]="${MACROS[CACHE_EVICTION_MULTIPLIER]:-2U}"
MACROS[CACHE_EVICTION_STRIDE_BYTES]="${MACROS[CACHE_EVICTION_STRIDE_BYTES]:-64U}"

# Validate an unsigned integer with an optional C integer suffix.
# Accepted examples: 0, 1U, 64U, 128UL, 256ULL.
parse_cache_uint() {
    local value="$1"
    local digits

    if [[ ! "$value" =~ ^([0-9]+)(U|UL|ULL)?$ ]]; then
        error "Invalid cache eviction integer: $value"
    fi

    digits="${BASH_REMATCH[1]}"

    # Avoid leading-zero octal interpretation in Bash.
    CACHE_PARSED_UINT="$((10#$digits))"
}

parse_cache_uint "${MACROS[CACHE_EVICTION_ENABLED]}"
CACHE_EVICTION_ENABLED_VALUE="$CACHE_PARSED_UINT"

parse_cache_uint "${MACROS[CACHE_EVICTION_MULTIPLIER]}"
CACHE_EVICTION_MULTIPLIER_VALUE="$CACHE_PARSED_UINT"

parse_cache_uint "${MACROS[CACHE_EVICTION_STRIDE_BYTES]}"
CACHE_EVICTION_STRIDE_VALUE="$CACHE_PARSED_UINT"

if (( CACHE_EVICTION_ENABLED_VALUE != 0 &&
      CACHE_EVICTION_ENABLED_VALUE != 1 )); then

    error "CACHE_EVICTION_ENABLED must be 0 or 1"
fi

if (( CACHE_EVICTION_MULTIPLIER_VALUE < 1 )); then
    error "CACHE_EVICTION_MULTIPLIER must be greater than zero"
fi

if (( CACHE_EVICTION_STRIDE_VALUE < 1 )); then
    error "CACHE_EVICTION_STRIDE_BYTES must be greater than zero"
fi

if (( CACHE_EVICTION_ENABLED_VALUE == 1 )); then

    if [[ -z "${MACROS[DETECTED_LLC_SIZE_BYTES]:-}" ]]; then
        error "Cache eviction requires DETECTED_LLC_SIZE_BYTES"
    fi

    parse_cache_uint "${MACROS[DETECTED_LLC_SIZE_BYTES]}"

    if (( CACHE_PARSED_UINT == 0 )); then
        error "Cache eviction requires a nonzero LLC size"
    fi
fi

info "Cache eviction enabled: ${CACHE_EVICTION_ENABLED_VALUE}"
info "Cache eviction multiplier: ${CACHE_EVICTION_MULTIPLIER_VALUE}"
info "Cache eviction stride: ${CACHE_EVICTION_STRIDE_VALUE} bytes"
}
