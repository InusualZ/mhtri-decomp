/*
 * Pl/pl_coll.h - `Pl/pl_coll.cpp`'s `.bss` collision-work run 0x806AB848..0x806AC8A8 (0x1060 B, 12 arrays, declared,
 *   never defined) and the record types its readers share.  The readers are `Pl/pl_coll.cpp`, `Pl/pl_act_step.cpp`,
 *   `Pl/pl_act.cpp` and `menu/menu_item.cpp`; the static initialiser `fn_80297C30` builds the arrays with
 *   `__construct_array`, and each member's comment gives its element constructor and stride.
 * `pl_move_work` is `[4][11]`, the shape the motion layer's addressing spells (`pl_move_work[chunk_ofs][i]`): a flat
 *   `[44]` indexed `chunk_ofs * 11 + i` makes MWCC emit a different multiply sequence.
 * GUESSes, named from the read sites beside each (no written body spells them): `pl_coll_slot_free`,
 *   `pl_coll_closest_0/1/2`, `pl_coll_slot_dist`, `pl_coll_slot_hit`, `pl_coll_slot_kind`, `pl_coll_slot_used`,
 *   `pl_hit_id_list`.
 */
#ifndef MHTRI_PL_PL_COLL_H
#define MHTRI_PL_PL_COLL_H

#include "types.h"
#include "nw4r/math.h"

/* One entry of the per-chunk move-work table (`.bss` `pl_move_work`, 0x420 B of 24-byte entries,
 * 264 B - 11 entries - per `_PLW::chunk_ofs` chunk).  Only the fields the readers name are spelled;
 * `+0x14` is the byte `pl_step_se_kind_get` scans for (`Pl/pl_act.cpp`), and the rest is padding.
 * size: 0x18 */
struct PlMoveEntry {
    /* +0x00 */ u16 kind;        /* non-zero marks a live entry; bit 1 is the "armed" flag */
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ u32 angle;       /* the wall's yaw relative to the player's facing (`pl_coll_shape_hits`) */
    /* +0x08 */ union {          /* the position, read whole by the constructor and by x/z by the motion layer */
        /* +0x08 */ nw4r::math::VEC3 pos;
        struct { /* size: 0xC */
            /* +0x08 */ f32 x_0x08;
            /* +0x0C */ u8 pad_0x0C[0x4];
            /* +0x10 */ f32 z_0x10;
        };
    };
    /* +0x14 */ u8 carve_0x14;   /* `pl_step_se_kind_get` returns the first live entry's byte here */
    /* +0x15 */ u8 pad_0x15[0x3];
};

/* One 0x10-byte placed sphere: an `nw4r::math::VEC3` position plus the radius the overlap tests read at +0xC.  The
 * body-shape lists (`pl_coll_body_shape`) are runs of these ended by a radius of -1.  size: 0x10 */
struct HitSphere {
    /* +0x0 */ nw4r::math::VEC3 pos;
    /* +0xC */ f32 radius;
};

/* A plane: the unit normal and the distance from the origin along it (`pl_plane_project` subtracts `dist` from the
 * point's dot product with the normal). size: 0x10 */
struct PlPlane {
    /* +0x0 */ nw4r::math::VEC3 normal;
    /* +0xC */ f32 dist;
};

/* One collision polygon of the stage's land file: the cells of a `PlLandGrid` list pointers to these, ended by
 * 0xFFFFFFFF.  The ground query reads the attribute bits against its mask, the area byte against the stage's area
 * bit, the three vertices' x/z for the footprint test and the plane (`normal`, `dist`) for the height.  The wall
 * queries keep copies of the polygons they hit in `pl_hit_box` (`pl_hit_box_set` copies 0x3C bytes).
 * size: 0x3C */
struct PlCollPoly {
    /* +0x00 */ u16 attr;              /* surface attribute bits; 0 is promoted to 0x20 by the ground query */
    /* +0x02 */ u8 kind;               /* copied into the `LandData` and the ceiling record */
    /* +0x03 */ u8 field_0x03;         /* copied into the `LandData` */
    /* +0x04 */ u16 layer;             /* the layer bits a query's `layers` argument excludes */
    /* +0x06 */ u8 field_0x06;         /* copied into the `LandData` */
    /* +0x07 */ u8 area_mask;          /* areas the polygon is disabled in (`stage_area_bit_get`); the low nibble is
                                        * copied into the `LandData` */
    /* +0x08 */ nw4r::math::VEC3 vtx0;    /* the three corners, read as a run of three by the edge tests */
    /* +0x14 */ nw4r::math::VEC3 vtx1;
    /* +0x20 */ nw4r::math::VEC3 vtx2;
    /* +0x2C */ nw4r::math::VEC3 normal;
    /* +0x38 */ f32 dist;              /* the plane offset: `normal . p + dist == 0` on the plane */
};

