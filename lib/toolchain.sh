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

# Detects the compiler and verifies OpenMP compilation and runtime support.
configure_toolchain() {
# ------------------------------------------------------------
# Select compiler
# ------------------------------------------------------------
CC_COMPILER="${CC:-${SELECTED_CC:-gcc}}"

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
}
