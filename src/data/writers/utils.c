#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "data/writers/utils.h"

// Creazione ricorsiva delle cartelle in C99 / POSIX
static void create_dir_recursive(const char *path) {
    if (!path || !*path) return;

    char temp[512];
    snprintf(temp, sizeof(temp), "%s", path);
    size_t len = strlen(temp);
    if (len > 0 && temp[len - 1] == '/') temp[len - 1] = '\0';

    for (char *p = temp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(temp, 0755);
            *p = '/';
        }
    }
    mkdir(temp, 0755);
}

// Genera il percorso: output/[compiler_name]/samples/[test_name].csv
char *prepare_filepath(const char *compiler_name, const char *test_name) {
    if (!compiler_name || !test_name) return NULL;

    char dir_path[512];
    snprintf(dir_path, sizeof(dir_path), "output/%s/samples", compiler_name);

    // Crea la gerarchia di directory se non esiste già
    create_dir_recursive(dir_path);

    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s.csv", dir_path, test_name);

    size_t len = strlen(full_path) + 1;
    char *result = malloc(len);
    if (result) {
        memcpy(result, full_path, len);
    }
    return result;
}

void write_csv_header_from_desc(FILE *fp, const ColumnDesc *cols, size_t num_cols) {
    if (!fp || !cols) return;
    for (size_t i = 0; i < num_cols; i++) {
        fprintf(fp, "%s%s", cols[i].header_name, (i < num_cols - 1) ? "," : "\n");
    }
}

void write_csv_row_from_desc(FILE *fp, const void *struct_ptr, const ColumnDesc *cols, size_t num_cols) {
    if (!fp || !struct_ptr || !cols) return;
    const char *base = (const char *)struct_ptr;

    for (size_t i = 0; i < num_cols; i++) {
        const void *field_ptr = base + cols[i].offset;

        switch (cols[i].type) {
            case TYPE_INT:
                fprintf(fp, cols[i].format ? cols[i].format : "%d", *(const int *)field_ptr);
                break;
            case TYPE_LONG:
                fprintf(fp, cols[i].format ? cols[i].format : "%ld", *(const long *)field_ptr);
                break;
            case TYPE_DOUBLE:
                fprintf(fp, cols[i].format ? cols[i].format : "%f", *(const double *)field_ptr);
                break;
            case TYPE_STRING: {
                const char *str = *(const char * const *)field_ptr;
                fprintf(fp, "%s", str ? str : "");
                break;
            }
        }
        fprintf(fp, "%s", (i < num_cols - 1) ? "," : "\n");
    }
}
