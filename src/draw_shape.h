/* The declarations `src/draw_shape.cpp` owns (docs/plan.md 6.5 rule 2): the draw-shape state block's
 * setters.  Created when `enemy/fn_80147CE0.cpp` registered as a consumer - the owner had no header.
 * The signature is the owner's own definition (`extern "C" void draw_shape_arm(u32 a, u32 b, u32 c)`,
 * which stores r3 and r5 as words and `(u8)r4`), so a caller with a pointer first argument casts it.
 */
#ifndef MHTRI_DRAW_SHAPE_H
#define MHTRI_DRAW_SHAPE_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80055E30 - selects the resource arena index the loaders draw from (and clears the switch byte);
 * `arena_resource_load` picks index 1 for the arena texture pack.  `res_file_ctor` (0x80054FE8) builds a
 * `ResFile` handle from a 32-byte-aligned file address (asserting the alignment) and `res_file_assign`
 * (0x80054FAC) copies one handle over another and returns the destination.  Added with
 * `quest/arenatask.cpp` (rule 2: this unit owns the addresses). */
void set_arena_idx(u8 index);
u32* res_file_ctor(u32* dst, u32 file_data);
u32* res_file_assign(u32* dst, u32* src);
/* 0x80056A54 - arm the draw-shape state block (`lbl_8066ACF8`). */
void draw_shape_arm(u32 a, u32 b, u32 c);
/* 0x80056A84 - the effect step's shape request: the position, the handle and the area byte.  Added
 * with `ef/eft053.cpp`, whose state machine fires it (`fn_80056A84(&pos, 10, area)`). */
void fn_80056A84(const nw4r::math::VEC3* pos, u32 handle, u8 flag);

/* 0x800553B4 - loads the stage's shape textures (GUESS name). */
void draw_shape_stage_load(u8 stage);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x80054E04 / 0x80055054 / 0x80055204 - load the item-icon, frame and menu texture packs (C++ scope: the map rows
 * are `itemicon_tex_load__Fv`, `gpframe_tex_load__Fv`, `menu_tex_load__Fv`). */
void itemicon_tex_load(void);
void gpframe_tex_load(void);
void menu_tex_load(void);
#endif

#endif /* MHTRI_DRAW_SHAPE_H */
