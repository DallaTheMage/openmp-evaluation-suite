/*
 * Copyright (C) 2026
 *
 * This file is part of the OpenMP compiler-agnostic benchmark suite.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file Generator.h
 * @brief Deterministic pseudo-random data generation utilities.
 *
 * This header provides the pseudo-random generator used to initialize
 * benchmark data deterministically.
 *
 * The generator state is explicitly represented by xoshiro256_state,
 * allowing callers to create independent deterministic streams.
 *
 * The generator is intended for benchmark data generation and setup.
 * It is not suitable for cryptographic or security-sensitive purposes.
 *
 * The data-generation interface is independent from the benchmark execution
 * and profiling layers.
 */

#ifndef DATA_GENERATOR_H
#define DATA_GENERATOR_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief State of the xoshiro256+ pseudo-random generator.
 * The state consists of four 64-bit words (256 bits).
 */
typedef struct {
    uint64_t s[4];
} xoshiro256_state;

/**
 * @brief Advance the generator state by 2^128 steps.
 * This is useful in parallel environments (OpenMP/MPI) to assign
 * non-overlapping subsequences to different threads or nodes.
 *
 * @param state Generator state to advance.
 */
void generator_jump(xoshiro256_state *state);

/**
 * @brief Generate one uniform double in the interval [0.0, 1.0).
 *
 * @param state Generator state.
 * @return A generated value with 53 bits of precision.
 */
static inline double generator_next_double(xoshiro256_state *state) {
    uint64_t *s = state->s;

    // Calcolo del risultato (xoshiro256+ usa la somma s[0] + s[3])
    const uint64_t result = s[0] + s[3];

    // Transizione di stato
    const uint64_t t = s[1] << 17;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= t;
    s[3] = (s[3] << 45) | (s[3] >> (64 - 45)); // rotl 45

    // Estrazione dei 53 bit di mantissa e conversione floating point
    return (result >> 11) * 0x1p-53;
}

/**
 * @brief Fill a contiguous memory region with doubles in [0.0, 1.0).
 *
 * @param seed Deterministic generator seed.
 * @param pool  Puntatore al buffer di memoria da riempire.
 * @param count Number of double elements to generate.
 */
void generator_fill_pool(uint64_t seed, double *restrict pool, size_t count);

#endif /* DATA_GENERATOR_H */
