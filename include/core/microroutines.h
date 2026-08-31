#ifndef MICROROUTINES_H
#define MICROROUTINES_H

#include <stddef.h>

#include "core/context.h"


typedef struct {
    const char *name;
    void (*run)(WorkContext *ctx);
} MicroRoutine;


const MicroRoutine *get_microroutines(void);

size_t get_microroutines_count(void);


void routine_micro_scale(WorkContext *ctx);
void routine_matrix_row_best(WorkContext *ctx);
void routine_matrix_col_worst(WorkContext *ctx);
void routine_matrix_collapse(WorkContext *ctx);
void routine_for(WorkContext *ctx);
void routine_reduction_sum(WorkContext *ctx);
void routine_scan_inclusive(WorkContext *ctx);
void routine_sections(WorkContext *ctx);
void routine_critical_reduction(WorkContext *ctx);
void routine_atomic_reduction(WorkContext *ctx);
void routine_barrier(WorkContext *ctx);
void routine_nowait(WorkContext *ctx);
void routine_ordered(WorkContext *ctx);
void routine_task_divide_conquer(WorkContext *ctx);
void routine_taskloop_scale(WorkContext *ctx);
void routine_simd(WorkContext *ctx);
void routine_parallel_for_simd(WorkContext *ctx);
void routine_masked(WorkContext *ctx);
void routine_loop(WorkContext *ctx);

#endif
