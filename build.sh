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

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_CONF="${SCRIPT_DIR}/default.conf"
CONF_FILE="$DEFAULT_CONF"
CLI_PARAMS=()
VERBOSE="${VERBOSE:-0}"
declare -A MACROS=()

source "${SCRIPT_DIR}/lib/common.sh"
source "${SCRIPT_DIR}/lib/build_config.sh"
source "${SCRIPT_DIR}/lib/toolchain.sh"

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
  OPT_LEVEL           Optimization flag (default: -O2; e.g. -O0, -O3)
  OPENMP_CFLAGS       Override OpenMP compilation flags
  OPENMP_LDFLAGS      Override OpenMP linking flags
  OPENMP_LIBS         Override OpenMP libraries

Priority:
  CLI KEY=VALUE overrides configuration-file values.
  CC environment variable overrides COMPILERNAME.
HELP
}
load_build_configuration "$@"
configure_toolchain

if [[ -v 'MACROS[SIZE_PER_THREAD_LOG2]' ]]; then
    info "Per-thread workload: 2^${MACROS[SIZE_PER_THREAD_LOG2]} elements"
fi

# ------------------------------------------------------------
# Build compiler flags
# ------------------------------------------------------------

OPT_LEVEL="${OPT_LEVEL:--O2}"
[[ "$OPT_LEVEL" =~ ^-O(0|1|2|3|s|g|fast)$ ]] || error "Unsupported OPT_LEVEL: $OPT_LEVEL"

CFLAGS_ARRAY=(
    "$OPT_LEVEL"
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

# Record the effective optimization/compilation options in the binary.
# Exclude preprocessor configuration macros (which represent the experiment
# configuration and may contain arbitrary data, not toolchain options).
# Encode as a valid C string literal and a single shell argument.
BUILD_FLAGS_TEXT="${CFLAGS_ARRAY[*]}"
BUILD_FLAGS_ESCAPED="${BUILD_FLAGS_TEXT//\\/\\\\}"
BUILD_FLAGS_ESCAPED="${BUILD_FLAGS_ESCAPED//\"/\\\"}"
CFLAGS_ARRAY+=("-DOES_COMPILER_FLAGS='\"${BUILD_FLAGS_ESCAPED}\"'")

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