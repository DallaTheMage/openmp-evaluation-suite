#ifndef KERNEL_REGISTRY_H
#define KERNEL_REGISTRY_H

    typedef enum KernelType {
        KERNEL_LOOP = 0,
        KERNEL_REDUCTION,
        KERNEL_SYNC,
        KERNEL_TASKING,
        KERNEL_SIMD,
        KERNEL_NATIVE_SCAN,
        KERNEL_TWO_PASS_SCAN,
        KERNEL_COUNT
    } KernelType;

#endif