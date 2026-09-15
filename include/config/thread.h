#ifndef THREAD_CONFIG_H
#define THREAD_CONFIG_H
    #define CUSTOM_THREAD_NUMBERS 8
    #ifndef CUSTOM_THREAD_NUMBERS
        #define STRESS_THREADS { 1 }
    #else
        #define STRESS_THREADS { 1, CUSTOM_THREAD_NUMBERS }
    #endif
#endif /* THREAD_CONFIG_H */
