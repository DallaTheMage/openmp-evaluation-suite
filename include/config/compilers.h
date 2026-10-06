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
 * @file compilers.h
 * @brief Compile-time compiler identification and metadata.
 *
 * This header detects the compiler used to build the benchmark suite and
 * exposes normalized compiler metadata through preprocessor macros.
 *
 * The detection order accounts for compilers that define compatibility
 * macros belonging to another compiler family. More specific compiler
 * identification macros must therefore be checked before generic
 * compatibility macros.
 *
 * The resulting metadata is used by the benchmark configuration and output
 * layers to identify the compiler associated with a benchmark build.
 *
 * No runtime compiler detection is performed by this header.
 */
#ifndef CONFIG_COMPILERS_H
#define CONFIG_COMPILERS_H


/* ========================================================================= */
/* --- Preprocessor stringification --------------------------------------- */
/* ========================================================================= */

#ifndef STRINGIFY2
    #define STRINGIFY2(x) #x
#endif

#ifndef STRINGIFY
    #define STRINGIFY(x) STRINGIFY2(x)
#endif


/* ========================================================================= */
/* --- Compiler identification -------------------------------------------- */
/* ========================================================================= */

/*
 * Detection order is intentional.
 *
 * Intel LLVM (icx) may expose Clang/GCC-compatible predefined macros,
 * therefore Intel compilers must be detected before Clang/GCC.
 */

#if defined(__INTEL_LLVM_COMPILER)

    #define OES_COMPILER_FAMILY  "Intel"
    #define OES_COMPILER_NAME    "icx"
    #define OES_COMPILER_VERSION STRINGIFY(__INTEL_LLVM_COMPILER)


#elif defined(__INTEL_COMPILER)

    #define OES_COMPILER_FAMILY  "Intel"
    #define OES_COMPILER_NAME    "icc"
    #define OES_COMPILER_VERSION STRINGIFY(__INTEL_COMPILER)


#elif defined(__clang__)

    #define OES_COMPILER_FAMILY  "LLVM"
    #define OES_COMPILER_NAME    "clang"

    #define OES_COMPILER_VERSION \
        STRINGIFY(__clang_major__) "." \
        STRINGIFY(__clang_minor__) "." \
        STRINGIFY(__clang_patchlevel__)


#elif defined(__GNUC__)

    #define OES_COMPILER_FAMILY  "GNU"
    #define OES_COMPILER_NAME    "gcc"

    #define OES_COMPILER_VERSION \
        STRINGIFY(__GNUC__) "." \
        STRINGIFY(__GNUC_MINOR__) "." \
        STRINGIFY(__GNUC_PATCHLEVEL__)


#else

    #define OES_COMPILER_FAMILY  "Unknown"
    #define OES_COMPILER_NAME    "unknown"
    #define OES_COMPILER_VERSION "unknown"

#endif


/* ========================================================================= */
/* --- Compiler flags ------------------------------------------------------ */
/* ========================================================================= */

/*
 * Compiler flags cannot be reliably reconstructed from inside the
 * executable.
 *
 * The build system can provide them explicitly:
 *
 *     -DOES_COMPILER_FLAGS=\"-O3 -march=native -fopenmp\"
 *
 * If they are not provided, Configuration records "unknown".
 */

#ifndef OES_COMPILER_FLAGS
    #define OES_COMPILER_FLAGS "unknown"
#endif


#endif /* CONFIG_COMPILERS_H */
