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
 * @file openmp.h
 * @brief Compile-time OpenMP version and feature detection.
 *
 * This header centralizes compile-time detection of the OpenMP version
 * supported by the compiler and exposes feature capability macros used
 * throughout the benchmark suite.
 *
 * The detected capabilities describe language and directive support
 * available to the current compilation unit. They are intentionally kept
 * separate from runtime configuration.
 *
 * OpenMP scheduling is selected at compile time through the configuration
 * provided by config/params.h. This header exposes the resulting schedule
 * capabilities but does not modify the OpenMP runtime state.
 *
 * The feature macros defined here are intended to keep benchmark code
 * compiler-agnostic while allowing individual kernels to be conditionally
 * compiled when a required OpenMP feature is unavailable.
 *
 * @note Some OpenMP features may require additional compiler-specific
 *       checks even when the corresponding OpenMP version is available.
 */

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
