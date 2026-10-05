#ifndef DATA_VIEW_SHAPE_FNS_H
#define DATA_VIEW_SHAPE_FNS_H
    #include <stdbool.h>
    #include "data/DataView.h"

    /* Ricalcola meta.* in base a window.size (e ai parametri fissi gia'
     * impostati alla creazione). Chiamata una volta per view_bind_window,
     * mai dentro il loop caldo di un kernel. */
    void shape_1d_recompute(DataView *view);
    void shape_2d_recompute(DataView *view);
    void shape_3d_recompute(DataView *view);
    void shape_csr_recompute(DataView *view);
    void shape_aos_recompute(DataView *view);
    void shape_soa_recompute(DataView *view);
    void shape_aosoa_recompute(DataView *view);

    /* Sanity check opzionale post-bind (usato da view_bind_window in debug
     * build o su richiesta esplicita, mai obbligatorio nel percorso caldo). */
    bool shape_1d_validate(const DataView *view);
    bool shape_2d_validate(const DataView *view);
    bool shape_3d_validate(const DataView *view);
    bool shape_csr_validate(const DataView *view);
    bool shape_aos_validate(const DataView *view);
    bool shape_soa_validate(const DataView *view);
    bool shape_aosoa_validate(const DataView *view);

#endif /* DATA_VIEW_SHAPE_FNS_H */