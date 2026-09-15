#include "data/readers/utils.h"
#include <string.h>
#include <stdlib.h>

static char *safe_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *dup = malloc(len);
    if (dup) {
        memcpy(dup, s, len);
    }
    return dup;
}

unsigned short parse_csv_line(FILE *fp, void *struct_ptr, const ColumnDesc *cols, size_t num_cols) {
    char line[1024];
    if (!fp || !fgets(line, sizeof(line), fp)) return 0;

    char *base = (char *)struct_ptr;

    // Rimuove eventuale newline finale (\r o \n)
    line[strcspn(line, "\r\n")] = '\0';

    char *token = strtok(line, ",");
    for (size_t i = 0; i < num_cols && token != NULL; i++) {
        void *field_ptr = base + cols[i].offset;

        switch (cols[i].type) {
            case TYPE_INT:
                *(int *)field_ptr = (int)strtol(token, NULL, 10);
                break;
            case TYPE_LONG:
                *(long *)field_ptr = strtol(token, NULL, 10);
                break;
            case TYPE_DOUBLE:
                *(double *)field_ptr = strtod(token, NULL);
                break;
            case TYPE_STRING:
                *(char **)field_ptr = safe_strdup(token);
                break;
        }
        token = strtok(NULL, ",");
    }
    return 1;
}
