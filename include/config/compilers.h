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
