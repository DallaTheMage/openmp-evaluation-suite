#include "test/stress.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
   if (argc < 2) {
      fprintf(stderr, "Usage: %s <output-file>\n", argv[0]);
      return 1;
   }
   const char *output_file = argv[1];
   stressTest(output_file);
   return 0;
}
