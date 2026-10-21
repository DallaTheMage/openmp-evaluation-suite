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

# =============================================================================
# OpenMP Evaluation Suite - Makefile
# =============================================================================
# build.sh supplies toolchain-specific flags and configuration macros.
# The defaults below also permit direct invocation of GNU Make.

.DEFAULT_GOAL := all

# GNU Make already defines CC=cc unless explicitly overridden.
CC ?= cc
CFLAGS ?= -O2 -g -Wall -Wextra -Wpedantic -std=c99 \
          -D_GNU_SOURCE -Iinclude -Isrc/microroutines \
          -fopenmp -MMD -MP
LDFLAGS ?= -fopenmp
LDLIBS ?= -lm

# Keep build products for different compiler names separate.
BUILD_DIR := build/$(notdir $(CC))
TARGET    := $(BUILD_DIR)/main

# Preserve the source layout below the compiler-specific build directory.
SOURCES := $(shell find src -type f -name '*.c')
OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
DEPS    := $(OBJECTS:.o=.d)

.PHONY: all clean erase

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p "$(@D)"
	$(CC) $(OBJECTS) $(LDFLAGS) -o "$@" $(LDLIBS)

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p "$(@D)"
	$(CC) $(CFLAGS) -c "$<" -o "$@"

# Header changes trigger incremental recompilation.
-include $(DEPS)

clean:
	rm -rf build

erase:
	rm -rf output
