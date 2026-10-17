#!/usr/bin/env bash

#
# Copyright (C) 2026
#
# This file is part of the OpenMP compiler-agnostic benchmark suite.
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

set -Eeuo pipefail

# ============================================================
# OpenMP Evaluation Suite - Build script
# ============================================================

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_CONF="${SCRIPT_DIR}/default.conf"
CONF_FILE="$DEFAULT_CONF"

CLI_PARAMS=()

# Verbose mode:
#   ./build.sh
#   VERBOSE=1 ./build.sh
VERBOSE="${VERBOSE:-0}"

# ------------------------------------------------------------
# Utility functions
# ------------------------------------------------------------

usage() {
    cat <<'HELP'
Usage:
  ./build.sh [file.conf] [KEY=VALUE ...] [options]

Examples:
  ./build.sh
  ./build.sh autotune.conf
  ./build.sh --config autotune.conf
  ./build.sh WORK_REPS=10
  ./build.sh autotune.conf WORK_REPS=10
  VERBOSE=1 ./build.sh
  VERBOSE=1 ./build.sh WORK_REPS=10

Options:
  -c, --config FILE   Configuration file to use
  -h, --help          Show this message

Environment variables:
  CC                  Compiler executable (overrides COMPILERNAME)
  VERBOSE=1           Show the full build command output
  OPENMP_CFLAGS       Override OpenMP compilation flags
  OPENMP_LDFLAGS      Override OpenMP linking flags
  OPENMP_LIBS         Override OpenMP libraries

Priority:
  CLI KEY=VALUE overrides configuration-file values.
  CC environment variable overrides COMPILERNAME.
HELP
}

error() {
    printf '[ERROR] %s\n' "$*" >&2
    exit 2
}

info() {
    printf '[+] %s\n' "$*"
}

warn() {
    printf '[WARNING] %s\n' "$*" >&2
}

trim() {
    local value="$1"

    value="${value#"${value%%[!$' \t\r\n']*}"}"
    value="${value%"${value##*[!$' \t\r\n']}"}"

    printf '%s' "$value"
}

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

declare -A MACROS=()

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

# ------------------------------------------------------------
# Select compiler
# ------------------------------------------------------------

CC_COMPILER="${CC:-${SELECTED_CC:-gcc}}"

# This script expects CC to contain one executable name/path.
# For wrappers such as "ccache gcc", use a wrapper executable.

if ! command -v "$CC_COMPILER" >/dev/null 2>&1; then
    error "Compiler not found: $CC_COMPILER"
fi

info "Compiler: $CC_COMPILER"

if [[ "$VERBOSE" == "1" ]]; then
    "$CC_COMPILER" --version
fi

# ------------------------------------------------------------
# Detect compiler family
# ------------------------------------------------------------

COMPILER_BASENAME="$(basename -- "$CC_COMPILER")"

case "$COMPILER_BASENAME" in
    icx|icpx)
        COMPILER_FAMILY="intel"
        ;;

    clang|clang-*)
        COMPILER_FAMILY="clang"
        ;;

    gcc|gcc-*)
        COMPILER_FAMILY="gcc"
        ;;

    *)
        COMPILER_VERSION="$("$CC_COMPILER" --version 2>&1 || true)"

        if grep -qi 'Intel.*Compiler' <<< "$COMPILER_VERSION"; then
            COMPILER_FAMILY="intel"

        elif grep -qi 'clang' <<< "$COMPILER_VERSION"; then
            COMPILER_FAMILY="clang"

        elif grep -qi 'gcc\|Free Software Foundation' <<< "$COMPILER_VERSION"; then
            COMPILER_FAMILY="gcc"

        else
            error "Unable to identify compiler family for: $CC_COMPILER
Set OPENMP_CFLAGS, OPENMP_LDFLAGS and OPENMP_LIBS manually if needed."
        fi
        ;;
esac

info "Compiler family: $COMPILER_FAMILY"

# ------------------------------------------------------------
# Configure OpenMP flags
# ------------------------------------------------------------

case "$COMPILER_FAMILY" in

    gcc)
        DEFAULT_OPENMP_CFLAGS=(-fopenmp)
        DEFAULT_OPENMP_LDFLAGS=(-fopenmp)
        DEFAULT_OPENMP_LIBS=()
        ;;

    clang)
        DEFAULT_OPENMP_CFLAGS=(-fopenmp)
        DEFAULT_OPENMP_LDFLAGS=(-fopenmp)
        DEFAULT_OPENMP_LIBS=()
        ;;

    intel)
        DEFAULT_OPENMP_CFLAGS=(-fiopenmp)
        DEFAULT_OPENMP_LDFLAGS=(-fiopenmp)
        DEFAULT_OPENMP_LIBS=()
        ;;

    *)
        error "Unsupported compiler family: $COMPILER_FAMILY"
        ;;

esac

# Environment overrides are useful for non-standard toolchains.
#
# Note: these variables are whitespace-separated flag lists.
# Paths containing spaces are not supported by this simple parser.

if [[ -n "${OPENMP_CFLAGS+x}" ]]; then
    read -r -a OPENMP_CFLAGS_ARRAY <<< "$OPENMP_CFLAGS"
