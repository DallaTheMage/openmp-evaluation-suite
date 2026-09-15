#ifndef WRITER_H
#define WRITER_H
    #include "data/writers/types.h"
    #include "config/file.h"

    ResultWriter *create_writer(void);
    void destroy_writer(ResultWriter *writer);
#endif /* WRITER_H */
