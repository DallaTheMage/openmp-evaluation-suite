#ifndef STRESS_H
#define STRESS_H
    #include "core/context.h"
    #include "data/generator.h"
    #include "data/writers/writer.h"

    int stressTest(ResultWriter *writer, WorkContext *ctx);
#endif /* STRESS_H */
