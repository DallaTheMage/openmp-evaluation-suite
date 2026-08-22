#ifndef MICROROUTINES_H
#define MICROROUTINES_H

#include "core/context.h"
#include "config/configuration.h"

typedef struct {
    const char *name;
    void (*run)(WorkContext *ctx);
} Routine;

void routine_micro_scale(WorkContext *ctx);
void routine_matrix_row_best(WorkContext *ctx);
void routine_matrix_col_worst(WorkContext *ctx);
void routine_reduction_sum(WorkContext *ctx);
void routine_scan_inclusive(WorkContext *ctx);
void routine_task_divide_conquer(WorkContext *ctx);
void routine_atomic_reduction(WorkContext *ctx);
void routine_taskloop_scale(WorkContext *ctx);

#endif /* MICROROUTINES_H */