/* The ceiling record a ground query fills when it meets a ceiling polygon (attribute bit 0x10): the hit flag, the
 * polygon's kind and the ceiling height.  size: 0x8 */
struct _HIT_TENJO_DATA {
    /* +0x0 */ u8 hit;
    /* +0x1 */ u8 kind;
    /* +0x2 */ u8 pad_0x2[0x2];
    /* +0x4 */ f32 height;
};

/* One collision grid of a land record (0x3C bytes, two per `PlLandCell`): the AABB, the cell size, the three
 * dimensions and the two relocated pointers `pl_land_grid_a_load`/`pl_land_grid_b_load` fill from the stage's land
 * file.  `cells` holds one word per cell (a pointer into `polys` after relocation); `fn_802969A8`/`fn_802969D0` index
 * it `[(x * dim_y + y) * dim_z + z]` and `fn_802963B0`/`fn_802963FC` range-check the triple.
 * size: 0x3C */
struct PlLandGrid {
    /* +0x00 */ u8 off_grid;           /* 1 when either corner of the box has a fractional coordinate */
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ nw4r::math::VEC3 min;  /* `fn_80296368` tests a point against `min`/`max` (x and z) */
    /* +0x10 */ nw4r::math::VEC3 max;
    /* +0x1C */ u32 cell_x;            /* the cell size along each axis (`pl_land_cell_index` divides by it) */
    /* +0x20 */ u32 cell_y;
    /* +0x24 */ u32 cell_z;
    /* +0x28 */ u32 dim_z;
    /* +0x2C */ u32 dim_y;
    /* +0x30 */ u32 dim_x;
    /* +0x34 */ u32* cells;            /* per cell, the address of its 0xFFFFFFFF-ended polygon list */
    /* +0x38 */ u32* polys;
};

/* One record of the land table (`.bss` `pl_land_data`, 15 records of 0x88 bytes - the count and the stride are
 * the `__construct_array` call `fn_80297C30` makes with `pl_land_cell_ctor` as the element constructor, which constructs
 * the two grids' corner vectors).  `loaded_b`/`loaded_a` are set by the two grid loaders.
 * size: 0x88 */
struct PlLandCell {
    /* +0x00 */ u8 loaded_b;           /* `pl_land_grid_b_load` sets it */
    /* +0x01 */ u8 loaded_a;           /* `pl_land_grid_a_load` sets it */
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ u32 angle;       /* the wall's yaw relative to the player's facing (`pl_coll_shape_hits`) */
    /* +0x08 */ PlLandGrid grid_a;
    /* +0x44 */ PlLandGrid grid_b;
    /* +0x80 */ u8 unused_0x80[0x88 - 0x80];
};

#ifdef __cplusplus
extern "C" {
#endif

/* The run, in address order: each comment is the construction `fn_80297C30` performs or the read site that
 * fixes the shape. */

extern PlMoveEntry pl_move_work[4][11];       /* 0x806AB848: 4 chunks x 11 x 0x18, element ctor
                                                  * `fn_80295544` - the shape is the readers' own
                                                  * `chunk_ofs * 264 + i * 24` addressing */
extern s8 pl_coll_slot_free[0x10];               /* 0x806ABC68: 16 free/used bytes - `fn_802929DC`
                                                  * releases a slot with `pl_coll_slot_free[i] = 0` */
extern PlCollPoly pl_hit_box[0x258 / 0x3C];     /* 0x806ABC78: the wall polygons hit, 10 x 0x3C, element ctor
                                                  * `pl_hit_box_ctor` */
extern nw4r::math::VEC3 pl_coll_closest_0[0x78 / 0xC]; /* 0x806ABED0: 10 x VEC3, `VEC3_ctor` */
extern nw4r::math::VEC3 pl_coll_closest_1[0x78 / 0xC]; /* 0x806ABF48: 10 x VEC3, `VEC3_ctor` */
extern nw4r::math::VEC3 pl_coll_closest_2[0x78 / 0xC]; /* 0x806ABFC0: 10 x VEC3, `VEC3_ctor` */
extern f32 pl_coll_slot_dist[0x28 / 4];          /* 0x806AC038: 10 per-slot distances; cleared with the
                                                  * two flag arrays by `coll_line_check`/`findInterSection3`,
                                                  * read one slot at a time by `fn_80294EFC` */
extern u8 pl_coll_slot_hit[0x0C];                /* 0x806AC060: 12 per-slot hit flags, cleared beside
                                                  * `pl_coll_slot_dist` (`findInterSection3`) */
extern s8 pl_coll_slot_kind[0x0C];               /* 0x806AC06C: 12 per-slot kind bytes, set by the
                                                  * `pl_coll_slot_*` writers of `fn_802924C0` */
extern s8 pl_coll_slot_used[0x10];               /* 0x806AC078: 16 per-slot liveness bytes (0/1),
                                                  * written beside `pl_coll_slot_kind` */
extern PlLandCell pl_land_data[0x7F8 / 0x88];      /* 0x806AC088: 15 x 0x88, element ctor `pl_land_cell_ctor` */
extern PlCollPoly* pl_hit_id_list[0x28 / 4];     /* 0x806AC880: the polygons the ground query has taken,
                                                  * scanned by `pl_hit_id_listed` */

/* The wall queries' slot counters (`.sbss`): the hit boxes filled so far and the free-slot cursor of the resolver. */
extern s8 pl_coll_hit_count;                     /* 0x80794B58 */
extern s8 pl_coll_free_count;                    /* 0x80794B59 */

/* The wall resolver's shared-corner table (`.sdata`): indexed by the corner bits two hit boxes share, the corner of
 * the second box to test (0-2), or 0xFF for none. */
extern u8 pl_coll_shared_vtx_table[8];          /* 0x80792270 */

/* The player's body shape for the wall queries (`.data`): spheres along the body, ended by a radius of -1
 * (`pl_coll_shape_set` walks it; the static initialiser fills the centres).  The second list is built the same way
 * and read by nothing in this unit.  GUESS names. */
extern HitSphere pl_coll_body_shape[2];         /* 0x805CDC48 */
extern HitSphere pl_coll_body_shape_alt[2];     /* 0x805CDC68 */

/* 0x80293A7C - scales `v` by the coll unit's constant over `len`.  GUESS name. */
void vec3_scale_inv(VEC3* v, f32 len);

/* 0x80290524 - projects `point` onto `plane` into `out`. */
void pl_plane_project(nw4r::math::VEC3* point, PlPlane* plane, nw4r::math::VEC3* out);

#ifdef __cplusplus
}

