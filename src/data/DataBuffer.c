#define _POSIX_C_SOURCE 200112L
/* DataBuffer.c */
#include <stdlib.h>
#include "data/DataBuffer.h"
#include "core/Configuration.h"

DataBuffer* create_data_buffer(Configuration *config) {
    DataBuffer *buffer = (DataBuffer*)malloc(sizeof(DataBuffer));
    if (!buffer) {
        return NULL;
    }
    buffer->size = config->double_count;
    buffer->alignment = config->memory_alignment;
    int result = posix_memalign((void**)&buffer->pool, config->memory_alignment, config->real_size_bytes);
    bool is_allocated = (result == 0);
    if (!is_allocated) {
        free(buffer);
        return NULL;
    }
    return buffer;
}

void destroy_data_buffer(DataBuffer *buffer) {
    if (!buffer) return;
    if (buffer->pool) free(buffer->pool);
    free(buffer);
}
