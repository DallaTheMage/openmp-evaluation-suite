#ifndef MICROROUTINES_TASKS_H
#define MICROROUTINES_TASKS_H

#include "config/openmp.h"
#include "core/context.h"

#if OPENMP_HAS_3_0
void routine_task_divide_conquer(WorkContext *ctx);
#endif

#if OPENMP_HAS_TASKLOOP
void routine_taskloop_scale(WorkContext *ctx);
#endif

#endif
