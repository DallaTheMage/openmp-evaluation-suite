#ifndef PHASES_H
#define PHASES_H

#include "core/context.h"

GeneralContext* preparation_phase(void);
int test_phase(GeneralContext *gen_ctx);
int postprocess_phase(GeneralContext *gen_ctx, const char *raw_input_filename);

#endif /* PHASES_H */
