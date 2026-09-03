#ifndef MICROROUTINES_SYNCHRONIZATION_H
#define MICROROUTINES_SYNCHRONIZATION_H

#include "config/openmp.h"
#include "core/context.h"

#if OPENMP_HAS_SECTIONS
void routine_sections(WorkContext *ctx);
#endif

#if OPENMP_HAS_BARRIER
void routine_barrier(WorkContext *ctx);
#endif

#if OPENMP_HAS_NOWAIT
void routine_nowait(WorkContext *ctx);
#endif

#if OPENMP_HAS_ORDERED
void routine_ordered(WorkContext *ctx);
#endif

#if OPENMP_HAS_MASKED
void routine_masked(WorkContext *ctx);
#endif

#endif
