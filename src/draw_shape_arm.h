/*
 * The data `src/draw_shape_arm.cpp` owns that `draw_shape.cpp` reads too (docs/plan.md 6.5 rule 2): the `.sdata2` float pool of the 2D shape layer
 * (0x80795C18..0x80795C48), split between the two units by phase 4 when it cut `draw_shape.cpp` at 0x80055EC4.  Named from the values and the
 * code that reads them (they were `lbl_80795C18/1C/24/40/44`): the pool holds 0.0f, -1.0f, 1.0f and the two copy-scale factors 0.05f and 0.01f.
 */
#ifndef MHTRI_DRAW_SHAPE_ARM_H
#define MHTRI_DRAW_SHAPE_ARM_H

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern f32 draw_shape_f32_zero;        /* 0.0f   .sdata2 0x80795C18 */
extern f32 draw_shape_f32_neg_one;     /* -1.0f  .sdata2 0x80795C1C */
extern f32 draw_shape_f32_one;         /* 1.0f   .sdata2 0x80795C24 */
extern f32 draw_shape_sprite_scale_x;  /* 0.05f  .sdata2 0x80795C40 */
extern f32 draw_shape_sprite_scale_y;  /* 0.01f  .sdata2 0x80795C44 */

/* `lbl_8066ACF8` (0x8066ACF8, .bss): the draw-shape state block - blend mode/alpha (0x3C..0x3F), the
 * two scale floats, the tint colour at +0x50, the resource handle and position, and the 2D sprite
 * state `fn_80056AF0` walks.  Only the bytes this range touches are named. size: 0x138 */
typedef struct DrawShapeState {
    /* +0x00 */ u8 pad_0x00[0x3C];
    /* +0x3C */ u8 field_0x3C;      /* active flag (fn_80056A20/3C) */
    /* +0x3D */ u8 field_0x3D;
    /* +0x3E */ u8 field_0x3E;      /* alpha */
    /* +0x3F */ u8 field_0x3F;      /* blend mode */
    /* +0x40 */ u8 pad_0x40[0x04];
    /* +0x44 */ f32 field_0x44;     /* scale x */
    /* +0x48 */ f32 field_0x48;     /* scale y */
    /* +0x4C */ u8 pad_0x4C[0x04];
    /* +0x50 */ GXColor color_0x50; /* tint */
    /* +0x54 */ u32 field_0x54;
    /* +0x58 */ u32 field_0x58;
    /* +0x5C */ u8 field_0x5C;      /* draw enabled */
    /* +0x5D */ u8 field_0x5D;
    /* +0x5E */ u8 pad_0x5E[0x02];
    /* +0x60 */ u8 field_0x60;
    /* +0x61 */ u8 pad_0x61[0x01];
    /* +0x62 */ u8 field_0x62;
    /* +0x63 */ u8 field_0x63;
    /* +0x64 */ nw4r::math::VEC3 vec_0x64;  /* position */
    /* +0x70 */ u32 field_0x70;
    /* +0x74 */ u8 field_0x74;      /* fn_80056F04 clears it */
    /* +0x75 */ u8 pad_0x75[0xBF];
    /* +0x134 */ u32 field_0x134;   /* copied into the 2D sprite's +0x38 */
} DrawShapeState; /* size: 0x138 */
#endif

#endif /* MHTRI_DRAW_SHAPE_ARM_H */
