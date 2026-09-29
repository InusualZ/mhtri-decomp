/*
 * Declarations owned by `Pl/bss_pool.cpp` (docs/plan.md 6.5 rule 2).  A consumer includes this
 * header instead of declaring one of the run's arrays itself.
 *
 * The unit is **data-only**: it owns the Pl band's shared `.bss` collision-work run,
 * `0x806AB848..0x806AC8A8` (0x1060 B, 12 arrays), and its source defines nothing.  For a
 * `NonMatching` unit the original bytes stay in the binary, so the DOL is untouched while the run
 * gains a single owner - which is what stops rule 12 firing for the run's consumers and what stops
 * dtk creating an anonymous `auto_*_bss` unit over the same bytes.
 *
 * The extent is measured, not chosen (`tools/units/callers.py`, one address at a time).  Every array
 * here is read only by the `Pl` ground/hit collision band - `Pl/fn_8028F66C.cpp`, `Pl/fn_80295EF4.cpp`,
 * `Pl/fn_8025F088.cpp` and `Pl/pl_act.cpp` - while the run's neighbours are reader-disjoint: `0x806AB83C`
 * before it is read only by `Pl/fn_80288CEC.cpp`, and `0x806AC8A8` - the seam - is the first address a
 * non-`Pl` unit touches (`menu/menu_item.cpp`).  So the run ends there and the lower half
 * (`0x806AC8A8..0x806AD698`, read by `menu/menu_item`, `menu/fn_8031EA8C`, `ai/fn_802D44F4`,
 * `enemy/em024_ai` and `lobby/fn_802076D4`) is a different owner's - see the unit header's residual.
 *
 * The construction is what fixes each array's shape: the band's static initializer `fn_80297C30`
 * (0x80297C30, inside `Pl/fn_80295EF4.cpp`) builds them all with `__construct_array`, and its element
 * ctor and stride are reproduced in each member's comment below.
 *
 * `pl_move_work` is declared two-dimensional (`[4][11]`) because that is the shape the motion layer's
 * own addressing spells - `pl_move_work[chunk_ofs][i]`, one chunk of 11 entries per `_PLW::chunk_ofs`
 * - and a flat `[44]` indexed `chunk_ofs * 11 + i` makes MWCC emit a different multiply sequence.
 *
 * The unit name is a **GUESS**: no `__FILE__` string and no runtime-dump name covers the range, so the
 * file keeps the name `tools/units/dataclaim.py` derives for the pool (`Pl/bss_pool.cpp`); a context
 * name would fit the collision band equally well and a later pass may rename it.
 *
 * GUESSES: the arrays no written body spells yet - `pl_coll_slot_free`, `pl_coll_closest_0/1/2`,
 * `pl_coll_slot_dist`, `pl_coll_slot_hit`, `pl_coll_slot_kind`, `pl_coll_slot_used`, `pl_hit_id_list` -
 * are named from the read sites recorded beside each of them; the shapes and strides are measured,
 * the semantic names are the best the call sites support.
 */
#ifndef MHTRI_PL_BSS_POOL_H
#define MHTRI_PL_BSS_POOL_H

#include "types.h"
#include "nw4r/math.h"

/* One entry of the per-chunk move-work table (`.bss` `pl_move_work`, 0x420 B of 24-byte entries,
 * 264 B - 11 entries - per `_PLW::chunk_ofs` chunk).  Only the fields the readers name are spelled;
 * `+0x14` is the byte `fn_8027D050` scans for (`Pl/pl_act.cpp`), and the rest is padding.
 * size: 0x18 */
struct PlMoveEntry {
    /* +0x00 */ u16 kind;        /* non-zero marks a live entry; bit 1 is the "armed" flag */
    /* +0x02 */ u8 pad_0x02[0x6];
    /* +0x08 */ f32 x_0x08;
    /* +0x0C */ u8 pad_0x0C[0x4];
    /* +0x10 */ f32 z_0x10;
    /* +0x14 */ u8 carve_0x14;   /* `fn_8027D050` returns the first live entry's byte here */
    /* +0x15 */ u8 pad_0x15[0x3];
};

