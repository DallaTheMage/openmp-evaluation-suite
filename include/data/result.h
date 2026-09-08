#ifndef RESULT_H
#define RESULT_H

typedef struct {
    int test_id;
    const char *test_type;
    const char *benchname;
    long log2n;
    int threadnumber;
    int chunksize;
    double time;
    double speedup;
    double efficiency;
    double overhead;
} Result;

#endif /* RESULT_H */
