#ifndef MICROROUTINES_SIMD_H
#define MICROROUTINES_SIMD_H

#include "config/openmp.h"
#include "core/context.h"

#if OPENMP_HAS_SIMD
void routine_simd(WorkContext *ctx);
void routine_parallel_for_simd(WorkContext *ctx);
#endif

#if OPENMP_HAS_LOOP
void routine_loop(WorkContext *ctx);
#endif

#endif
