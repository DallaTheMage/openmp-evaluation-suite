#ifndef RESULT_H
#define RESULT_H

typedef struct {
    const char *benchname;
    long log2n;
    int threadnumber;
    int chunksize;
    double time;
    double speedup;
    double overhead;
} Result;

#endif /* RESULT_H */