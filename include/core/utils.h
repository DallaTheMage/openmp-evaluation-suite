#ifndef UTILS_H
#define UTILS_H
/* =========================================================================
   MACRO HELPER BITWISE PER ANSI C (C89)
   ========================================================================= */

/* Conteggio elementi totale N = 2^log2_n */
#define GET_ELEMENT_COUNT(log2_n)   ((unsigned long)1UL << (log2_n))

/* Dimensione lato matrice M x M dove M = sqrt(N) = 2^(log2_n / 2) */
#define GET_MATRIX_DIM(log2_n)      ((unsigned long)1UL << ((log2_n) >> 1))

/* Occupazione dinamica della memoria in Byte basata sul tipo dati */
#define GET_SIZE_IN_BYTES(log2_n)   (GET_ELEMENT_COUNT(log2_n) * sizeof(datatype))

#endif /* UTILS_H */
