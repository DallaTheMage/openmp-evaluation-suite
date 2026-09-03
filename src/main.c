#include <stdio.h>

#include "test/stress.h"
#include "test/weak.h"
#include "test/strong.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <output-file>\n", argv[0]);
        return 1;
    }

    const char *output_file = argv[1];

    printf("\n=== Starting Weak Scaling Test ===\n");
    if (weakScalingTest(output_file) != 0) {
        fprintf(stderr, "Error during Weak Scaling Test execution.\n");
        return 1;
    }

    printf("\n=== Starting Strong Scaling Test ===\n");
    if (strongScalingTest(output_file) != 0) {
        fprintf(stderr, "Error during Strong Scaling Test execution.\n");
        return 1;
    }

    printf("=== Starting Stress Test ===\n");
    if (stressTest(output_file) != 0) {
        fprintf(stderr, "Error during Stress Test execution.\n");
        return 1;
    }

    printf("\nAll benchmarks completed successfully.\n");
    return 0;
}
