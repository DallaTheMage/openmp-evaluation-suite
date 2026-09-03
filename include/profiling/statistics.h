#ifndef STATISTICS_H
#define STATISTICS_H

#include <stddef.h>

/* =========================================================================
   1. STRUTTURE DATI STATISTICHE E DI SCALABILITÀ
   ========================================================================= */

/* Statistiche descrittive di base per un set di campioni (es. tempi, Joule) */
typedef struct {
    double mean;
    double variance;
    double stddev;
    double min;
    double max;
} Statistics;

/* Risultato per un punto di STRESS TEST (N varia, P fisso/massimo) */
typedef struct {
    unsigned int log2_n;          /* Esponente dimensione (es. 28) */
    unsigned long element_count;  /* N = 2^log2_n */
    Statistics time_stats;        /* Tempi di esecuzione (secondi) */
    Statistics energy_stats;      /* Consumo energetico (Joule) */
    double bandwidth_gbps;        /* Bandwidth calcolata sulla media dei tempi */
} StressResult;

/* Risultato per un punto di STRONG SCALING (N fisso, P varia) */
typedef struct {
    unsigned int threadnumber;         /* Numero di thread P */
    Statistics time_stats;        /* Tempi di esecuzione T_P */
    double speedup;               /* Speedup S = T_1 / T_P */
    double efficiency;            /* Efficienza E = S / P */
} StrongScalingResult;

/* Risultato per un punto di WEAK SCALING (N/P fisso, N e P variano) */
typedef struct {
    unsigned int threadnumber;         /* Numero di thread P */
    unsigned int log2_n_total;    /* Dimensione totale N per questo P */
    Statistics time_stats;        /* Tempi di esecuzione T_P */
    double efficiency;            /* Efficienza E = T_1 / T_P */
} WeakScalingResult;


/* =========================================================================
   2. DICHIARAZIONE FUNZIONI DI CALCOLO
   ========================================================================= */

/* Calcola media, varianza, dev. std, min e max da un array di campioni */
void statistics_compute(Statistics *stats, const double *results, size_t size);

/* Popola la struct StressResult a partire dai campioni di tempo ed energia */
void stress_result_compute(StressResult *res,
                           unsigned int log2_n,
                           const double *raw_times,
                           const double *raw_energy,
                           size_t size);

/* Calcola Speedup ed Efficienza Strong Scaling basandosi sul tempo medio a 1 thread */
void strong_scaling_compute(StrongScalingResult *res,
                            unsigned int threadnumber,
                            double t1_mean,
                            const double *raw_times,
                            size_t size);

/* Calcola l'Efficienza Weak Scaling basandosi sul tempo medio a 1 thread */
void weak_scaling_compute(WeakScalingResult *res,
                          unsigned int threadnumber,
                          unsigned int log2_n_total,
                          double t1_mean,
                          const double *raw_times,
                          size_t size);

#endif /* STATISTICS_H */
