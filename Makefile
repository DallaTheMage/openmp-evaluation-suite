CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -Iinclude -fopenmp
LDLIBS = -fopenmp -lm

BUILD_DIR = build/$(notdir $(CC))
TARGET = $(BUILD_DIR)/main

SRC := $(shell find src -name '*.c')
OBJ := $(SRC:src/%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(@D)
	$(CC) $(OBJ) -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
