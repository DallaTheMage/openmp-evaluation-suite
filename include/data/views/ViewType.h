#ifndef DATA_VIEW_TYPE_H
#define DATA_VIEW_TYPE_H

/* Tag della union in DataView: uno per ciascun layout di interpretazione
 * zero-copy supportato sul pool. VIEW_TYPE_COUNT non e' un tipo valido:
 * serve solo a dimensionare tabelle di dispatch (es. shape_fn[VIEW_TYPE_COUNT]). */
typedef enum {
    VIEW_1D = 0,
    VIEW_2D,
    VIEW_3D,
    VIEW_CSR,
    VIEW_AOS,
    VIEW_SOA,
    VIEW_AOSOA,
    VIEW_TYPE_COUNT
} ViewType;

#endif /* DATA_VIEW_TYPE_H */