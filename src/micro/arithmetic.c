#include "micro/arithmetic.h"
#include "config/iterations.h"

double arithmetic_step(double value) {
    // Aumentando il lavoro computazionale interno
    for (int i = 0; i < SLOWDOWN_REPS; ++i) {
        value = value * (double)1.000001 + (double)0.000001;
    }
    return value;
}
