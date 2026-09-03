#ifndef CONFIGURATION_H
#define CONFIGURATION_H

    #ifndef CHOSEN_SCHEDULE
        #define CHOSEN_SCHEDULE static
    #endif

    #ifndef WARMUP_REPS
        #define WARMUP_REPS         0
    #endif

    #ifndef WORK_REPS
        #define WORK_REPS      1
    #endif


    /* =========================================================================
       STRESS TEST PROBLEM SIZES (POWER OF TWO LOGARITHMIC EXPONENTIALS)
       ========================================================================= */
    /* 1. L1/L2 CACHE BOUND: N = 2^14 = 16.384 elementi (128 KB) -> M = 128 */
    #define STRESS_LOG2_N_L1_L2        14U
    /* 2. L2/L3 CACHE BOUND: N = 2^18 = 262.144 elementi (2 MB) -> M = 512 */
    #define STRESS_LOG2_N_L2_L3        18U
    /* 3. L3 LIMIT / TRANSIZIONE: N = 2^22 = 4.194.304 elementi (32 MB) -> M = 2048 */
    #define STRESS_LOG2_N_L3_TRANS     22U
    /* 4. RAM / BANDWIDTH BOUND: N = 2^26 = 67.108.864 elementi (512 MB) -> M = 8192 */
    #define STRESS_LOG2_N_RAM_BOUND    26U
    /* 5. FULL RAM STRESS: N = 2^28 = 268.435.456 elementi (2 GB) -> M = 16384 */
    #define STRESS_LOG2_N_RAM_STRESS   28U
    /* Array di inizializzazione per i ciclo sui problem size di stress */
    #define STRESS_PROBLEM_SIZES       { STRESS_LOG2_N_L1_L2,     \
                                         STRESS_LOG2_N_L2_L3,     \
                                         STRESS_LOG2_N_L3_TRANS,  \
                                         STRESS_LOG2_N_RAM_BOUND, \
                                         STRESS_LOG2_N_RAM_STRESS }
    #define STRESS_NUM_PROBLEM_SIZES   5U
    /* Dimensione del problema N PER THREAD per mantenere il carico costante */
    #define WEAK_LOG2_N_PER_THREAD     22U
    /* Dimensione fissa del problema N per misurare lo Strong Scaling al variare di P */
    #define STRONG_LOG2_N_DEFAULT      26U

#endif /* CONFIGURATION_H */