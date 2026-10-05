#ifndef DATA_BUFFER_H
#define DATA_BUFFER_H
    #include <stddef.h>
    #include <core/Configuration.h>

    typedef struct DataBuffer DataBuffer;

    struct DataBuffer {
        double *pool;
        size_t  size;
        size_t  alignment;
    };

    DataBuffer* create_data_buffer(Configuration *config);
    void        destroy_data_buffer(DataBuffer *buffer);
#endif /* DATA_BUFFER_H */
