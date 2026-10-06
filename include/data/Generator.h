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

#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Stato del generatore xoshiro256+.
 * Dimensione fisica: 256 bit (32 byte).
 */
typedef struct {
    uint64_t s[4];
} xoshiro256_state;

/**
 * @brief Fa avanzare lo stato del generatore di 2^128 passi.
 * Utile in ambienti paralleli (OpenMP/MPI) per assegnare sottosequenze
 * non sovrapposte a thread o nodi diversi.
 *
 * @param state Pointer allo stato da fare avanzare.
 */
void generator_jump(xoshiro256_state *state);

/**
 * @brief Genera un singolo numero double uniforme nell'intervallo [0.0, 1.0).
 *
 * @param state Pointer allo stato del generatore.
 * @return double Valore generato con 53 bit di precisione.
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
 * @brief Riempie un array/pool di memoria contigua con valori double in [0.0, 1.0).
 *
 * @param seed  Seme di generazione.
 * @param pool  Puntatore al buffer di memoria da riempire.
 * @param count Numero di elementi double da generare.
 */
void generator_fill_pool(uint64_t seed, double *restrict pool, size_t count);

#endif /* GENERATOR_H */
