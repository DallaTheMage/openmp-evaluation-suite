#ifndef CORE_FILE_MANAGER_H
#define CORE_FILE_MANAGER_H

#include <stdio.h>

#include "core/Configuration.h"


/* ========================================================================= */
/* --- FileManager -------------------------------------------------------- */
/* ========================================================================= */

/**
 * Gestisce il filesystem associato a una singola run di O.E.S.
 *
 * La struttura delle directory viene derivata dalla Configuration:
 *
 *     <base_dir>/
 *         <compiler_name>/
 *             <schedule>/
 *                 <test_name>/
 *
 * FileManager non possiede la Configuration e non ne modifica
 * alcun campo.
 */
typedef struct FileManager {

    char base_dir[256];

    char test_name[64];

    const Configuration *config;

} FileManager;


/* ========================================================================= */
/* --- Lifecycle ---------------------------------------------------------- */
/* ========================================================================= */

/**
 * Crea un FileManager associato alla Configuration fornita.
 *
 * base_dir può essere NULL; in tal caso viene utilizzato "./output".
 *
 * La Configuration deve rimanere valida per tutta la vita del
 * FileManager.
 */
FileManager *create_file_manager(
    const char *base_dir,
    const Configuration *config
);


/**
 * Distrugge il FileManager.
 *
 * La Configuration non viene liberata.
 */
void destroy_file_manager(
    FileManager *manager
);


/* ========================================================================= */
/* --- Test context ------------------------------------------------------- */
/* ========================================================================= */

/**
 * Imposta il test corrente e crea automaticamente la struttura
 * di directory necessaria.
 *
 * Il percorso risultante è:
 *
 *     <base_dir>/<compiler_name>/<schedule>/<test_name>/
 */
int set_current_test(
    FileManager *manager,
    const char *test_name
);


/* ========================================================================= */
/* --- CSV files ---------------------------------------------------------- */
/* ========================================================================= */

/**
 * Apre un CSV appartenente al test corrente.
 *
 * Esempio:
 *
 *     open_csv_file(manager, "raw_samples")
 *
 * apre:
 *
 *     <base_dir>/<compiler>/<schedule>/<test>/raw_samples.csv
 *
 * Il file viene aperto in modalità "w".
 */
FILE *open_csv_file(
    FileManager *manager,
    const char *filename
);

#endif /* CORE_FILE_MANAGER_H */
