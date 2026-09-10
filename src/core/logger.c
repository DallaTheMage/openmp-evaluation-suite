#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "core/logger.h"

static void log_message(const char *message) {
    if (LOGGING_ENABLED) {
        printf("[LOG]: %s\n", message);
    }
    return;
}

static void log_info(const char *test_name, bool passed) {
    if (LOGGING_ENABLED) {
        printf("[INFO]: Test '%s' %s\n", test_name, passed ? "PASSED" : "FAILED");
    }
    return;
}

static void log_error(const char *message) {
    if (LOGGING_ENABLED) {
        fprintf(stderr, "[ERROR]: %s\n", message);
    }
    return;
}

Logger *create_logger() {
    Logger *logger = (Logger *)malloc(sizeof(Logger));
    if (logger == NULL) {
        return NULL;
    }

    logger->log = log_message;
    logger->info = log_info;
    logger->error = log_error;

    return logger;
}