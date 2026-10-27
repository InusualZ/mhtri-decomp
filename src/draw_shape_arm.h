/*
 * The declarations `src/draw_shape_arm.cpp` owns (docs/plan.md 6.5 rule 2): the screen-sprite record `DrawSpriteSlot` (the
 * captured-screen overlay at +0x3C of the draw state block `lbl_8066ACF8`), the view of that block this unit writes through, and
 * the unit's functions.
 */
#ifndef MHTRI_DRAW_SHAPE_ARM_H
#define MHTRI_DRAW_SHAPE_ARM_H

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"

#ifdef __cplusplus

struct _ENEMY_WORK;

/* The screen sprite: a half-size copy of the EFB (`tex_buf`) drawn back over the screen, either flat (`mode` 1/2 with the
 * tint) or as a zoom toward a tracked screen point (`mode` 3; `draw_sprite_track` follows an enemy joint or a world
 * position and eases `level`).  `draw_sprite_init` memsets the first 0x38 bytes. size: 0x38 */
typedef struct DrawSpriteSlot {
    /* +0x00 */ u8 enable;
    /* +0x01 */ u8 mode;            /* 1 additive, 2 subtractive, 3 zoom toward (center_x, center_y), else alpha */
    /* +0x02 */ u8 alpha;
    /* +0x03 */ u8 level;           /* the zoom strength (mode 3) / the flat tint level */
    /* +0x04 */ u8 blend;           /* `draw_sprite_blend_draw`: 0 off, 1..3 the TEV scale */
    /* +0x05 */ u8 blend_alpha;
    /* +0x06 */ u8 blend_level;
    /* +0x07 */ u8 pad_0x07;
    /* +0x08 */ f32 center_x;       /* the zoom centre in [-1, 1] screen units */
    /* +0x0C */ f32 center_y;
    /* +0x10 */ u8* tex_buf;        /* the captured EFB copy */
    /* +0x14 */ GXColor color;      /* the TEV colour (rgb) */
    /* +0x18 */ s32 timer;          /* ticks left in the current phase */
    /* +0x1C */ struct _ENEMY_WORK* enemy;  /* the tracked enemy (`use_pos` 0) */
    /* +0x20 */ u8 active;
    /* +0x21 */ u8 phase;           /* 0 start, 1 rise, 2 fall */
    /* +0x22 */ u8 pad_0x22[0x02];
    /* +0x24 */ u8 joint;           /* the tracked enemy joint */
    /* +0x25 */ u8 step;            /* the per-tick `level` step */
    /* +0x26 */ u8 use_pos;         /* 1: track `pos` instead of the enemy */
    /* +0x27 */ u8 area;            /* the stage area `pos` lies in */
    /* +0x28 */ nw4r::math::VEC3 pos;
    /* +0x34 */ s32 count;          /* frames drawn since `draw_sprite_start` */
} DrawSpriteSlot; /* size: 0x38 */

/* A view of `src/fn_80056F24.cpp`'s glare filter record (`GlareFilter`, private to that unit), as `glare_filter_init`
 * fills it. size: 0x3C */
typedef struct GlareFilterView {
    /* +0x00 */ u8 enable;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u8 color_r;
    /* +0x03 */ u8 color_g;
    /* +0x04 */ u8 color_b;
    /* +0x05 */ u8 pad_0x05[0x03];
    /* +0x08 */ u8* tex_buf;        /* the glare texture */
    /* +0x0C */ u32 tex_size;
    /* +0x10 */ u8 pad_0x10[0x0C];
    /* +0x1C */ u16 width;          /* half the screen */
    /* +0x1E */ u16 height;
    /* +0x20 */ u8 pad_0x20[0x08];
    /* +0x28 */ u32 dark_level;
    /* +0x2C */ u32 bright_level;
    /* +0x30 */ f32 dark_scale;
    /* +0x34 */ f32 bright_scale;
    /* +0x38 */ u8* capture_buf;    /* the draw state's per-frame EFB copy */
} GlareFilterView; /* size: 0x3C */

/* `lbl_8066ACF8` (0x8066ACF8, .bss, owned by `src/fn_80056F24.cpp` as `GameDrawState`): the draw state block as this unit
 * reads it - the screen sprite at +0x3C, the glare filter at +0x74 and the EFB copy pointer at +0x134. size: 0x14C */
typedef struct DrawShapeState {
    /* +0x000 */ u8 pad_0x00[0x3C];
    /* +0x03C */ DrawSpriteSlot sprite;
    /* +0x074 */ GlareFilterView glare;
    /* +0x0B0 */ u8 pad_0xB0[0x84];
    /* +0x134 */ u8* capture_buf;
    /* +0x138 */ u8 pad_0x138[0x14];
} DrawShapeState; /* size: 0x14C */

extern "C" {
void draw_sprite_init(DrawSpriteSlot* s);
void draw_sprite_draw(DrawSpriteSlot* s);
void sprite_gx_end(void);
void sprite_gx_texcoord2f32(f32 s, f32 t);
void sprite_gx_position2s16(s16 x, s16 y);
void sprite_gx_set_tex_coord_gen(u32 dst, u32 func, u32 src, u32 mtx);
void draw_sprite_blend_draw(DrawSpriteSlot* s);
void sprite_gx_position2u16(u16 x, u16 y);
void draw_sprite_set(u8 alpha, u8 mode, u32 level, f32 center_x, f32 center_y, const GXColor* color, s32 timer);
void draw_sprite_reset(void);
void draw_sprite_start(void);
void draw_sprite_stop(void);
void draw_shape_arm(struct _ENEMY_WORK* enemy, u32 joint, s32 timer);
void draw_shape_arm_pos(const nw4r::math::VEC3* pos, s32 timer, u8 area);
s32 draw_sprite_track(DrawSpriteSlot* s);
void draw_shape_state_clear(void);
void glare_filter_init(GlareFilterView* g);
void glare_filter_off(void);
}

#endif

#endif /* MHTRI_DRAW_SHAPE_ARM_H */
