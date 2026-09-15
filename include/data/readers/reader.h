#ifndef READER_H
#define READER_H
    #include "data/readers/types.h"
    #include "config/file.h"

    ResultReader *create_reader(void);
    void destroy_reader(ResultReader *reader);
#endif /* READER_H */
