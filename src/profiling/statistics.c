#include <stdio.h>
#include <math.h>
#include <omp.h>
#include "profiling/statistics.h"

static double compute_min(const double *data, size_t size) {
    double min_val = data[0];
    size_t i;

    for (i = 1; i < size; ++i) {
        if (data[i] < min_val) {
            min_val = data[i];
        }
    }
    return min_val;
}

static double compute_max(const double *data, size_t size) {
    double max_val = data[0];
    size_t i;

    for (i = 1; i < size; ++i) {
        if (data[i] > max_val) {
            max_val = data[i];
        }
    }
    return max_val;
}

static double compute_geometric_mean(const double *data, size_t size) {
    double log_sum = 0.0;
    size_t i;

    for (i = 0; i < size; ++i) {
        log_sum += log(data[i]);
    }

    return exp(log_sum / (double)size);
}

static void compute_mean_and_variance(const double *data, size_t size, double *mean, double *variance) {
    double sum = 0.0;
    double sum_sq = 0.0;
    size_t i;

    for (i = 0; i < size; ++i) {
        double val = data[i];
        sum += val;
        sum_sq += val * val;
    }

    *mean = compute_geometric_mean(data, size);

    {
        double mean_arithmetic = sum / (double)size;
        double mean_of_squares = sum_sq / (double)size;
        *variance = mean_of_squares - (mean_arithmetic * mean_arithmetic);
    }
}

static double compute_stddev(double variance) {
    return sqrt(variance);
}

void statistics_compute(Statistics *stats, const double *results, size_t size) {
    if (stats == NULL || results == NULL || size == 0) {
        return;
    }

    stats->min = compute_min(results, size);
    stats->max = compute_max(results, size);

    compute_mean_and_variance(results, size, &stats->mean, &stats->variance);

    stats->stddev = compute_stddev(stats->variance);
}
