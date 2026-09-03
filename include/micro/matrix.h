#ifndef MICROROUTINES_MATRIX_H
#define MICROROUTINES_MATRIX_H

#include "core/context.h"

void routine_matrix_row_best(WorkContext *ctx);
void routine_matrix_col_worst(WorkContext *ctx);

#if OPENMP_HAS_COLLAPSE
void routine_matrix_collapse(WorkContext *ctx);
#endif

#endif
