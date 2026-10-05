#ifndef OPENMP_CONFIG_H
#define OPENMP_CONFIG_H

#include "config/params.h"

/* ========================================================================= */
/* --- OpenMP Version Detection ------------------------------------------- */
/* ========================================================================= */

#ifdef _OPENMP
    #define OPENMP_VERSION _OPENMP
#else
    #define OPENMP_VERSION 0L
#endif


/* ========================================================================= */
/* --- OpenMP Version Capabilities ---------------------------------------- */
/* ========================================================================= */

#define OPENMP_HAS_2_0  (OPENMP_VERSION >= 200203L)
#define OPENMP_HAS_2_5  (OPENMP_VERSION >= 200505L)
#define OPENMP_HAS_3_0  (OPENMP_VERSION >= 200805L)
#define OPENMP_HAS_3_1  (OPENMP_VERSION >= 201107L)
#define OPENMP_HAS_4_0  (OPENMP_VERSION >= 201307L)
#define OPENMP_HAS_4_5  (OPENMP_VERSION >= 201511L)
#define OPENMP_HAS_5_0  (OPENMP_VERSION >= 201811L)
#define OPENMP_HAS_5_1  (OPENMP_VERSION >= 202011L)
#define OPENMP_HAS_5_2  (OPENMP_VERSION >= 202111L)
#define OPENMP_HAS_6_0  (OPENMP_VERSION >= 202411L)


/* ========================================================================= */
/* --- OpenMP Feature Capabilities ---------------------------------------- */
/* ========================================================================= */

#define OPENMP_HAS_TASKLOOP OPENMP_HAS_4_5
#define OPENMP_HAS_SIMD     OPENMP_HAS_4_0
#define OPENMP_HAS_COLLAPSE OPENMP_HAS_3_0
#define OPENMP_HAS_ORDERED  OPENMP_HAS_3_0
#define OPENMP_HAS_SECTIONS OPENMP_HAS_2_0
#define OPENMP_HAS_ATOMIC   OPENMP_HAS_2_0
#define OPENMP_HAS_CRITICAL OPENMP_HAS_2_0
#define OPENMP_HAS_BARRIER  OPENMP_HAS_2_0
#define OPENMP_HAS_SINGLE   OPENMP_HAS_2_0
#define OPENMP_HAS_NOWAIT   OPENMP_HAS_2_0
#define OPENMP_HAS_MASKED   OPENMP_HAS_5_1
#define OPENMP_HAS_LOOP     OPENMP_HAS_5_0


/* ========================================================================= */
/* --- Scan Capabilities -------------------------------------------------- */
/* ========================================================================= */

/*
 * Native OpenMP scan.
 *
 * OpenMP >= 5.0 is required.
 *
 * Attualmente disabilitato su Clang/ICX per evitare di affidarsi
 * a implementazioni/runtime LLVM note per problematiche nella
 * configurazione utilizzata dalla suite.
 */
#if defined(__clang__) || defined(__INTEL_CLANG_COMPILER)

    #define OPENMP_HAS_NATIVE_SCAN 0

#else

    #if OPENMP_HAS_5_0 && (CHOSEN_SCHEDULE_ID == SCHED_STATIC)
        #define OPENMP_HAS_NATIVE_SCAN 1
    #else
        #define OPENMP_HAS_NATIVE_SCAN 0
    #endif

#endif


/*
 * Custom two-pass scan.
 *
 * Richiede solamente supporto OpenMP di base.
 */
#define OPENMP_HAS_TWO_PASS_SCAN OPENMP_HAS_2_0


/* ========================================================================= */
/* --- OpenMP Version String ---------------------------------------------- */
/* ========================================================================= */

#if OPENMP_VERSION == 0L

    #define OPENMP_VERSION_STRING "disabled"

#elif OPENMP_VERSION >= 202411L

    #define OPENMP_VERSION_STRING "6.0+"

#elif OPENMP_VERSION >= 202111L

    #define OPENMP_VERSION_STRING "5.2"

#elif OPENMP_VERSION >= 202011L

    #define OPENMP_VERSION_STRING "5.1"

#elif OPENMP_VERSION >= 201811L

    #define OPENMP_VERSION_STRING "5.0"

#elif OPENMP_VERSION >= 201511L

    #define OPENMP_VERSION_STRING "4.5"

#elif OPENMP_VERSION >= 201307L

    #define OPENMP_VERSION_STRING "4.0"

#elif OPENMP_VERSION >= 201107L

    #define OPENMP_VERSION_STRING "3.1"

#elif OPENMP_VERSION >= 200805L

    #define OPENMP_VERSION_STRING "3.0"

#elif OPENMP_VERSION >= 200505L

    #define OPENMP_VERSION_STRING "2.5"

#else

    #define OPENMP_VERSION_STRING "2.0"

#endif


#endif /* OPENMP_CONFIG_H */
