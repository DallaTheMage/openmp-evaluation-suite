#ifndef CONFIG_TYPES_H
#define CONFIG_TYPES_H

#include <stdint.h>


/*
 * ============================================================
 * Fixed-width integer aliases
 * ============================================================
 */

typedef int8_t   int8;
typedef uint8_t  uint8;

typedef int16_t  int16;
typedef uint16_t uint16;

typedef int32_t  int32;
typedef uint32_t uint32;

typedef int64_t  int64;
typedef uint64_t uint64;


/*
 * ============================================================
 * DATATYPE identifiers
 * ============================================================
 */

#define DATATYPE_INT8       1
#define DATATYPE_UINT8      2
#define DATATYPE_INT16      3
#define DATATYPE_UINT16     4
#define DATATYPE_INT32      5
#define DATATYPE_UINT32     6
#define DATATYPE_INT64      7
#define DATATYPE_UINT64     8
#define DATATYPE_FLOAT      9
#define DATATYPE_DOUBLE     10


/*
 * Default datatype.
 */
#ifndef DATATYPE
#define DATATYPE DATATYPE_FLOAT
#endif


/*
 * ============================================================
 * Datatype selection
 * ============================================================
 */

#if DATATYPE == DATATYPE_INT8

    typedef int8_t datatype;

    #define DATATYPE_IS_INTEGER   1
    #define DATATYPE_IS_FLOATING  0
    #define DATATYPE_IS_SIGNED    1
    #define DATATYPE_IS_UNSIGNED  0
    #define DATATYPE_IS_FLOAT     0
    #define DATATYPE_IS_DOUBLE    0

#elif DATATYPE == DATATYPE_UINT8

    typedef uint8_t datatype;

    #define DATATYPE_IS_INTEGER   1
    #define DATATYPE_IS_FLOATING  0
    #define DATATYPE_IS_SIGNED    0
    #define DATATYPE_IS_UNSIGNED  1
    #define DATATYPE_IS_FLOAT     0
    #define DATATYPE_IS_DOUBLE    0

#elif DATATYPE == DATATYPE_INT16

    typedef int16_t datatype;

    #define DATATYPE_IS_INTEGER   1
    #define DATATYPE_IS_FLOATING  0
    #define DATATYPE_IS_SIGNED    1
    #define DATATYPE_IS_UNSIGNED  0
    #define DATATYPE_IS_FLOAT     0
    #define DATATYPE_IS_DOUBLE    0

#elif DATATYPE == DATATYPE_UINT16

    typedef uint16_t datatype;

    #define DATATYPE_IS_INTEGER   1
    #define DATATYPE_IS_FLOATING  0
    #define DATATYPE_IS_SIGNED    0
    #define DATATYPE_IS_UNSIGNED  1
    #define DATATYPE_IS_FLOAT     0
    #define DATATYPE_IS_DOUBLE    0

#elif DATATYPE == DATATYPE_INT32

    typedef int32_t datatype;

    #define DATATYPE_IS_INTEGER   1
    #define DATATYPE_IS_FLOATING  0
    #define DATATYPE_IS_SIGNED    1
    #define DATATYPE_IS_UNSIGNED  0
    #define DATATYPE_IS_FLOAT     0
    #define DATATYPE_IS_DOUBLE    0

#elif DATATYPE == DATATYPE_UINT32

    typedef uint32_t datatype;

    #define DATATYPE_IS_INTEGER   1
    #define DATATYPE_IS_FLOATING  0
    #define DATATYPE_IS_SIGNED    0
    #define DATATYPE_IS_UNSIGNED  1
    #define DATATYPE_IS_FLOAT     0
    #define DATATYPE_IS_DOUBLE    0

#elif DATATYPE == DATATYPE_INT64

    typedef int64_t datatype;

    #define DATATYPE_IS_INTEGER   1
    #define DATATYPE_IS_FLOATING  0
    #define DATATYPE_IS_SIGNED    1
    #define DATATYPE_IS_UNSIGNED  0
    #define DATATYPE_IS_FLOAT     0
    #define DATATYPE_IS_DOUBLE    0

#elif DATATYPE == DATATYPE_UINT64

    typedef uint64_t datatype;

    #define DATATYPE_IS_INTEGER   1
    #define DATATYPE_IS_FLOATING  0
    #define DATATYPE_IS_SIGNED    0
    #define DATATYPE_IS_UNSIGNED  1
    #define DATATYPE_IS_FLOAT     0
    #define DATATYPE_IS_DOUBLE    0

#elif DATATYPE == DATATYPE_FLOAT

    typedef float datatype;

    #define DATATYPE_IS_INTEGER   0
    #define DATATYPE_IS_FLOATING  1
    #define DATATYPE_IS_SIGNED    1
    #define DATATYPE_IS_UNSIGNED  0
    #define DATATYPE_IS_FLOAT     1
    #define DATATYPE_IS_DOUBLE    0

#elif DATATYPE == DATATYPE_DOUBLE

    typedef double datatype;

    #define DATATYPE_IS_INTEGER   0
    #define DATATYPE_IS_FLOATING  1
    #define DATATYPE_IS_SIGNED    1
    #define DATATYPE_IS_UNSIGNED  0
    #define DATATYPE_IS_FLOAT     0
    #define DATATYPE_IS_DOUBLE    1

#else

    #error "Unsupported DATATYPE"

#endif


/*
 * Backward compatibility.
 */
#if DATATYPE == DATATYPE_DOUBLE
    #define IS_DOUBLE 1
#else
    #define IS_DOUBLE 0
#endif


/*
 * ============================================================
 * File types
 * ============================================================
 */

#define FILETYPE_CSV 1

#ifndef FILETYPE
#define FILETYPE FILETYPE_CSV
#endif


#endif /* CONFIG_TYPES_H */