# Consente di sovrascrivere CC da riga di comando (es. make CC=clang)
CC ?= gcc

CFLAGS  := -Wall -Wextra -Wpedantic -std=c99 -Iinclude -Isrc/microroutines -fopenmp
# File di dipendenza automatica (.d) per intercettare le modifiche ai file .h
CFLAGS  += -MMD -MP

LDFLAGS := -fopenmp
LDLIBS  := -lm

# Isola gli oggetti in base al nome del compilatore (es. build/gcc o build/clang)
BUILD_DIR := build/$(notdir $(CC))
TARGET    := $(BUILD_DIR)/main

SRC := $(shell find src -name '*.c')
OBJ := $(SRC:src/%.c=$(BUILD_DIR)/%.o)
DEP := $(OBJ:.o=.d)

.PHONY: all clean

all: $(TARGET)

# Linking
$(TARGET): $(OBJ)
	@mkdir -p $(@D)
	$(CC) $(OBJ) $(LDFLAGS) -o $@ $(LDLIBS)

# Compilazione dei singoli file sorgente
$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

# Includi le dipendenze automatiche generate da GCC/Clang (-MMD -MP)
-include $(DEP)

clean:
	rm -rf build