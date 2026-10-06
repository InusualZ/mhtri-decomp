/* g3d/g3d_calcvtx.h - the shape-blend driver fn_8007270C and the two handle-copy helpers `g3d/g3d_calcvtx.cpp`
 *   owns (C linkage); its accessor copies are declared by their classes (g3d/g3d_resvtx.h). */
#ifndef MHTRI_G3D_G3D_CALCVTX_H
#define MHTRI_G3D_G3D_CALCVTX_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8007270C - blends the animated shapes over the model's vertex arrays (the three tables are the position,
 * normal and colour arrays the blend writes). */
void fn_8007270C(void* pMdl, void* pAnmObjShp, const void** vtxPosTable, const void** vtxNrmTable,
                 const void** vtxClrTable); /* untyped: opaque handle */

/* 0x800732C0/0x800734D8 - copy a ResVtxClr/ResVtxNrm handle word. */
void CopyResVtxClrHandle(void* pDst, const void* pSrc); /* untyped: opaque handle */
void CopyResVtxNrmHandle(void* pDst, const void* pSrc); /* untyped: opaque handle */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_CALCVTX_H */
