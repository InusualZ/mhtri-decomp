/* face_spr.h - the face-sprite records: `fn_80047398.cpp` carves and clears the two `FaceSprBuf`s (`face_buf_tbl`),
 * `userdata_item.cpp`'s sprite queue fills and draws them (rule 1: the type both units use, defined once). */
#ifndef MHTRI_FACE_SPR_H
#define MHTRI_FACE_SPR_H

#include "types.h"

/* One queued face sprite (`face_spr_add` copies it whole): the screen rectangle, the depth, the texel rectangle
 * normalised to 0..1 and the vertex colour.  size: 0x24 */
struct FaceSpr {
    /* +0x00 */ u8 unused_0x00[4];
    /* +0x04 */ s16 x0;
    /* +0x06 */ s16 x1;
    /* +0x08 */ s16 y0;
    /* +0x0A */ s16 y1;
    /* +0x0C */ f32 depth;
    /* +0x10 */ f32 s0;
    /* +0x14 */ f32 s1;
    /* +0x18 */ f32 t0;
    /* +0x1C */ f32 t1;
    /* +0x20 */ u32 color;
};

/* One face-sprite buffer (`face_buf_tbl`'s two): 32 runs, each a texture slot (-1 free), its sprite count and up to
 * 16 sprites.  size: 0x4880 */
struct FaceSprBuf {
    /* +0x0000 */ s16 tex[0x20];
    /* +0x0040 */ s16 count[0x20];
    /* +0x0080 */ FaceSpr spr[0x20][0x10];
};

#endif /* MHTRI_FACE_SPR_H */