/* One cell of the land table (`.bss` `pl_land_data`, 15 records of 0x88 bytes - the count and the
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
    /* +0x3C */ u32* cells_0x3C;       /* grid A: the word each cell holds */
    /* +0x40 */ u8 unused_0x40[0x48 - 0x40];
    /* +0x48 */ nw4r::math::VEC3 box_min_0x48;
    /* +0x54 */ nw4r::math::VEC3 box_max_0x54;
    /* +0x60 */ u8 unused_0x60[0x6C - 0x60];
    /* +0x6C */ s32 dim_z_0x6C;        /* grid B: third index; `fn_802963B0` bounds `z` by it */
    /* +0x70 */ s32 dim_y_0x70;        /* grid B: second index (the x stride) */
    /* +0x74 */ s32 dim_x_0x74;        /* grid B: first index */
    /* +0x78 */ u32* cells_0x78;       /* grid B: the word each cell holds */
    /* +0x7C */ u8 unused_0x7C[0x88 - 0x7C];
};

/* The 0x3C-byte box record (`pl_hit_box`, ten of them - the `__construct_array` call `fn_80297C30`
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

#ifdef __cplusplus
extern "C" {
#endif

/* The run, in address order.  Each comment is the construction `fn_80297C30` performs (or the read
 * site that fixes the shape) and the address the map row carried. */

extern PlMoveEntry pl_move_work[4][11];       /* 0x806AB848: 4 chunks x 11 x 0x18, element ctor
                                                  * `fn_80295544` - the shape is the readers' own
                                                  * `chunk_ofs * 264 + i * 24` addressing */
extern u8 pl_coll_slot_free[0x10];               /* 0x806ABC68: 16 free/used bytes - `fn_802929DC`
                                                  * releases a slot with `pl_coll_slot_free[i] = 0` */
extern PlHitBox pl_hit_box[0x258 / 0x3C];        /* 0x806ABC78: 10 x 0x3C, element ctor `fn_80297DE8` */
extern nw4r::math::VEC3 pl_coll_closest_0[0x78 / 0xC]; /* 0x806ABED0: 10 x VEC3, `VEC3_ctor` */
extern nw4r::math::VEC3 pl_coll_closest_1[0x78 / 0xC]; /* 0x806ABF48: 10 x VEC3, `VEC3_ctor` */
extern nw4r::math::VEC3 pl_coll_closest_2[0x78 / 0xC]; /* 0x806ABFC0: 10 x VEC3, `VEC3_ctor` */
extern f32 pl_coll_slot_dist[0x28 / 4];          /* 0x806AC038: 10 per-slot distances; cleared with the
                                                  * two flag arrays by `fn_80293A88`/`findInterSection3`,
                                                  * read one slot at a time by `fn_80294EFC` */
extern u8 pl_coll_slot_hit[0x0C];                /* 0x806AC060: 12 per-slot hit flags, cleared beside
                                                  * `pl_coll_slot_dist` (`findInterSection3`) */
extern u8 pl_coll_slot_kind[0x0C];               /* 0x806AC06C: 12 per-slot kind bytes, set by the
                                                  * `pl_coll_slot_*` writers of `fn_802924C0` */
extern u8 pl_coll_slot_used[0x10];               /* 0x806AC078: 16 per-slot liveness bytes (0/1),
                                                  * written beside `pl_coll_slot_kind` */
extern LandData pl_land_data[0x7F8 / 0x88];      /* 0x806AC088: 15 x 0x88, element ctor `fn_80297D9C` */
extern u32 pl_hit_id_list[0x28 / 4];             /* 0x806AC880: 10 ids, scanned by `fn_80296228` */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_BSS_POOL_H */
