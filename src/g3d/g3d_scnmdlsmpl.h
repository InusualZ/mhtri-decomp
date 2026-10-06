/* g3d/g3d_scnmdlsmpl.h - the `ScnMdlSimple` option accessors, node-visibility walk and replacement-buffer helper
 *   `g3d/g3d_scnmdl.cpp` calls (C linkage); the setters take (self, type), and the `type == 5` fast paths are out of
 *   line. */
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

/* The two option setters the game's model users call. */
void fn_80080B10(void* arg0, u32 arg1);
void fn_800810DC(void* arg0, s32 arg1);

#ifdef __cplusplus
}

#include "nw4r/fn_805012C4.h" /* nw4r::math::AABB, owner nw4r/fn_805012C4.cpp (rule 2) */

/* 0x80080F44 - constructs an AABB's two corner records (two VEC3_ctor no-ops) and returns it: the element
 * constructor ScnObj's bounding-box array is built with. */
extern "C" nw4r::math::AABB* AABB_ctor(nw4r::math::AABB* pBox);
#endif

#endif /* MHTRI_G3D_G3D_SCNMDLSMPL_H */
