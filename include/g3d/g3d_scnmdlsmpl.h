/*
 * The `g3d/g3d_scnmdlsmpl.cpp` unit's cross-unit declarations (docs/plan.md 6.5 rule 2).
 *
 * `g3d_scnmdlsmpl.cpp` (`.text` 0x8007F0E4-0x800813B8) is the `ScnMdlSimple` scene-model object - the
 * base class `g3d/g3d_scnmdl.cpp`'s ScnMdl builds on.  Its option accessors, its node-visibility walk
 * and its replacement-buffer helper are the ones the ScnMdl bodies call.
 *
 * All keep C linkage (their map names are plain `fn_XXXXXXXX`/`dtor_XXXXXXXX` stems).  The signatures
 * are the ones the target bodies imply: the setters take (self, type) and the `type == 5` fast paths
 * are out of line here.
 */
#ifndef MHTRI_G3D_G3D_SCNMDLSMPL_H
#define MHTRI_G3D_G3D_SCNMDLSMPL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The `type == 5` option paths the ScnMdl setter/getter pairs tail-call. */
u32 fn_800808C4(void* pSelf, u32 type);  /* 0x800808C4 - the option getter */
u32 fn_800809A4(void* pSelf, u32 type);  /* 0x800809A4 - the second option getter */
u32 fn_80080A00(void* pSelf, u32 type);  /* 0x80080A00 - the second option setter */
void fn_80080A5C(void* pSelf, u32 type, u32 on); /* 0x80080A5C - the shared option setter */
u32 fn_8007FFC4(void* pSelf, u32 type, u32 on);  /* 0x8007FFC4 - the base's option setter */
u32 fn_80080004(void* pSelf, u32 type, u32* pOut); /* 0x80080004 - the base's option query */

/* The replacement-buffer counts and the draw-buffer builder's arguments. */
u32 fn_80080B5C(void* pSelf);            /* 0x80080B5C - the first buffer count */
u32 fn_80080BA8(void* pSelf);            /* 0x80080BA8 - the second buffer count */
u32 fn_80080C04(void* pSelf);            /* 0x80080C04 - the third buffer count */
void dtor_80080F7C(void* pSelf, s32 flag); /* 0x80080F7C - the base's teardown */
u32 fn_8007F41C(void* pSelf, u32* pArg2, u32* pArg3); /* 0x8007F41C - the copied-material pass */
void fn_80080C60(void* pSelf, void* pArg2, u32* pArg3); /* 0x80080C60 - the ScnMdlSimple base constructor */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_SCNMDLSMPL_H */
