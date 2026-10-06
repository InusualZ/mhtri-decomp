/* RVLGX/GXGetTexObjWidth.h - the leaf declarations of `GXGetTexObjWidth` and `GXGetTexObjHeight`, which
 *   `RVLGX/GXTexture_tail.cpp` defines (caller: g3d/g3d_state.cpp). */
#ifndef MHTRI_RVLGX_GXGETTEXOBJWIDTH_H
#define MHTRI_RVLGX_GXGETTEXOBJWIDTH_H

#include "types.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804B8440 - returns the texture object's width. */
u16 GXGetTexObjWidth(const GXTexObj* obj);
/* 0x804B8460 - returns the texture object's height. */
u16 GXGetTexObjHeight(const GXTexObj* obj);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXGETTEXOBJWIDTH_H */
