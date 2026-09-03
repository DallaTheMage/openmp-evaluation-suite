#ifndef MICROROUTINES_REDUCTION_H
#define MICROROUTINES_REDUCTION_H

#include "config/openmp.h"
#include "core/context.h"

void routine_reduction_sum(WorkContext *ctx);

#if OPENMP_HAS_SCAN
void routine_scan_inclusive(WorkContext *ctx);
#endif

#if OPENMP_HAS_CRITICAL
void routine_critical_reduction(WorkContext *ctx);
#endif

#if OPENMP_HAS_ATOMIC
void routine_atomic_reduction(WorkContext *ctx);
#endif

#endif
