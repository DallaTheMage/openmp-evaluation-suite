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

# ============================================================
# OpenMP Evaluation Suite - Makefile
# ============================================================

# Compiler can be overridden from the command line.
# Example: make CC=clang
CC ?= gcc

# Default flags.
# build.sh normally overrides these values.
CFLAGS ?= -O2 -g -Wall -Wextra -Wpedantic -std=c99 \
          -D_GNU_SOURCE \
          -Iinclude \
          -Isrc/microroutines \
          -fopenmp \
          -MMD -MP

LDFLAGS ?= -fopenmp
LDLIBS  ?= -lm

# ------------------------------------------------------------
# Build directories
# ------------------------------------------------------------

# Separate output directories by compiler executable name.
# Examples:
#   build/gcc/
#   build/clang/
#   build/icx/

BUILD_DIR := build/$(notdir $(CC))
TARGET    := $(BUILD_DIR)/main

# ------------------------------------------------------------
# Source files
# ------------------------------------------------------------

SRC := $(shell find src -name '*.c')
OBJ := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRC))
DEP := $(OBJ:.o=.d)

# ------------------------------------------------------------
# Targets
# ------------------------------------------------------------

.PHONY: all clean erase

all: $(TARGET)

# ------------------------------------------------------------
# Link executable
# ------------------------------------------------------------

$(TARGET): $(OBJ)
	@mkdir -p $(@D)
	$(CC) $(OBJ) $(LDFLAGS) -o $@ $(LDLIBS)

# ------------------------------------------------------------
# Compile source files
# ------------------------------------------------------------

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

# ------------------------------------------------------------
# Automatic header dependencies
# ------------------------------------------------------------

-include $(DEP)

# ------------------------------------------------------------
# Clean build artifacts
# ------------------------------------------------------------

clean:
	rm -rf build

# ------------------------------------------------------------
# Remove generated results
# ------------------------------------------------------------

erase:
	rm -rf output