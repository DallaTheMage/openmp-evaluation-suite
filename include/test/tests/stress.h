// stress.h
#ifndef STRESS_H
#define STRESS_H

#include "core/context.h"
#include "core/logger.h"
#include "data/writers/writer.h"

int stressTest(ResultWriter *writer, WorkContext *ctx, Logger *logger);

#endif /* STRESS_H */
