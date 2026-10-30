#!/bin/bash

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

set -euo pipefail

TESTBATTERY_FILE="${1:-default.battery.conf}"
if [[ ! -f "$TESTBATTERY_FILE" ]]; then
    echo "Error: Test battery configuration file '$TESTBATTERY_FILE' not found." >&2
    exit 1
fi

source "$(realpath "$TESTBATTERY_FILE")"
REQUIRED_VARIABLES=(
    CONFIGURATION_FILE
    COMPILERS
    SCHEDULING_POLICIES
    OPTIMIZATION_LEVELS
)

for variable in "${REQUIRED_VARIABLES[@]}"; do
    if [[ -z "${!variable:-}" ]]; then
        echo "Error: Missing or empty configuration parameter: $variable" >&2
        exit 1
    fi
done

if [[ ! -f "$CONFIGURATION_FILE" ]]; then
    echo "Error: Application configuration file '$CONFIGURATION_FILE' not found." >&2
    exit 1
fi

if [[ ! -x ./build.sh ]]; then
    echo "Error: Build script './build.sh' not found or not executable." >&2
    exit 1
fi

validate_list_format() {
    local name="$1"
    local value="$2"

    if [[ "$value" == ,* ||
          "$value" == *, ||
          "$value" == *,,* ]]; then
        echo "Error: Invalid comma-separated list: $name" >&2
        exit 1
    fi
}

check_duplicates() {
    local list_name="$1"
    shift

    local -A seen=()
    local value

    for value in "$@"; do
        if [[ -v seen["$value"] ]]; then
            echo "Error: Duplicate value '$value' in $list_name." >&2
            exit 1
        fi

        seen["$value"]=1
    done
}

validate_list_format "COMPILERS" "$COMPILERS"
validate_list_format "SCHEDULING_POLICIES" "$SCHEDULING_POLICIES"
validate_list_format "OPTIMIZATION_LEVELS" "$OPTIMIZATION_LEVELS"

IFS=',' read -r -a COMPILER_LIST <<< "$COMPILERS"
IFS=',' read -r -a SCHEDULING_LIST <<< "$SCHEDULING_POLICIES"
IFS=',' read -r -a OPTIMIZATION_LIST <<< "$OPTIMIZATION_LEVELS"

if (( ${#COMPILER_LIST[@]} == 0 ||
      ${#SCHEDULING_LIST[@]} == 0 ||
      ${#OPTIMIZATION_LIST[@]} == 0 )); then
    echo "Error: Configuration lists must not be empty." >&2
    exit 1
fi

check_duplicates "COMPILERS" "${COMPILER_LIST[@]}"
check_duplicates "SCHEDULING_POLICIES" "${SCHEDULING_LIST[@]}"
check_duplicates "OPTIMIZATION_LEVELS" "${OPTIMIZATION_LIST[@]}"

for compiler in "${COMPILER_LIST[@]}"; do
    case "$compiler" in
        gcc|clang|icx) ;;
        *)
            echo "Error: Unsupported compiler: '$compiler'" >&2
            exit 1
            ;;
    esac
done

# Initialize Intel oneAPI only when icx is part of the configured compiler list.
for compiler in "${COMPILER_LIST[@]}"; do
    if [[ "$compiler" == "icx" ]]; then
        # Intel oneAPI setup scripts may reference unset environment variables.
        set +u
        source /opt/intel/oneapi/setvars.sh
        set -u
        break
    fi
done

for schedule in "${SCHEDULING_LIST[@]}"; do
    case "$schedule" in
        1U|2U|3U) ;;
        *)
            echo "Error: Unsupported scheduling policy: '$schedule'" >&2
            exit 1
            ;;
    esac
done

for optimization in "${OPTIMIZATION_LIST[@]}"; do
    case "$optimization" in
        O0|O1|O2|O3) ;;
        *)
            echo "Error: Unsupported optimization level: '$optimization'" >&2
            exit 1
            ;;
    esac
done

# Generate a unique identifier for the current test battery
RUN_ID="$(date +%Y%m%d_%H%M%S)_$$"

# Define the results directory
RESULTS_DIR="results/$RUN_ID"

# Track the currently executing operation
CURRENT_TEST="Results directory setup"

