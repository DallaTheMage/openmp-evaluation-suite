#!/usr/bin/env bash

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
