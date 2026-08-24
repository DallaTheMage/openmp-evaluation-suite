CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -Iinclude -fopenmp
LDLIBS = -fopenmp -lm

TARGET = main

SRC = $(wildcard src/*.c) \
      $(wildcard src/core/*.c) \
      $(wildcard src/data/*.c) \
      $(wildcard src/data/writers/*.c) \
      $(wildcard src/data/generators/*.c) \
      $(wildcard src/data/rngs/*.c) \
      $(wildcard src/metrics/*.c) \
      $(wildcard src/profiling/*.c) \
      $(wildcard src/test/*.c)

OBJ = $(SRC:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)