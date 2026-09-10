#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MKDIR(path) mkdir(path, 0755)

#include "data/writers/utils.h"
#include "config/file.h"

/*
 * Ricrea il comportamento di 'mkdir -p' in modo nativo e sicuro,
 * evitando chiamate a system() e prevenendo la Command Injection.
 */
static void ensure_directories_exist(const char *filepath) {
    char temp[1024];
    snprintf(temp, sizeof(temp), "%s", filepath);

    char *last_slash = strrchr(temp, '/');
    if (last_slash == NULL) {
        return; // Nessuna cartella specificata nel percorso
    }

    *last_slash = '\0'; // Separa la directory dal nome del file

    // Scorriamo il percorso e creiamo ciascuna sottocartella se non esiste
    for (char *p = temp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            MKDIR(temp); // Crea la directory intermedia (se esiste già, fallisce silenziosamente)
            *p = '/';
        }
    }
    MKDIR(temp); // Crea la directory finale
}

char *prepare_filepath(const char *filename, const char *extension) {
    if (filename == NULL || extension == NULL) {
        return NULL;
    }

    const char *path = FILEPATH;
    const char *compiler_name = COMPILERNAME;

    size_t len = strlen(filename);
    size_t path_len = strlen(path);
    size_t compiler_len = strlen(compiler_name);
    size_t extension_len = strlen(extension);

    int need_slash_1 = (path_len > 0 && path[path_len - 1] != '/') ? 1 : 0;
    int need_slash_2 = (compiler_len > 0 && compiler_name[compiler_len - 1] != '/') ? 1 : 0;

    int has_extension = (len >= extension_len) &&
                        (strcmp(filename + len - extension_len, extension) == 0);

    size_t extra_ext_len = has_extension ? 0 : extension_len;

    size_t total_len = path_len + need_slash_1 +
                       compiler_len + need_slash_2 +
                       len + extra_ext_len + 1;

    char *full_path = malloc(total_len);
    if (full_path == NULL) {
        return NULL;
    }

    // 1. Copia FILEPATH
    strcpy(full_path, path);
    size_t offset = path_len;

    if (need_slash_1) {
        full_path[offset++] = '/';
    }

    // 2. Copia COMPILERNAME
    strcpy(full_path + offset, compiler_name);
    offset += compiler_len;

    if (need_slash_2) {
        full_path[offset++] = '/';
    }

    // 3. Copia e sanifica filename (sostituisce spazi e tab con '_')
    for (size_t i = 0; i < len; ++i) {
        char c = filename[i];
        full_path[offset + i] = (c == '\t' || c == ' ') ? '_' : c;
    }
    offset += len;

    // 4. Aggiunge l'estensione se mancante
    if (!has_extension) {
        memcpy(full_path + offset, extension, extension_len);
        offset += extension_len;
    }

    full_path[offset] = '\0';

    // 5. Garantisce la presenza fisica delle directory in modo sicuro e nativo
    ensure_directories_exist(full_path);

    return full_path;
}