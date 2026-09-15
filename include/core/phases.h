#ifndef PHASES_H
#define PHASES_H

#include "core/logger.h"

int run_test_phase(const char *raw_output_filename, Logger *logger);
int run_postprocess_phase(const char *raw_input_filename, const char *aggregated_output_filename, Logger *logger);

#endif /* PHASES_H */
