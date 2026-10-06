/* g3d/g3d_calcvtx.h - the `ResVtxPos`/`ResVtxClr`/`ResVtxNrm` block accessors and the shape-blend driver
 *   fn_8007270C that `g3d/g3d_calcvtx.cpp` owns (C linkage); `ResVtxNrmBlock` stays an incomplete type here. */
#ifndef MHTRI_G3D_G3D_CALCVTX_H
#define MHTRI_G3D_G3D_CALCVTX_H

#include "types.h"
#include "nw4r/g3d/res_common.h"

/* The resource block the vertex accessors hand back (the owner's own `ResVtxNrmBlock`). */
struct ResVtxNrmBlock;

#ifdef __cplusplus
extern "C" {
#endif

struct ResVtxNrmBlock* fn_800730D8(ResHandle* pSelf); /* 0x800730D8 - the `ResVtxNrm` block */
void* fn_800732F0(ResHandle* pSelf);                  /* 0x800732F0 - the `ResVtxClr` block */
void fn_8007270C(void* pMdl, void* pAnmObjShp, const void** vtxPosTable, const void** pClrTable,
                const void** pTexTable); /* 0x8007270C - the shape-blend driver (the three tables are its
                                           * vertex-position/colour/tex-coord inputs) */

/* The resource-range store helper `g3d/g3d_resfile.cpp` calls. */
void fn_800734E4(void* pBase, u32 size); /* 0x800734E4 - the resource-range store */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_CALCVTX_H */
