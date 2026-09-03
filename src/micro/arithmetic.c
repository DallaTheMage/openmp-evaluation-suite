#include "config/types.h"
#include "config/types.h"

#include "micro/arithmetic.h"

datatype arithmetic_step(datatype value)
{
#if DATATYPE_IS_FLOATING
    return value
         * (datatype)1.000001
         + (datatype)0.000001;
#else
    return value
         * (datatype)2
         + (datatype)1;
#endif
}
