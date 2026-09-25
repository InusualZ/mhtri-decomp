/*
 * The `g3d/g3d_calcvtx.cpp` unit's cross-unit declarations (docs/plan.md 6.5 rule 2).
 *
 * `g3d_calcvtx.cpp` (`.text` 0x8007270C-0x800736F8) owns the `ResVtxPos`/`ResVtxClr`/`ResVtxNrm`
 * block accessors and the shape-blend driver `fn_8007270C`.  They were declared in the consumer's own
 * file while the range was unclaimed; a consumer (`g3d/g3d_scnmdl.cpp`, the ScnMdl unit that calls the
 * three symbols below) now includes this header instead.
 *
 * All three keep C linkage (their map names are plain `fn_XXXXXXXX` stems).  `ResVtxNrmBlock`'s full
 * layout belongs to the owner's source; only the incomplete type is needed here.
 */
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

/* Added when `g3d/g3d_resfile.cpp` registered (rule 2): the resource-range store helper. */
void fn_800734E4(void* pBase, u32 size); /* 0x800734E4 - the resource-range store */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_CALCVTX_H */