/* 0x80295924 - the segment test `from` -> `to` against the stage collision of `area` for the hit mask; 0 when nothing
 * is in the way (`enemy/em_common.cpp`'s meat search).  The parameter names beyond the three vectors are GUESSES. */
s32 findInterSection(nw4r::math::VEC3* from, nw4r::math::VEC3* to, nw4r::math::VEC3* out, u8 mode, u32 mask, u8 area,
                     u16 hit_mask, u8* out_kind);

/* 0x8029595C - the same query with a `LandData` record (`enemy/em_common.cpp`'s area-edge probe). */
s32 findInterSection2(nw4r::math::VEC3* from, nw4r::math::VEC3* to, nw4r::math::VEC3* out, u8 group,
                      u32 ground, u8 area_no, u16 angle, u8* out_flags, struct LandData* land);
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* 0x8029708C - resets the player collision work (the system core's move-work reset). */
void pl_coll_table_clear(void);

/* The body push tests `Pl/pl_body_push.cpp` calls: 1 when the two shapes overlap, with the push that separates them. */
struct PlCapsuleBox;
u32 pl_sphere_push(HitSphere* a, HitSphere* b, nw4r::math::VEC3* out);
u32 pl_capsule_capsule_push(struct PlCapsuleBox* a, struct PlCapsuleBox* b, nw4r::math::VEC3* push_a,
                            nw4r::math::VEC3* push_b);
u32 pl_capsule_sphere_push(struct PlCapsuleBox* a, HitSphere* sphere, nw4r::math::VEC3* push_a,
                           nw4r::math::VEC3* push_sphere);

/* The hit contact tests `Pl/pl_hit.cpp` calls: 1 when the two shapes touch, with the contact point in `out`. */
s32 pl_sphere_contact(HitSphere* a, HitSphere* b, nw4r::math::VEC3* out);
s32 pl_capsule_capsule_contact(struct PlCapsuleBox* a, struct PlCapsuleBox* b, nw4r::math::VEC3* out);
s32 pl_capsule_sphere_contact(struct PlCapsuleBox* a, HitSphere* sphere, nw4r::math::VEC3* out);
s32 pl_sphere_capsule_contact(HitSphere* sphere, struct PlCapsuleBox* a, nw4r::math::VEC3* out);
#ifdef __cplusplus
}
#endif

/* 0x80291664 - the ground height under `pos`; `hit` receives the hit flag (C++ linkage: the map's mangling). */
f32 GetGroundHit2(nw4r::math::VEC3* pos, u32 ground, u8 flag, u8* hit);

/* 0x8029163C - the ground height under `pos` without the hit flag (C++ linkage: the map's mangling). */
f32 GetGroundHit(nw4r::math::VEC3* pos, u32 ground, u8 flag);

#endif /* MHTRI_PL_PL_COLL_H */
