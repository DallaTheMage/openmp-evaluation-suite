#ifndef CONFIG_RNG_H
#define CONFIG_RNG_H


/*
 * ============================================================
 * Available RNG implementations
 * ============================================================
 */

#define RNG_XOSHIRO256  1
#define RNG_SPLITMIX64  2


/*
 * Default RNG.
 */
#ifndef RNG_TYPE
#define RNG_TYPE RNG_SPLITMIX64
#endif


/*
 * Validate RNG selection.
 */
#if RNG_TYPE != RNG_XOSHIRO256 && \
    RNG_TYPE != RNG_SPLITMIX64

    #error "Unsupported RNG_TYPE"

#endif


#endif /* CONFIG_RNG_H */
