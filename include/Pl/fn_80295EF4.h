/*
 * `Pl/fn_80295EF4.cpp`'s hit-registry record and its remaining unowned declaration (docs/plan.md 6.5
 * rules 3/4).  The array types this band reads are owned by `Pl/bss_pool.cpp`'s header
 * `Pl/bss_pool.h`, which `src/Pl/fn_80295EF4.cpp` includes (rule 2).
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

/* The land record (`.bss` `pl_land_data`, 15 x 0x88) and the 0x3C-byte hit-box record (`.bss`
 * `pl_hit_box`, 10 of them) are declared by their owner's header `Pl/bss_pool.h`, which
 * `src/Pl/fn_80295EF4.cpp` includes (rule 2); the layouts live there. */

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

/* The remaining globals this unit's functions operate on: the hit registry (`.bss` `lbl_806AC8A8`,
 * 0x10, through the `HitRegistry` type above) and the 0x25-byte tile-id table `lbl_805CDC88`
 * (`.data`).  Both are unowned, so they are declared here; the land table `pl_land_data` and the
 * hit-id list `pl_hit_id_list` moved to their owner's header `Pl/bss_pool.h` (rule 2). */
/* The 0x25-byte tile-id table `fn_8029B8F4` indexes (`.data`; unowned, so declared here). */
extern u8 lbl_805CDC88[];
extern HitRegistry lbl_806AC8A8;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_FN_80295EF4_H */
