#ifndef MICROROUTINES_ARITHMETIC_H
#define MICROROUTINES_ARITHMETIC_H
#include "config/iterations.h"

/**
 * Esegue il passo aritmetico per rallentare l'esecuzione e dare tempo
 * alla memoria RAM.
 *
 * Utilizza 'static inline' (C99) per azzerare l'overhead della chiamata a funzione
 * garantendo la correttezza della sintassi e il ritorno del valore.
 */
static inline double arithmetic_step(double value)
{
    for (int i = 0; i < SLOWDOWN_REPS; ++i) {
        value = value * (double)1.000001 + (double)0.000001;
    }
    return value;
}

#endif /* MICROROUTINES_ARITHMETIC_H */
