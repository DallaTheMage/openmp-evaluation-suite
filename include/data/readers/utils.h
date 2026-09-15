#ifndef READER_UTILS_H
#define READER_UTILS_H

#include <stdio.h>
#include <stddef.h>
#include "data/result.h"

// Legge una riga dal FILE e parsa i valori basandosi sui descrittori di colonna
// Restituisce 1 se la riga è stata letta con successo, 0 se EOF o errore di parsing.
unsigned short parse_csv_line(FILE *fp, void *struct_ptr, const ColumnDesc *cols, size_t num_cols);

#endif /* READER_UTILS_H */
