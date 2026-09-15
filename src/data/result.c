#include "data/result.h"

// Tabella descrittiva per i campioni grezzi (RawSample)
static const ColumnDesc RAW_SAMPLE_COLUMNS[] = {
    {"id",            offsetof(RawSample, meta.id),            TYPE_INT,    "%d"},
    {"type",          offsetof(RawSample, meta.type),          TYPE_STRING, NULL},
    {"benchname",     offsetof(RawSample, meta.benchname),     TYPE_STRING, NULL},
    {"log2n",         offsetof(RawSample, config.log2n),       TYPE_LONG,   "%ld"},
    {"thread_number", offsetof(RawSample, config.thread_number), TYPE_INT,    "%d"},
    {"chunksize",     offsetof(RawSample, config.chunksize),   TYPE_INT,    "%d"},
    {"run_id",        offsetof(RawSample, run_id),             TYPE_INT,    "%d"},
    {"elapsed_time",  offsetof(RawSample, elapsed_time),       TYPE_DOUBLE, "%.9f"}
};

// Tabella descrittiva per i dati aggregati (AggregatedResult)
static const ColumnDesc AGGREGATED_RESULT_COLUMNS[] = {
    {"id",            offsetof(AggregatedResult, meta.id),            TYPE_INT,    "%d"},
    {"type",          offsetof(AggregatedResult, meta.type),          TYPE_STRING, NULL},
    {"benchname",     offsetof(AggregatedResult, meta.benchname),     TYPE_STRING, NULL},
    {"log2n",         offsetof(AggregatedResult, config.log2n),       TYPE_LONG,   "%ld"},
    {"thread_number", offsetof(AggregatedResult, config.thread_number), TYPE_INT,    "%d"},
    {"chunksize",     offsetof(AggregatedResult, config.chunksize),   TYPE_INT,    "%d"},
    {"mean_time",     offsetof(AggregatedResult, time.mean),          TYPE_DOUBLE, "%.6f"},
    {"min_time",      offsetof(AggregatedResult, time.min),           TYPE_DOUBLE, "%.6f"},
    {"max_time",      offsetof(AggregatedResult, time.max),           TYPE_DOUBLE, "%.6f"},
    {"variance_time", offsetof(AggregatedResult, time.variance),      TYPE_DOUBLE, "%.6e"},
    {"speedup",       offsetof(AggregatedResult, metrics.speedup),    TYPE_DOUBLE, "%.4f"},
    {"efficiency",    offsetof(AggregatedResult, metrics.efficiency), TYPE_DOUBLE, "%.4f"},
    {"overhead",      offsetof(AggregatedResult, metrics.overhead),   TYPE_DOUBLE, "%.6f"}
};

const ColumnDesc* obtain_raw_column_descs(size_t *out_count) {
    if (out_count) {
        *out_count = sizeof(RAW_SAMPLE_COLUMNS) / sizeof(RAW_SAMPLE_COLUMNS[0]);
    }
    return RAW_SAMPLE_COLUMNS;
}

const ColumnDesc* obtain_aggregated_column_descs(size_t *out_count) {
    if (out_count) {
        *out_count = sizeof(AGGREGATED_RESULT_COLUMNS) / sizeof(AGGREGATED_RESULT_COLUMNS[0]);
    }
    return AGGREGATED_RESULT_COLUMNS;
}
