#ifndef TYPES_H
#define TYPES_H
    typedef unsigned long uint64;
    typedef long          int64;
    typedef unsigned int  uint32;
    typedef int           int32;

    #ifndef DATATYPE
    #define DATATYPE            double
    #endif /* DATATYPE */

    #ifndef IS_DOUBLE
    #define IS_DOUBLE           1
    #endif /* IS_DOUBLE */

    /* Typedef globale conforme C89 per la precisione selezionata */
    typedef DATATYPE datatype;
#endif /* TYPES_H */