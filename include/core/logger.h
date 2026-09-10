#ifndef LOGGER_H
#define LOGGER_H
    #include <stdbool.h>
    #include "config/logging.h"

    typedef struct Logger Logger;

    struct Logger {
        void (*log)(const char *message);
        void (*info) (const char *test_name, bool passed);
        void (*error)(const char *message);
    };

    Logger *create_logger();
#endif /* LOGGER_H */