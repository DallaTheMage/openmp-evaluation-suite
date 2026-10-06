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

/* Optional Struct */
typedef struct Generator {
    xoshiro256_state state;
    void (*init)(xoshiro256_state *state, uint64_t seed);
    void (*jump)(xoshiro256_state *state);
    void (*next)(xoshiro256_state *state);
} Generator;

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


Generator *generator_create(uint64_t seed);
void generator_destroy(Generator *generator);

#endif /* GENERATOR_H */