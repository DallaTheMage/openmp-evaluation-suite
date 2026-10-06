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
 * @file FileManager.h
 * @brief Filesystem and CSV-file management for benchmark output.
 *
 * FileManager owns the output-path state associated with one benchmark run.
 *
 * The directory hierarchy is:
 *
 *     <base_dir>/
 *         <compiler_name>/
 *             <schedule>/
 *                 <test_name>/
 *
 * The compiler and schedule names are copied into the FileManager during
 * construction. The manager therefore does not depend on the lifetime of
 * the Configuration object used to create it.
 *
 * The OpenMP scheduling policy is a compile-time property of the benchmark
 * build. FileManager only records its textual name as output metadata; it
 * never changes or queries the OpenMP runtime scheduling state.
 *
 * FileManager is intentionally independent from the benchmark analysis
 * subsystem. It does not know about RawSample, AggregatedSample or
 * statistical results.
 *
 * The manager is not thread-safe. It is intended to be used by the
 * benchmark control thread.
 */

#ifndef CORE_FILE_MANAGER_H
#define CORE_FILE_MANAGER_H

#include <stdio.h>


/**
 * @brief Maximum length of a FileManager base directory.
 */
#define FILE_MANAGER_BASE_DIR_MAX 256U


/**
 * @brief Maximum length of a compiler name stored by FileManager.
 */
#define FILE_MANAGER_COMPILER_NAME_MAX 32U


/**
 * @brief Maximum length of an OpenMP schedule name stored by FileManager.
 */
#define FILE_MANAGER_SCHEDULE_NAME_MAX 32U


/**
 * @brief Maximum length of a benchmark test name.
 */
#define FILE_MANAGER_TEST_NAME_MAX 64U


/**
 * @brief File manager state for one benchmark output hierarchy.
 *
 * FileManager owns all strings stored in this structure.
 *
 * No pointer in this structure refers to Configuration or other external
 * benchmark state.
 */
typedef struct FileManager {
    char base_dir[FILE_MANAGER_BASE_DIR_MAX];
    char compiler_name[FILE_MANAGER_COMPILER_NAME_MAX];
    char schedule_name[FILE_MANAGER_SCHEDULE_NAME_MAX];
    char test_name[FILE_MANAGER_TEST_NAME_MAX];
} FileManager;


/**
 * @brief Create a FileManager for one benchmark build.
 *
 * The function copies the supplied path and metadata strings into the
 * returned object.
 *
 * If @p base_dir is NULL, "./output" is used.
 *
 * The compiler and schedule names are copied and therefore do not need
 * to remain valid after this function returns.
 *
 * No directory is created by this function. Directory creation is
 * performed when a test is selected with set_current_test().
 *
 * @param base_dir
 *     Root directory for benchmark output. May be NULL.
 *
 * @param compiler_name
 *     Compiler name to include in the output hierarchy.
 *
 * @param schedule_name
 *     Compile-time OpenMP schedule name to include in the output hierarchy.
 *
 * @return
 *     Newly allocated FileManager on success.
 *     NULL on invalid input or allocation failure.
 *
 * @pre compiler_name != NULL.
 * @pre schedule_name != NULL.
 *
 * @note The returned object is owned by the caller.
 */
FileManager *create_file_manager(
    const char *base_dir,
    const char *compiler_name,
    const char *schedule_name
);


/**
 * @brief Destroy a FileManager.
 *
 * All resources owned by the manager are released.
 *
 * @param manager
 *     FileManager to destroy.
 *
 * @note Passing NULL is allowed and has no effect.
 */
void destroy_file_manager(
    FileManager *manager
);


/**
 * @brief Select the current benchmark test and create its output directory.
 *
 * The resulting directory is:
 *
 *     <base_dir>/<compiler_name>/<schedule_name>/<test_name>/
 *
 * If the directory hierarchy already exists, this function succeeds.
 *
 * @param manager
 *     FileManager whose current test is being changed.
 *
 * @param test_name
 *     Name of the benchmark test.
 *
 * @return
 *     0 on success.
 *     Non-zero on invalid input, invalid path components or filesystem
 *     failure.
 *
 * @pre manager != NULL.
 * @pre test_name != NULL.
 *
 * @post On success, manager->test_name contains the selected test name.
 */
int set_current_test(
    FileManager *manager,
    const char *test_name
);


/**
 * @brief Open a CSV file in the current test directory.
 *
 * The supplied filename identifies the CSV file relative to the current
 * test directory.
 *
 * The ".csv" extension is added by the implementation when necessary.
 *
 * Files are opened in write mode and therefore existing contents are
 * replaced.
 *
 * @param manager
 *     FileManager with a selected current test.
 *
 * @param filename
 *     CSV filename without a directory component.
 *
 * @return
 *     Open FILE stream on success.
 *     NULL on invalid input, missing current test or filesystem failure.
 *
 * @pre manager != NULL.
 * @pre filename != NULL.
 * @pre set_current_test() must have completed successfully.
 *
 * @note The caller owns the returned FILE stream and must close it with
 *       fclose().
 */
FILE *open_csv_file(
    FileManager *manager,
    const char *filename
);


#endif /* CORE_FILE_MANAGER_H */
