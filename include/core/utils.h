#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

/* Conteggio elementi totale N = 2^log2_n */
#define GET_ELEMENT_COUNT(log2_n)   ((size_t)1 << (size_t)(log2_n))

/* Dimensione lato matrice M x M dove M = 2^(log2_n / 2) */
#define GET_MATRIX_DIM(log2_n)      ((size_t)1 << ((size_t)(log2_n) >> 1))

/* Occupazione dinamica della memoria in Byte */
#define GET_SIZE_IN_BYTES(log2_n)   (GET_ELEMENT_COUNT(log2_n) * sizeof(double))

#endif /* UTILS_H */
