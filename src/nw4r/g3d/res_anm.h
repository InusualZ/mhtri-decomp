/*
 * The nw4r g3d animation-channel records that **more than one unit** reconstructs (docs/plan.md 6.5
 * rule 1).  A type two units need is defined once, under `include/`, and included where needed.
 *
 * The two records here are the `ResAnmScn` channel element types whose array strides
 * `g3d/g3d_resanmscn.cpp` indexes: `ResAnmAmbLightData` (stride 0x1C) and `ResAnmFogData` (stride 0x28).
 * `g3d/g3d_resanmamblight.c` (0x80089F94, `__FILE__` `g3d_resanmamblight.cpp`) and `g3d/g3d_resanmfog.cpp`
 * (0x8008F6E8, `__FILE__` `g3d_resanmfog.cpp`) are their first consumers and still carry their own partial
 * copies of these two names in their source (`unused_0x00`/`field_0x04`/`pad_0x08[0xC]`/`field_0x14`/...);
 * folding those onto this header is the residual sweep's job, not this unit's - the definitions here are
 * layout-identical, so the fold is mechanical.  Field names follow the retail record the `*_ac.h` asserts
 * and the nw4r headers evidence (`size`/`toResAnmScnData`/`name`/`id`/`refNumber`); the fields only this
 * unit's getters read keep their name, the untouched tail keeps its offset annotation.
 */
#ifndef MHTRI_NW4R_G3D_RES_ANM_H
#define MHTRI_NW4R_G3D_RES_ANM_H

#include "types.h"

/* The ambient-light channel element.  `g3d/g3d_resanmscn.cpp`'s `toResAnmAmbLightDataArray` index
 * advances by 0x1C, which is this record's size. */
struct ResAnmAmbLightData {
    /* +0x00 */ u32 size;
    /* +0x04 */ s32 toResAnmScnData;
    /* +0x08 */ s32 name;
    /* +0x0C */ u32 id; /* the word the ResAnmScn getter bounds-checks */
    /* +0x10 */ u8 pad_0x10[0x1C - 0x10];
}; /* size: 0x1C */

/* The fog channel element.  The ResAnmScn getter's index stride is 0x28, which is this record's size. */
struct ResAnmFogData {
    /* +0x00 */ u32 size;
    /* +0x04 */ s32 toResAnmScnData;
    /* +0x08 */ s32 name;
    /* +0x0C */ u32 id; /* the word the ResAnmScn getter bounds-checks */
    /* +0x10 */ u8 pad_0x10[0x28 - 0x10];
}; /* size: 0x28 */

#endif /* MHTRI_NW4R_G3D_RES_ANM_H */
