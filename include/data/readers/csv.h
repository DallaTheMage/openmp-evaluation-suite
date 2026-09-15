#ifndef CSV_READER_H
#define CSV_READER_H
    #include "data/readers/types.h"

    ResultReader *create_csv_reader(void);
    void destroy_csv_reader(ResultReader *reader);

#endif
