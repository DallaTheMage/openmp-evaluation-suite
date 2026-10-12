# ============================================================
# OpenMP Evaluation Suite - Makefile
# ============================================================

# Compiler can be overridden from the command line.
# Example: make CC=clang
CC ?= gcc

# Default flags.
# build.sh normally overrides these values.
CFLAGS ?= -g -Wall -Wextra -Wpedantic -std=c99 \
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