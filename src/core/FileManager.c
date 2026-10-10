/*
 * Copyright (C) 2026
 *
 * This file is part of the OpenMP compiler-agnostic benchmark suite.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file FileManager.c
 * @brief Filesystem and CSV-file management for benchmark output.
 */

#include "core/FileManager.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define FILE_MANAGER_PATH_MAX 1024U


static int make_directory_recursive(const char *path)
{
    char temporary[FILE_MANAGER_PATH_MAX];
    size_t length;

    if (path == NULL || path[0] == '\0') {
        return -1;
    }

    if (snprintf(temporary, sizeof(temporary), "%s", path) < 0 ||
        strlen(path) >= sizeof(temporary)) {
        return -1;
    }

    length = strlen(temporary);
    while (length > 1U && temporary[length - 1U] == '/') {
        temporary[--length] = '\0';
    }

    for (char *cursor = temporary + 1; *cursor != '\0'; ++cursor) {
        if (*cursor != '/') {
            continue;
        }

        *cursor = '\0';
        if (mkdir(temporary, 0755) != 0 && errno != EEXIST) {
            return -1;
        }
        *cursor = '/';
    }

    if (mkdir(temporary, 0755) != 0 && errno != EEXIST) {
        return -1;
    }

    return 0;
}


static int is_valid_component(const char *component)
{
    if (component == NULL || component[0] == '\0') {
        return 0;
    }

    if (strcmp(component, ".") == 0 || strcmp(component, "..") == 0) {
        return 0;
    }

    for (const char *cursor = component; *cursor != '\0'; ++cursor) {
        if (*cursor == '/' || *cursor == '\\') {
            return 0;
        }
    }

    return 1;
}


static int build_test_directory(const FileManager *manager, char *buffer, size_t buffer_size) {
    int written;
    if (manager == NULL ||
        buffer == NULL ||
        buffer_size == 0U ||
        manager->test_name[0] == '\0') {
        return -1;
    }

    written = snprintf(
        buffer,
        buffer_size,
        "%s/%s/%s/%s",
        manager->base_dir,
        manager->compiler_name,
        manager->schedule_name,
        manager->test_name
    );
    if (written < 0 || (size_t) written >= buffer_size) {
        return -1;
    }
    return 0;
}


FileManager *create_file_manager(const char *base_dir, const char *compiler_name, const char *schedule_name) {
    FileManager *manager;
    int written;

    if (!is_valid_component(compiler_name) ||
        !is_valid_component(schedule_name)) {
        return NULL;
    }

    manager = calloc(1U, sizeof(*manager));
    if (manager == NULL) {
        return NULL;
    }

    if (base_dir == NULL) {
        base_dir = "./output";
    }

    written = snprintf(manager->base_dir, sizeof(manager->base_dir), "%s", base_dir);
    if (written < 0 || (size_t) written >= sizeof(manager->base_dir)) {
        free(manager);
        return NULL;
    }

    written = snprintf(manager->compiler_name, sizeof(manager->compiler_name), "%s", compiler_name);
    if (written < 0 || (size_t) written >= sizeof(manager->compiler_name)) {
        free(manager);
        return NULL;
    }

    written = snprintf(manager->schedule_name, sizeof(manager->schedule_name), "%s", schedule_name);
    if (written < 0 || (size_t) written >= sizeof(manager->schedule_name)) {
        free(manager);
        return NULL;
    }
    return manager;
}


void destroy_file_manager(FileManager *manager) {
    free(manager);
}


int set_current_test(FileManager *manager, const char *test_name) {
    char path[FILE_MANAGER_PATH_MAX];
    int written;

    if (manager == NULL || !is_valid_component(test_name)) {
        return -1;
    }

    written = snprintf(manager->test_name, sizeof(manager->test_name), "%s", test_name);
    if (written < 0 || (size_t) written >= sizeof(manager->test_name)) {
        return -1;
    }

    if (build_test_directory(manager, path, sizeof(path)) != 0 ||
        make_directory_recursive(path) != 0) {
        manager->test_name[0] = '\0';
        return -1;
    }

    return 0;
}

FILE *open_csv_file(FileManager *manager, const char *filename) {
    char directory[FILE_MANAGER_PATH_MAX];
    char file_path[FILE_MANAGER_PATH_MAX];
    size_t filename_length;
    int written;

    if (manager == NULL ||
        manager->test_name[0] == '\0' ||
        !is_valid_component(filename)) {
        return NULL;
    }

    if (build_test_directory(manager, directory, sizeof(directory)) != 0) {
        return NULL;
    }

    filename_length = strlen(filename);
    if (filename_length > 4U &&
        strcmp(filename + filename_length - 4U, ".csv") == 0) {
        written = snprintf(file_path, sizeof(file_path), "%s/%s", directory, filename);
    } else {
        written = snprintf(file_path, sizeof(file_path), "%s/%s.csv", directory, filename);
    }

    if (written < 0 || (size_t) written >= sizeof(file_path)) {
        return NULL;
    }

    return fopen(file_path, "w");
}