#ifndef CSV_WRITER_H
#define CSV_WRITER_H
    #include "data/writers/types.h"

    ResultWriter *create_csv_writer(void);
    void destroy_csv_writer(ResultWriter *writer);
#endif
