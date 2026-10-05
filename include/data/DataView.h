#ifndef DATA_DATAVIEW_H
#define DATA_DATAVIEW_H

#include <stddef.h>
#include <stdint.h>

#include "core/Configuration.h"
#include "data/DataBuffer.h"
#include "data/views/ViewType.h"


/* ========================================================================= */
/* Data window                                                              */
/* ========================================================================= */

typedef struct {
    uint64_t offset;
    uint64_t size;
} DataWindow;


/* ========================================================================= */
/* View metadata                                                            */
/* ========================================================================= */

typedef struct {
    uint64_t rows;
    uint64_t cols;
} View2DMetadata;


typedef struct {
    uint64_t height;
    uint64_t depth;
    uint64_t cols;
} View3DMetadata;


typedef struct {
    uint64_t num_structs;
    uint64_t struct_size;
} ViewAoSMetadata;


typedef struct {
    uint64_t num_fields;
    uint64_t field_len;
} ViewSoAMetadata;


typedef struct {
    uint64_t num_blocks;
    uint64_t vector_length;
    uint64_t num_fields;
} ViewAoSoAMetadata;


typedef struct {
    uint64_t nrows;
    uint64_t ncols;
    uint64_t nnz;

    /*
     * Non-owning pointers.
     *
     * The CSR topology is owned externally, typically by
     * CSRIndexBuffer.
     */
    const uint64_t *row_ptr;
    const uint64_t *col_ind;
} ViewCSRMetadata;


/* ========================================================================= */
/* View metadata union                                                      */
/* ========================================================================= */

typedef union {
    View2DMetadata     v2d;
    View3DMetadata     v3d;
    ViewAoSMetadata    aos;
    ViewSoAMetadata    soa;
    ViewAoSoAMetadata  aosoa;
    ViewCSRMetadata    csr;
} ViewMetadata;


/* ========================================================================= */
/* View creation configuration                                              */
/* ========================================================================= */

/*
 * Runtime descriptor used exclusively when constructing a DataView.
 *
 * Unlike ViewShapeConfig, this type may contain pointers to externally
 * managed runtime data such as CSR topology arrays.
 */

typedef struct {
    uint64_t rows;
    uint64_t cols;
} View2DCreateConfig;


typedef struct {
    uint64_t height;
    uint64_t depth;
    uint64_t cols;
} View3DCreateConfig;


typedef struct {
    uint64_t num_structs;
    uint64_t struct_size;
} ViewAoSCreateConfig;


typedef struct {
    uint64_t num_fields;
    uint64_t field_len;
} ViewSoACreateConfig;


typedef struct {
    uint64_t num_blocks;
    uint64_t vector_length;
    uint64_t num_fields;
} ViewAoSoACreateConfig;


typedef struct {
    uint64_t nrows;
    uint64_t ncols;
    uint64_t nnz;

    const uint64_t *row_ptr;
    const uint64_t *col_ind;
} ViewCSRCreateConfig;


typedef union {
    View2DCreateConfig     v2d;
    View3DCreateConfig     v3d;
    ViewAoSCreateConfig    aos;
    ViewSoACreateConfig    soa;
    ViewAoSoACreateConfig  aosoa;
    ViewCSRCreateConfig    csr;
} ViewCreateConfig;


/* ========================================================================= */
/* DataView                                                                 */
/* ========================================================================= */

/**
 * Non-owning view over a region of a DataBuffer.
 *
 * DataView does not own:
 *
 *   - the DataBuffer;
 *   - CSR row_ptr;
 *   - CSR col_ind.
 */
typedef struct {
    DataBuffer   *buffer;
    ViewType      type;
    DataWindow    window;
    ViewMetadata  meta;
} DataView;


/* ========================================================================= */
/* Constructor                                                              */
/* ========================================================================= */

DataView create_data_view(
    DataBuffer *buffer,
    ViewType type,
    const ViewCreateConfig *config,
    uint64_t offset,
    uint64_t size
);

#endif /* DATA_DATAVIEW_H */
