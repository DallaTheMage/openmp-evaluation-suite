#ifndef TEST_RESULT_H
#define TEST_RESULT_H

#include <stddef.h>

// --- Metadati e Configurazione (Comuni) ---
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

// --- Dati Campionati Grezzi (Misurati a Runtime) ---
typedef struct {
    TestMeta meta;
    ExecutionConfig config;
    int run_id;            // Numero di ripetizione/run
    double elapsed_time;   // Tempo del singolo campionamento
} RawSample;

// --- Dati Aggregati e Metriche (Post-Processing) ---
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

typedef struct {
    TestMeta meta;
    ExecutionConfig config;
    SampleStats time;
    ParallelMetrics metrics;
} AggregatedResult;

// --- Descrittore Generico per Colonna (Per la tabella/CSV generico) ---
typedef enum {
    TYPE_INT,
    TYPE_LONG,
    TYPE_DOUBLE,
    TYPE_STRING
} FieldType;

typedef struct {
    const char *header_name; // Nome della colonna
    size_t offset;           // offsetof(...)
    FieldType type;          // Tipo di dato
    const char *format;      // Formato di stampa es. "%.6f" (opzionale)
} ColumnDesc;

// Funzioni per ottenere il descrittore completo di tutte le colonne
const ColumnDesc* obtain_raw_column_descs(size_t *out_count);
const ColumnDesc* obtain_aggregated_column_descs(size_t *out_count);

// Array di stringhe solo con gli header (se servono indipendentemente)
const char* const* obtain_raw_header_array(void);
const char* const* obtain_aggregated_header_array(void);

#endif /* TEST_RESULT_H */
