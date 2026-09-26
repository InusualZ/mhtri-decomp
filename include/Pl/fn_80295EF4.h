/*
 * The types and the unowned-symbol declarations `src/Pl/fn_80295EF4.cpp` needs (docs/plan.md 6.5
 * rules 3/4).  The two records below are shared with the neighbouring Pl bands (the land table
 * `lbl_806AC088` and the hit registry are walked by `Pl/fn_8028F66C.cpp`'s range as well), so this
 * header is where they live until the band's own owner header exists - see the unit's
 * `config_requests` entry.
 */
#ifndef MHTRI_PL_FN_80295EF4_H
#define MHTRI_PL_FN_80295EF4_H

#include "types.h"
#include "nw4r/math.h"

/* One entry of the global hit registry's linked lists (the map's `_HIT_W`, spelled by
 * `hit_flag_set__FP6_HIT_WUl` / `hit_result_check__FP6_HIT_W`): the record a live attack is
 * registered as.  `+0x00` is the list link (`fn_8029EFDC`/`fn_8029F00C` push the record onto one of
 * the registry's two heads and write the previous head here), `+0x05` is tested for zero before the
 * push, `+0x06`/`+0x07` select the owner's type twice over and `+0x10`/`+0x14` are the two owner
 * pointers - `fn_8029F204` stores the hit id at `+0x0C` and reads the owner's area byte through
 * `+0x10`.
 * size: 0x60 (lower bound - the highest offset this unit touches is +0x5B) */
struct _HIT_W {
    /* +0x00 */ _HIT_W* next_0x00;
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 active_0x05;
    /* +0x06 */ u8 owner_kind_0x06;   /* 0 = player, 1 = enemy, 2 = ?, 3 = AI NPC (`fn_80299EF8`) */
    /* +0x07 */ u8 kind_0x07;
    /* +0x08 */ u8 unused_0x08[0x0C - 0x08];
    /* +0x0C */ u16 hit_id_0x0C;
    /* +0x0E */ u8 unused_0x0E[0x10 - 0x0E];
    /* +0x10 */ void* owner_0x10;
    /* +0x14 */ void* owner_0x14;
    /* +0x18 */ u8 unused_0x18[0x50 - 0x18];
    /* +0x50 */ f32 value_0x50;      /* `fn_8029F204` stores `-100.0f` or a lowered byte here */
    /* +0x54 */ f32 value_0x54;      /* the same for the second owner slot */
    /* +0x58 */ u8 unused_0x58[0x60 - 0x58];
};

/* The hit registry itself (`.bss` `lbl_806AC8A8`, size 0x10): two singly-linked lists of live hits,
 * their counts and the running id `get_hit_id()` hands out.
 * size: 0x10 */
struct HitRegistry {
    /* +0x00 */ _HIT_W* head_0x00;
    /* +0x04 */ _HIT_W* head_0x04;
    /* +0x08 */ u16 count_0x08;
    /* +0x0A */ u16 count_0x0A;
    /* +0x0C */ u16 next_id_0x0C;   /* `get_hit_id` increments it and wraps at 0xEA60 */
    /* +0x0E */ u16 unused_0x0E;
};

/* One cell of the land table (`.bss` `lbl_806AC088`, 15 records of 0x88 bytes - the count and the
 * stride are the `__construct_array` call `fn_80297C30` makes with `fn_80297D9C` as the element
 * constructor).  The four 0xC-byte vectors are the elements that constructor zeroes; `box_min_0x48`
 * /`box_max_0x54` are the AABB `fn_80296368` tests a point against (its lower bounds are read at
 * +0x48/+0x50 and its upper bounds at +0x54/+0x5C); the two accessor pairs `dim0_0x34`..`cells_0x3C`
 * and `dim0_0x74`..`cells_0x78` are the 3-D grids `fn_802969A8`/`fn_802969D0` index and
 * `fn_802963B0`/`fn_802963FC` range-check - the names are the index each dimension bounds, read out
 * of the index arithmetic, not a guess.
 * size: 0x88 */
struct LandData {
    /* +0x00 */ u8 unused_0x00[0x0C];
    /* +0x0C */ nw4r::math::VEC3 vec_0x0C;
    /* +0x18 */ nw4r::math::VEC3 vec_0x18;
    /* +0x24 */ u8 unused_0x24[0x30 - 0x24];
    /* +0x30 */ s32 dim_z_0x30;        /* grid A: third index; `fn_802963FC` bounds `z` by it */
    /* +0x34 */ s32 dim_y_0x34;        /* grid A: second index (the x stride) */
    /* +0x38 */ s32 dim_x_0x38;        /* grid A: first index */
    /* +0x3C */ u32* cells_0x3C;      /* grid A: the word each cell holds */
    /* +0x40 */ u8 unused_0x40[0x48 - 0x40];
    /* +0x48 */ nw4r::math::VEC3 box_min_0x48;
    /* +0x54 */ nw4r::math::VEC3 box_max_0x54;
    /* +0x60 */ u8 unused_0x60[0x6C - 0x60];
    /* +0x6C */ s32 dim_z_0x6C;        /* grid B: third index; `fn_802963B0` bounds `z` by it */
    /* +0x70 */ s32 dim_y_0x70;        /* grid B: second index (the x stride) */
    /* +0x74 */ s32 dim_x_0x74;        /* grid B: first index */
    /* +0x78 */ u32* cells_0x78;      /* grid B: the word each cell holds */
    /* +0x7C */ u8 unused_0x7C[0x88 - 0x7C];
};

/* The 0x3C-byte box record `lbl_806ABC78` holds ten of (the `__construct_array` call `fn_80297C30`
 * makes with `fn_80297DE8` as the element constructor): four 0xC-byte vectors behind an 8-byte
 * header, plus a trailing word the constructor does not touch.
 * size: 0x3C */
struct PlHitBox {
    /* +0x00 */ u8 unused_0x00[0x08];
    /* +0x08 */ nw4r::math::VEC3 vec_0x08;
    /* +0x14 */ nw4r::math::VEC3 vec_0x14;
    /* +0x20 */ nw4r::math::VEC3 vec_0x20;
    /* +0x2C */ nw4r::math::VEC3 vec_0x2C;
    /* +0x38 */ u8 unused_0x38[0x3C - 0x38];
};

/* The 0x14-byte position record `fn_80297BE4` copies, `fn_802977E4` initialises and the joint
 * followers write through `fn_8029971C`/`fn_80299780`.  `fn_802977E4` zeroes the eight header bytes
 * one at a time and then sets the vector, and `fn_80297BE4` copies exactly those fields, which is
 * what fixes the layout.
 * size: 0x14 */
struct PlHitPoint {
    /* +0x00 */ u16 field_0x00;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u16 field_0x04;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ nw4r::math::VEC3 pos_0x08;
};

#ifdef __cplusplus
extern "C" {
#endif

/* The three globals this unit's functions operate on; their sizes are the map's `size:` fields
 * (`lbl_806AC880` 0x28, `lbl_806AC088` 0x7F8, `lbl_806AC8A8` 0x10 - the last one through the
 * `HitRegistry` type above).  All three sit in `.bss`, which this unit does not claim. */
extern u32 lbl_806AC880[0x28 / 4];
/* The 0x25-byte tile-id table `fn_8029B8F4` indexes (`.data`; unowned, so declared here). */
extern u8 lbl_805CDC88[];
extern LandData lbl_806AC088[0x7F8 / 0x88];
extern HitRegistry lbl_806AC8A8;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_FN_80295EF4_H */
