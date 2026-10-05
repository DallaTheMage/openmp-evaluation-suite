#include <stdio.h>
#include <stdlib.h>
#include "core/Configuration.h"
#include "core/ScalingPoint.h"
#include "data/DataBuffer.h"
#include "data/DataView.h"

int main(void) {
    /* 1. Caricamento configurazione */
    Configuration config = load_configuration();

    /* 2. Allocazione del DataBuffer principale */
    DataBuffer *buffer = create_data_buffer(&config);
    if (!buffer) {
        destroy_configuration(&config);
        return EXIT_FAILURE;
    }

    /* 3. Generazione della suite di esperimenti */
    ExperimentSuite *suite = generate_full_scale_suite(buffer, &config);
    if (!suite) {
        destroy_data_buffer(buffer);
        destroy_configuration(&config);
        return EXIT_FAILURE;
    }

    printf("=== Avvio Sweep Completo (%lu punti) ===\n", suite->points.count);
    for (size_t i = 0; i < suite->points.count; i++) {
        ScalingPoint *pt = &suite->points.data[i];
        printf("[Punto %3lu] Window Size: %lu elems | Layouts: %lu\n",
               i,
               pt->window_size,
               pt->layouts.count);
    }

    /* 4. Pulizia delle risorse */
    destroy_experiment_suite(suite);
    destroy_data_buffer(buffer);
    destroy_configuration(&config);

    return EXIT_SUCCESS;
}