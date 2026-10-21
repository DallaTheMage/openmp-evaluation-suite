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

select_workload() {
# ---------------------------------------------------------------------------
# Problem-size selection
# ---------------------------------------------------------------------------

# Use a conservative data budget (25% of currently available RAM).
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
# (2^SIZE_PER_THREAD_LOG2) * LOGICAL_CORES must fit in the original buffer and budget.
per_thread_limit=$(( (1 << MAX_PROBLEM_SIZE) / LOGICAL_CORES ))
if (( per_thread_limit < 1 )); then
    printf '[ERROR] Memory budget cannot accommodate one element per thread.\n' >&2
    exit 1
fi

SIZE_PER_THREAD_LOG2=0
for (( exp=1; exp<=62; exp++ )); do
    if (( (1 << exp) <= per_thread_limit )); then
        SIZE_PER_THREAD_LOG2=$exp
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
}
