#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "core/FileManager.h"
#include "config/params.h"


/* ========================================================================= */
/* --- Internal helpers --------------------------------------------------- */
/* ========================================================================= */

/**
 * Creates a directory hierarchy recursively.
 *
 * Example:
 *
 *     ./output/gcc/static/test1
 */
static int make_directory_recursive(
    const char *path
)
{
    if (!path || path[0] == '\0')
        return -1;


    char temp[512];

    int written = snprintf(
        temp,
        sizeof(temp),
        "%s",
        path
    );

    if (written < 0 || (size_t)written >= sizeof(temp))
        return -1;


    size_t len = strlen(temp);

    if (len == 0)
        return -1;


    /*
     * Remove trailing slash, except for the root directory.
     */
    if (len > 1 && temp[len - 1] == '/') {
        temp[len - 1] = '\0';
        --len;
    }


    /*
     * Create each intermediate directory.
     */
    for (char *p = temp + 1; *p != '\0'; ++p) {

        if (*p != '/')
            continue;


        *p = '\0';

        if (mkdir(temp, 0755) != 0 &&
            errno != EEXIST) {

            return -1;
        }

        *p = '/';
    }


    /*
     * Create the final directory.
     */
    if (mkdir(temp, 0755) != 0 &&
        errno != EEXIST) {

        return -1;
    }


    return 0;
}


/* ========================================================================= */

/**
 * Returns the textual representation of the configured OpenMP schedule.
 */
static const char *schedule_name(
    int schedule_id
)
{
    switch (schedule_id) {

        case SCHED_STATIC:
            return "static";

        case SCHED_DYNAMIC:
            return "dynamic";

        case SCHED_GUIDED:
            return "guided";

        case SCHED_RUNTIME:
            return "runtime";

        default:
            return "unknown";
    }
}


/* ========================================================================= */

/**
 * Builds the directory associated with the current test.
 *
 *     <base_dir>/<compiler_name>/<schedule>/<test_name>
 */
static int build_test_directory(
    const FileManager *manager,
    char *buffer,
    size_t buffer_size
)
{
    if (!manager ||
        !manager->config ||
        !buffer ||
        buffer_size == 0 ||
        manager->test_name[0] == '\0') {

        return -1;
    }


    const Configuration *config =
        manager->config;

    const char *compiler =
        config->build.compiler_name;

    const char *schedule =
        schedule_name(
            config->execution.schedule_id
        );


    int written = snprintf(
        buffer,
        buffer_size,
        "%s/%s/%s/%s",
        manager->base_dir,
        compiler,
        schedule,
        manager->test_name
    );


    if (written < 0 ||
        (size_t)written >= buffer_size) {

        return -1;
    }


    return 0;
}


/* ========================================================================= */
/* --- Lifecycle ---------------------------------------------------------- */
/* ========================================================================= */

FileManager *create_file_manager(
    const char *base_dir,
    const Configuration *config
)
{
    if (!config)
        return NULL;


    FileManager *manager =
        calloc(1, sizeof(FileManager));

    if (!manager)
        return NULL;


    const char *directory =
        base_dir
            ? base_dir
            : "./output";


    int written = snprintf(
        manager->base_dir,
        sizeof(manager->base_dir),
        "%s",
        directory
    );


    if (written < 0 ||
        (size_t)written >= sizeof(manager->base_dir)) {

        free(manager);
        return NULL;
    }


    manager->config =
        config;

    manager->test_name[0] =
        '\0';


    return manager;
}


/* ========================================================================= */

void destroy_file_manager(
    FileManager *manager
)
{
    if (!manager)
        return;

    /*
     * Configuration is non-owning.
     */
    manager->config = NULL;

    free(manager);
}


/* ========================================================================= */
/* --- Test context ------------------------------------------------------- */
/* ========================================================================= */

int set_current_test(
    FileManager *manager,
    const char *test_name
)
{
    if (!manager ||
        !manager->config ||
        !test_name ||
        test_name[0] == '\0') {

        return -1;
    }


    int written = snprintf(
        manager->test_name,
        sizeof(manager->test_name),
        "%s",
        test_name
    );


    if (written < 0 ||
        (size_t)written >= sizeof(manager->test_name)) {

        return -1;
    }


    char path[512];

    if (build_test_directory(
            manager,
            path,
            sizeof(path)) != 0) {

        manager->test_name[0] =
            '\0';

        return -1;
    }


    if (make_directory_recursive(path) != 0) {

        manager->test_name[0] =
            '\0';

        return -1;
    }


    return 0;
}


/* ========================================================================= */
/* --- CSV files ---------------------------------------------------------- */
/* ========================================================================= */

FILE *open_csv_file(
    FileManager *manager,
    const char *filename
)
{
    if (!manager ||
        !manager->config ||
        manager->test_name[0] == '\0' ||
        !filename ||
        filename[0] == '\0') {

        return NULL;
    }


    char directory[512];

    if (build_test_directory(
            manager,
            directory,
            sizeof(directory)) != 0) {

        return NULL;
    }


    char file_path[768];

    int written = snprintf(
        file_path,
        sizeof(file_path),
        "%s/%s.csv",
        directory,
        filename
    );


    if (written < 0 ||
        (size_t)written >= sizeof(file_path)) {

        return NULL;
    }


    return fopen(
        file_path,
        "w"
    );
}