else
    OPENMP_CFLAGS_ARRAY=("${DEFAULT_OPENMP_CFLAGS[@]}")
fi

if [[ -n "${OPENMP_LDFLAGS+x}" ]]; then
    read -r -a OPENMP_LDFLAGS_ARRAY <<< "$OPENMP_LDFLAGS"
else
    OPENMP_LDFLAGS_ARRAY=("${DEFAULT_OPENMP_LDFLAGS[@]}")
fi

if [[ -n "${OPENMP_LIBS+x}" ]]; then
    read -r -a OPENMP_LIBS_ARRAY <<< "$OPENMP_LIBS"
else
    OPENMP_LIBS_ARRAY=("${DEFAULT_OPENMP_LIBS[@]}")
fi

# ------------------------------------------------------------
# Check OpenMP compilation and runtime support
# ------------------------------------------------------------

TEMP_DIR="$(mktemp -d)"

cleanup_temp() {
    rm -rf "$TEMP_DIR"
}

trap cleanup_temp EXIT

OPENMP_TEST_SOURCE="${TEMP_DIR}/openmp_test.c"
OPENMP_TEST_BINARY="${TEMP_DIR}/openmp_test"

cat > "$OPENMP_TEST_SOURCE" <<'EOF'
#include <stdio.h>
#include <omp.h>

int main(void)
{
    int thread_count = 0;

    #pragma omp parallel num_threads(2) reduction(+:thread_count)
    {
        thread_count += 1;
    }

    printf("OpenMP test: %d thread(s)\n", thread_count);

    return thread_count >= 2 ? 0 : 1;
}
EOF

info "Checking OpenMP support..."

if ! "$CC_COMPILER" \
    "${OPENMP_CFLAGS_ARRAY[@]}" \
    "$OPENMP_TEST_SOURCE" \
    "${OPENMP_LDFLAGS_ARRAY[@]}" \
    -o "$OPENMP_TEST_BINARY" \
    "${OPENMP_LIBS_ARRAY[@]}" \
    >/dev/null
then
    error "The compiler cannot compile/link the OpenMP test.
Check the compiler installation and OpenMP flags.
For custom toolchains, set OPENMP_CFLAGS, OPENMP_LDFLAGS and OPENMP_LIBS."
fi

if ! OMP_NUM_THREADS=2 OMP_DYNAMIC=FALSE \
    "$OPENMP_TEST_BINARY" >/dev/null
then
    error "The OpenMP test compiled but did not run with at least two threads.
Check the runtime environment and OpenMP configuration."
fi

info "OpenMP compilation and runtime test passed."

# ------------------------------------------------------------
# Build compiler flags
# ------------------------------------------------------------

CFLAGS_ARRAY=(
    -g
    -Wall
    -Wextra
    -Wpedantic
    -std=c99
    -D_GNU_SOURCE
    -Iinclude
    -Isrc/microroutines
    -MMD
    -MP
)

CFLAGS_ARRAY+=("${OPENMP_CFLAGS_ARRAY[@]}")

# Add configuration values as preprocessor macros.
for key in "${!MACROS[@]}"; do

    value="${MACROS[$key]}"

    [[ "$value" != *$'\n'* ]] ||
        error "Multiline values are not allowed for parameter: $key"

    CFLAGS_ARRAY+=("-D${key}=${value}")

done

LDFLAGS_ARRAY=("${OPENMP_LDFLAGS_ARRAY[@]}")
LDLIBS_ARRAY=(-lm "${OPENMP_LIBS_ARRAY[@]}")

# Convert arrays to strings for GNU Make command-line variables.
# The values here are whitespace-separated compiler flags.
CFLAGS_VALUE="${CFLAGS_ARRAY[*]}"
LDFLAGS_VALUE="${LDFLAGS_ARRAY[*]}"
LDLIBS_VALUE="${LDLIBS_ARRAY[*]}"

# ------------------------------------------------------------
# Configure verbosity
# ------------------------------------------------------------

MAKE_FLAGS=()

if [[ "$VERBOSE" != "1" ]]; then
    MAKE_FLAGS=(-s)
fi

# ------------------------------------------------------------
# Clean and build
# ------------------------------------------------------------

info "Cleaning previous build..."

make "${MAKE_FLAGS[@]}" -C "$SCRIPT_DIR" clean

info "Building project..."

if ! make "${MAKE_FLAGS[@]}" \
    -C "$SCRIPT_DIR" \
    CC="$CC_COMPILER" \
    CFLAGS="$CFLAGS_VALUE" \
    LDFLAGS="$LDFLAGS_VALUE" \
    LDLIBS="$LDLIBS_VALUE"
then
    error "Build failed. Run VERBOSE=1 ./build.sh to inspect the full commands."
fi

# ------------------------------------------------------------
# Report result
# ------------------------------------------------------------

TARGET="${SCRIPT_DIR}/build/$(basename -- "$CC_COMPILER")/main"

if [[ ! -x "$TARGET" ]]; then
    error "Build finished, but executable not found: $TARGET"
fi

info "Build completed successfully."
info "Executable: $TARGET"