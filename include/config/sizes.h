#ifndef CONFIG_SIZES_H
#define CONFIG_SIZES_H

   #ifndef MAX_PROBLEM_SIZE
      #define MAX_PROBLEM_SIZE 24U
   #endif

   #ifndef WORK_SIZE_SLICES
      #define WORK_SIZE_SLICES 3U
   #endif

   #ifndef WEAK_SCALING_SIZE
      #define WEAK_SCALING_SIZE (MAX_PROBLEM_SIZE / WORK_SIZE_SLICES)
   #endif

   #ifndef STRONG_SCALING_SIZE
      #define STRONG_SCALING_SIZE MAX_PROBLEM_SIZE
   #endif

#endif /* CONFIG_SIZES_H */