trap 'status=$?; echo "Error: Test battery failed during: $CURRENT_TEST (line $LINENO, exit code $status)." >&2' ERR
trap 'echo "Test battery interrupted during: $CURRENT_TEST" >&2; exit 130' INT
trap 'echo "Test battery terminated during: $CURRENT_TEST" >&2; exit 143' TERM

if [[ -e output || -L output ]]; then
    echo "Cleaning previous temporary output..."
    sudo make clean-output

    if [[ -e output || -L output ]]; then
        echo "Error: Output path still exists after cleanup." >&2
        exit 1
    fi
fi

sudo mkdir -p results
if [[ -e "$RESULTS_DIR" || -L "$RESULTS_DIR" ]]; then
    echo "Error: Results directory '$RESULTS_DIR' already exists." >&2
    exit 1
fi
sudo mkdir "$RESULTS_DIR"
# Assign ownership to the current user
sudo chown "$(id -u):$(id -g)" "$RESULTS_DIR"
cp "$TESTBATTERY_FILE" "$RESULTS_DIR/testbattery.battery.conf"
cp "$CONFIGURATION_FILE" "$RESULTS_DIR/application.conf"
CURRENT_TEST="Saving compiler information"
COMPILER_INFO="$RESULTS_DIR/compiler_versions.txt"

{
    for compiler in "${COMPILER_LIST[@]}"; do
        echo "========================================"
        echo "Compiler: $compiler"
        echo "========================================"
        "$compiler" --version
        echo ""
    done
} > "$COMPILER_INFO"

COUNT=1
TOTAL_TESTS=$((
    ${#COMPILER_LIST[@]} *
    ${#SCHEDULING_LIST[@]} *
    ${#OPTIMIZATION_LIST[@]}
))

echo ""
echo "========================================"
echo "           TEST BATTERY SETUP           "
echo "========================================"
echo "Configuration:       $CONFIGURATION_FILE"
echo "Compilers:           $COMPILERS  		  "
echo "Scheduling policies: $SCHEDULING_POLICIES"
echo "Optimization levels: $OPTIMIZATION_LEVELS"
echo "Total tests:         $TOTAL_TESTS       "
echo "Results directory:   $RESULTS_DIR       "
echo "========================================"
echo ""

for optimization in "${OPTIMIZATION_LIST[@]}"; do
    for compiler in "${COMPILER_LIST[@]}"; do
        for schedule in "${SCHEDULING_LIST[@]}"; do
            CURRENT_TEST="Test $COUNT/$TOTAL_TESTS - $compiler $schedule -$optimization"

            echo ""
            echo "=========================================="
            echo " Test $COUNT / $TOTAL_TESTS				"
            echo "=========================================="
            echo " Compiler:     $compiler     				"
            echo " Schedule:     $schedule     				"
            echo " Optimization: -$optimization				"
            echo "=========================================="

            ./build.sh "$CONFIGURATION_FILE" \
                OPT_LEVEL="-$optimization" \
                COMPILERNAME="$compiler" \
                CHOSEN_SCHEDULE_ID="$schedule"

            EXECUTABLE="./build/$compiler/main"
            if [[ ! -x "$EXECUTABLE" ]]; then
                echo "Error: Executable '$EXECUTABLE' not found or not executable." >&2
                exit 1
            fi

            sudo "$EXECUTABLE"
            if [[ ! -d output ]]; then
                echo "Error: Output directory not generated by $CURRENT_TEST." >&2
                exit 1
            fi
            ((COUNT += 1))

        done
    done

    CURRENT_TEST="Saving results for -$optimization"
    DESTINATION="$RESULTS_DIR/output_$optimization"
    echo ""
    echo "Moving results to: $DESTINATION"
    if [[ -e "$DESTINATION" || -L "$DESTINATION" ]]; then
        echo "Error: Destination '$DESTINATION' already exists." >&2
        exit 1
    fi
    sudo mv output "$DESTINATION"
    sudo chown -R "$(id -u):$(id -g)" "$DESTINATION"
    echo "Results successfully saved for -$optimization"
done

echo ""
echo "==================================================="
echo "       TEST BATTERY COMPLETED"
echo "==================================================="
echo "Total tests executed: $TOTAL_TESTS"
echo "Results saved in:     $RESULTS_DIR"
echo "==================================================="