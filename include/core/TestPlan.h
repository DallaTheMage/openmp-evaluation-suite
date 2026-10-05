#ifndef CORE_TEST_PLAN_H
#define CORE_TEST_PLAN_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "core/Configuration.h"
#include "kernels/types.h"
#include "data/DataView.h"

typedef struct DataBuffer DataBuffer;

typedef uint64_t TestSetUID;
typedef uint64_t TestCaseUID;


typedef struct TestCase {
    TestCaseUID uid;
    uint32_t    num_threads;
    uint64_t    chunk_size;
} TestCase;

typedef struct TestCaseArray {
    TestCase *data;
    size_t    count;
} TestCaseArray;


typedef struct KernelTestGroup {
    KernelType    kernel;
    TestCaseArray cases;
} KernelTestGroup;

typedef struct KernelTestGroupArray {
    KernelTestGroup *data;
    size_t           count;
} KernelTestGroupArray;


typedef struct TestSet {
    TestSetUID           uid;
    DataView             view;
    KernelTestGroupArray kernels;
} TestSet;


typedef struct TestPlan {
    TestSet *data;
    size_t   count;
} TestPlan;


TestPlan *generate_proportional_test_plan(
    DataBuffer *buffer,
    const Configuration *config
);

TestPlan *generate_full_scale_test_plan(
    DataBuffer *buffer,
    const Configuration *config
);

bool validate_test_plan(
    const TestPlan *plan
);

void destroy_test_plan(
    TestPlan *plan
);

#endif /* CORE_TEST_PLAN_H */
