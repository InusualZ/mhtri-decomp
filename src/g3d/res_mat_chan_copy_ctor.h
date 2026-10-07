/* g3d/res_mat_chan_copy_ctor.h - the leaf header of `fn_80059550.cpp`'s ResMatChan handle copy constructor (C linkage). */
#ifndef MHTRI_G3D_RES_MAT_CHAN_COPY_CTOR_H
#define MHTRI_G3D_RES_MAT_CHAN_COPY_CTOR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8005A8E0 (0x30): copy-constructs a channel handle and returns the destination. */
struct ResHandle* res_mat_chan_copy_ctor(struct ResHandle* pDst, struct ResHandle* pSrc);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_RES_MAT_CHAN_COPY_CTOR_H */
