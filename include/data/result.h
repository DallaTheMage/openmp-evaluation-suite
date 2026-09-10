#ifndef TEST_RESULT_H
#define TEST_RESULT_H

// Sotto-struct pulite e leggibili
typedef struct {
    int id;
    const char *type;
    const char *benchname;
} TestMeta;

typedef struct {
    long log2n;
    int thread_number;
    int chunksize;
} ExecutionConfig;

typedef struct {
    double mean;
    double min;
    double max;
    double variance;
} SampleStats;

typedef struct {
    double speedup;
    double efficiency;
    double overhead;
} ParallelMetrics;

// Contenitore principale
typedef struct {
    TestMeta meta;
    ExecutionConfig config;
    SampleStats time;
    ParallelMetrics metrics;
} TestResult;

#endif /* TEST_RESULT_H */