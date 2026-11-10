/*
 * Pl/pl_coll.cpp - the player/enemy collision band: the stage land grids (ground, wall, line and sweep queries over
 *   the polygons of `pl_land_data`), the wall resolver and the body-shape sweeps that feed the move work.  The shape
 *   tests ahead of it (sphere, capsule, segment, triangle) are `Pl/pl_hit_sphere.cpp`'s.
 * RANGE. .text 0x8029135C-0x80297E34 (60 functions, in address order below); .ctors 0x8056F370, .data
 *   0x805CDC48-0x805CDC88, .bss 0x806AB848-0x806AC8A8 (`Pl/pl_coll.h`'s arrays), .sdata 0x80792270-0x80792278,
 *   .sbss 0x80794B58-0x80794B60, .sdata2 0x8079A330-0x8079A380, extab 0x800132DC-0x80013454, extabindex
 *   0x80030AC8-0x80030CFC.
 * SEAM. The band's `.sdata2` holds 0.5/0.001/1/1e-5/-1 twice, and MWCC dedupes a TU's float literals, so the band is
 *   two TUs: the pool at 0x8079A310 is read only by 0x8028F44C..0x8029135C (`Pl/pl_hit_sphere.cpp`), the pool from
 *   0x8079A330 (0, 50, 1, -0, 0.5, 0.001, ... the 0x4330000000000000 double) only by `GetGroundHit2` (0x80291664)
 *   on.  The two grid loaders at 0x8029135C read no pool and fill `pl_land_data`, so they open this unit; every data
 *   section of the band has its first referrer at or after them.
 * FLAGS. The `Pl` library's settled set (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on -fp_contract off`).
 * NAMES. `GetGroundHit`, `GetGroundHit2`, `hit_ground_comon` and `findInterSection*` are the map's (mangled) names;
 *   every other function is a C-linkage name derived from its body.  GUESS (from each body and its callers): every
 *   `pl_*`/`em_coll_*` function name, `vec3_scale_inv`, `vec3_scale_to`, `coll_wall_slide`, `coll_line_check`,
 *   `npc_ground_check`, the data names `pl_coll_hit_count`, `pl_coll_free_count`, `pl_coll_shared_vtx_table`,
 *   `pl_coll_body_shape(_alt)`, and the registry names `floorf_thunk`, `sqrtf_thunk`, `stage_area_bit_get`,
 *   `calc_xz_angle`; the GUESS class `PlCollSphere` (0x8029573C's own constructor), the rows `pl_land_data_reset`,
 *   `pl_ground_check_all_layers`.
 * RESIDUALS.
 *  - `GetGroundHit2`, `hit_ground_comon`: the five-hit cap leaves both loops with one branch in retail (`goto`-shaped);
 *    the conformant double `break` costs three instructions in each (100 % with a `goto`, refused by rule 8). An
 *    inline helper whose `return` leaves both loops measures worse (`blt`+`b` for retail's `bge`, and the count's copy).
 *  - `pl_wall_resolve` (0x802929DC): register colouring, named by the compiler debugger (the Wii/1.3 stand-in compiles
 *    this unit byte-identically; the final dump MATCHes 711/711 instructions): the `regalloc` pass (dump 24) colours
 *    `work` (node r32) and `flag` (r34) fifth and fourth, to r27/r28, and `pos` to r14, where retail spills `pos` to
 *    0x8(r1) at entry (`stw r4,0x8(r1)` against our `mr r14,r4`, reloaded `lwz r3,0x8(r1)`) and colours `flag`/`work`
 *    last, to r14/r15; the shared-corner loop's constant order follows the colouring.  An unused `&pos` changes nothing.
 *  - `pl_coll_shape_hits`: retail keeps `count * 0x18` across the floor-kind `if` and recomputes it for `count++`;
 *    ours does the opposite (`opt_common_subs off` -63 B, `count++` split out -16 B).
 *  - `pl_wall_slot_push`: retail reuses the edge branch's `slot * 0xC` for `pl_coll_closest_0[slot]`; ours recomputes.
 *  - register colouring only (declaration orders hill-climbed, no gain): `coll_wall_slide`, `coll_line_check`,
 *    `pl_coll_sweep_ck`, `findInterSection3`, `em_coll_shape_hits`.
 * SHAPES. Locals are declared in retail's construction order: a box or plane right before its first use, a slide
 *   work after the vectors; `PlSlideWork::ends` is an array (its constructor loops), `PlBodySegment` has no
 *   constructor of its own (the implicit one constructs its two ends).  `pl_hit_box_set`/`pl_land_copy` copy through
 *   the plain images `PlCollPolyImage`/`LandDataImage` (retail's trivial record copy).  `#pragma pool_data off`
 *   over the definitions at the foot of the file makes the `__sinit` load each object's address.
 *  - Data: `.data`, `.sdata` and `.sbss` are defined here; the `.bss` arrays stay declared in `Pl/pl_coll.h` (defined
 *    here, MWCC addresses them from one section base where retail relocates each by name: 4 rows dropped to 61/81);
 *    `.ctors` is missing
 *    (the initialiser above).
 *  - Consumer declarations that disagree with the definitions (C linkage, so the code is unaffected): `coll_wall_slide`
 *    (`Pl/coll_wall_slide.h`, `ai/fn_802D0F34.h`: no radius), `pl_ground_probe` (`Pl/pl_segment_sphere_hit.h`: seven
 *    arguments), `npc_ground_check`, `pl_coll_shape_set`.
 * SHAPES. `pl_tri2d_cross` keeps its two products in a local array (retail stores and reloads them around the
 *   one-ulp test); the grid cell index is `index = (x * dim_y + y) * dim_z` then `cells[index + z]` (the last `add`
 *   keeps the product first); the shape-hit yaw is the call's raw result narrowed in a second statement.
 */

/* ==== 0x8029135C-0x80295EF4: the collision queries ==== */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "pl.h"              /* the Pl module header */
#include "fn_8004CAD8.h"     /* the vector helpers this unit calls (rule 2) */
#include "ef/fn_800AEE48.h"  /* `vec3_sub_assign` (rule 2) */
#include "Pl/fn_80288CEC.h"
#include "Pl/fn_8028F66C.h"  /* `LandData` and the record initialisers this unit calls (rule 2) */
#include "Pl/pl_hit_sphere.h" /* the capsule boxes and `hit_point_sphr` (rule 2) */
#include "ef/effect.h"    /* `CAPSULE::CAPSULE()` (rule 2) */
#include "Runtime.PPCEABI.H/memset.h"
#include "Runtime.PPCEABI.H/CPlusLibPPC.h" /* `__construct_array` (rule 2) */
#include "ef/fn_800CDB2C.h"   /* PlayMode_ck (rule 2: the owner is `ef/system_core.cpp`) */
#include "menu/hit_attack_list_push.h"
#include "Pl/pl_coll.h"   /* this unit's `.bss` arrays and record types */
#include "stage/stg_w.h" /* `get_now_areano` (rule 2) */
#include "stage/stage_area_bit_get.h"
#include "g3d/floorf_thunk.h"
#include "sqrtf_thunk.h"
#include "ef/ef_vec3_dist_sq.h"
#include "g3d/g3d_calcworld.h" /* `VEC3::operator+=` (rule 2) */
#include "g3d/g3d_draw.h"      /* `vec3_negate` (rule 2) */
#include "calc_xz_angle.h"
#include "enemy/fn_801251D0.h" /* `get_enemy_data` (rule 2) */
#include "enemy/EnemyData.h"
#include "enemy/fn_8012EC74.h" /* `get_em_chg_scale` (rule 2) */
#include "enemy/ENEMY_WORK.h" /* the enemy work `pl_ground_probe` reads the area of */




/* The push bounds `pl_push_bound_add` accumulates: per axis, the largest push each way and the excess over the
 * limit.  size: 0x30 */
struct PlPushBound {
    /* +0x00 */ nw4r::math::VEC3 pos;
    /* +0x0C */ nw4r::math::VEC3 neg;
    /* +0x18 */ nw4r::math::VEC3 pos_rest;
    /* +0x24 */ nw4r::math::VEC3 neg_rest;
};

/* The swept segment a wall query works on: the two ends, the radius around it, its length and the mode flags the
 * caller picked (`coll_wall_slide`'s `mode`: 0 neither, 1 both, 2 `slide` only).  Its constructor constructs
 * the two ends.  size: 0x24 */
struct PlSlideWork {
    /* +0x00 */ nw4r::math::VEC3 ends[2];   /* [0] the start, [1] the end of the move */
    /* +0x18 */ f32 radius;
    /* +0x1C */ f32 length;
    /* +0x20 */ u16 slide;              /* the resolver lets the move slide along the walls */
    /* +0x22 */ u16 skip_below;         /* polygons wholly below the query point are ignored */

    /* 0x80292468 (0x58): constructs the two end points (emitted after its first user, `coll_wall_slide`). */
    PlSlideWork() __attribute__((never_inline)) {}
};

/* The body segment the shape setters sweep: two end points with no constructor of their own (the implicit one
 * constructs them member by member).  size: 0x18 */
struct PlBodySegment {
    /* +0x00 */ nw4r::math::VEC3 start;
    /* +0x0C */ nw4r::math::VEC3 end;
};

/* One collision grid as the stage's land file stores it: the box, the cell size, the dimensions and the byte offsets
 * of the cell and polygon tables from this header (`pl_land_grid_a_load`/`pl_land_grid_b_load` read it).
 * size: 0x3C */
struct PlLandFileGrid {
    /* +0x00 */ nw4r::math::VEC3 min;
    /* +0x0C */ nw4r::math::VEC3 max;
    /* +0x18 */ u32 cell_x;
    /* +0x1C */ u32 cell_y;
    /* +0x20 */ u32 cell_z;
    /* +0x24 */ s32 dim_x;
    /* +0x28 */ s32 dim_y;
    /* +0x2C */ s32 dim_z;
    /* +0x30 */ u8 unused_0x30[0x4];
    /* +0x34 */ u32 cells_ofs;
    /* +0x38 */ u32 polys_ofs;
};

/* The stage's land file as the grid loaders read it: an 8-byte header, then the grid.  size: 0x44 (lower bound) */
struct PlLandFile {
    /* +0x00 */ u8 unused_0x00[0x8];
    /* +0x08 */ PlLandFileGrid grid;
};

/* The three functions this unit exports with a real (mangled) name that are declared here; the
 * parameters are the ones the manglings spell, so the front-end reproduces the map's names (rule 9). */
f32 GetGroundHit2(nw4r::math::VEC3* pos, u32 ground, u8 flag, u8* hit);
s32 hit_ground_comon(nw4r::math::VEC3* pos, u8 ground, LandData* land, f32* out, u32 kind,
                     u16 layers, f32* surf, f32* extra, _HIT_TENJO_DATA* tenjo);
s32 findInterSection3(nw4r::math::VEC3* from, nw4r::math::VEC3* to, nw4r::math::VEC3* out, u8 flags, u32 kind, u8 area,
                      u16 layers, u8* grid_hit, LandData* land, u8 max);

/* Every bare `fn_XXXXXXXX` is the map's own (unmangled) name, so the definitions carry C linkage. */
extern "C" {
void pl_land_grid_b_load(PlLandFile* file, s32 index);
void pl_land_grid_a_load(PlLandFile* file, s32 index);
f32 pl_tri2d_cross(VEC3* a, VEC3* b, VEC3* c);
u8 pl_tri2d_point_ck(VEC3* a, VEC3* b, VEC3* c, VEC3* point, f32 cross);
u32 pl_tri_normal(VEC3* a, VEC3* b, VEC3* c, VEC3* out);
u32 pl_tri_point_inside(VEC3* point, VEC3* tri, VEC3* normal);
void pl_vec3_snap_zero(VEC3* out, VEC3* in);
void pl_push_bound_add(VEC3* move, PlPushBound* bound, f32 limit);
u32 pl_coll_poly_usable(PlCollPoly* poly, u32* list, s32 list_count, u32 kind, u16 layers, s32 taken_count, u8 area);
u32 pl_id_list_find(u32 key, u32* list, s32 count);
void pl_coll_shape_hits(PlBodySegment* seg, f32 radius, u32 kind, u16 layers, _PLW* self);
void em_coll_shape_hits(PlBodySegment* seg, f32 radius, u32 kind, u16 layers, _ENEMY_WORK* self);
void pl_land_copy(LandData* dst, LandData* src);
s32 pl_land_grid_a_clip(VEC3* out, VEC3* from, VEC3* to, PlLandCell* land);
void pl_land_grid_a_span(VEC3* from, VEC3* to, s32* count_x, s32* count_y, s32* count_z, PlLandCell* land, s32* start_x,
                         s32* start_y, s32* start_z);
void pl_land_grid_b_span(VEC3* from, VEC3* to, s32* count_x, s32* count_y, s32* count_z, PlLandCell* land, s32* start_x,
                         s32* start_y, s32* start_z);
void pl_hit_box_set(PlCollPoly* dst, PlCollPoly* src);
s32 pl_wall_poly_hit(PlSlideWork* work, PlCollPoly* poly, VEC3* pos);
s32 pl_wall_resolve(PlSlideWork* work, VEC3* pos, u8* flag);
s8 pl_wall_slot_push(PlSlideWork* work, VEC3* push, s8 slot, u8* flag);
s32 pl_line_poly_hit(PlSlideWork* work, PlCollPoly* poly, u8 collect);
s32 pl_sweep_poly_hit(PlSlideWork* work, PlCollPoly* poly);
s32 pl_line_hit_nearest(PlSlideWork* work, VEC3* hit);
s32 pl_inter_poly_hit(VEC3* to, VEC3* from, PlCollPoly* poly, u8 grid);
u32 pl_vec3_fraction_ck(VEC3* v);
u32 pl_land_box_ck(VEC3* pos, PlLandCell* land);
s32 pl_land_grid_b_range_ck(s32 x, s32 y, s32 z, PlLandCell* land);
s32 pl_land_grid_a_range_ck(s32 x, s32 y, s32 z, PlLandCell* land);
void pl_land_cell_index(VEC3* pos, PlLandCell* land, u32* grid_x, u32* grid_z);
PlCollPoly** pl_land_grid_b_cell(s32 x, s32 y, s32 z, PlLandCell* land);
PlCollPoly** pl_land_grid_a_cell(s32 x, s32 y, s32 z, PlLandCell* land);
u32 pl_tri2d_point_in(VEC3* tri, VEC3* point);
u32 pl_hit_id_listed(PlCollPoly* poly, s32 count);
s32 pl_ground_check(_PLW* self, VEC3* pos, LandData* land, f32* out, u32 kind);
s32 pl_ground_probe(_ENEMY_WORK* self, VEC3* pos, LandData* land, f32* out, f32* surface, f32* extra,
                    _HIT_TENJO_DATA* tenjo, u32 kind);
s32 npc_ground_check(_PLW* self, VEC3* pos, LandData* land, f32* out, u32 kind);
void pl_ground_select(LandData* land, VEC3* pos, LandData* lands, f32* heights, f32* out, s8 count);
s32 pl_ground_check_all_layers(_PLW* self, VEC3* pos, LandData* land, f32* out, u32 kind);
void pl_land_data_reset(LandData* land);
s32 stage_water_surface_get(VEC3* pos, u8 flag, u16* hit_layer, f32* out, u32 kind);
void pl_area_ground_probe(VEC3* pos, u8 flag, LandData* land, u32 kind);
void vec3_scale_inv(VEC3* v, f32 divisor);
s32 coll_wall_slide(VEC3* to, VEC3* from, f32 radius, VEC3* pos, u32 kind, u16 layers, u8 mode, u8 area);
s32 coll_line_check(VEC3* to, VEC3* from, f32 radius, VEC3* hit, u8 flags, u32 kind, u8 area, u16 layers);
void pl_coll_shape_set(_PLW* self, u8 mode, u16 layers);
void em_coll_shape_set(_ENEMY_WORK* self, u8 mode, u16 layers);
}

s8 pl_coll_hit_count;
s8 pl_coll_free_count;

/* ------------------------------------------------------------------------------------------------ *
 * Bodies, in address order.
 * ------------------------------------------------------------------------------------------------ */

/* 0x8029135C (0x170): loads grid B of land record `index` from the land file: copies the box, the cell size and the
 * dimensions, and relocates the cell and polygon offsets against the file's grid header. */
void pl_land_grid_b_load(PlLandFile* file, s32 index) {
    PlLandFileGrid* src;
    PlLandCell* cell;
    u32 x;
    u32 z;
    u32 y;
    u32 word;
    u32* poly;

    cell = &pl_land_data[index];
    src = &file->grid;
    cell->loaded_b = 1;
    cell->grid_b.min = src->min;
    cell->grid_b.max = src->max;
    if (pl_vec3_fraction_ck(&cell->grid_b.min) == 0 && pl_vec3_fraction_ck(&cell->grid_b.max) == 0) {
        cell->grid_b.off_grid = 0;
    } else {
        cell->grid_b.off_grid = 1;
    }
    cell->grid_b.cell_x = src->cell_x;
    cell->grid_b.cell_y = src->cell_y;
    cell->grid_b.cell_z = src->cell_z;
    cell->grid_b.dim_x = src->dim_x;
    cell->grid_b.dim_y = src->dim_y;
    cell->grid_b.dim_z = src->dim_z;
    cell->grid_b.cells = (u32*)((u32)src + src->cells_ofs);
    cell->grid_b.polys = (u32*)((u32)src + src->polys_ofs);
    for (x = 0; x < cell->grid_b.dim_x; x++) {
        for (y = 0; y < cell->grid_b.dim_y; y++) {
            for (z = 0; z < cell->grid_b.dim_z; z++) {
                u32 at = (y + cell->grid_b.dim_y * x) * cell->grid_b.dim_z;
                poly = (u32*)(cell->grid_b.cells[at + z] += (u32)src);
                while ((word = *poly) != 0xFFFFFFFF) {
                    *poly = word + (u32)cell->grid_b.polys;
                    poly++;
                }
            }
        }
    }
}

/* 0x802914CC (0x170): the same load for grid A of land record `index`. */
void pl_land_grid_a_load(PlLandFile* file, s32 index) {
    PlLandFileGrid* src;
    PlLandCell* cell;
    u32 x;
    u32 z;
    u32 y;
    u32 word;
    u32* poly;

    cell = &pl_land_data[index];
    src = &file->grid;
    cell->loaded_a = 1;
    cell->grid_a.min = src->min;
    cell->grid_a.max = src->max;
    if (pl_vec3_fraction_ck(&cell->grid_a.min) == 0 && pl_vec3_fraction_ck(&cell->grid_a.max) == 0) {
        cell->grid_a.off_grid = 0;
    } else {
        cell->grid_a.off_grid = 1;
    }
    cell->grid_a.cell_x = src->cell_x;
    cell->grid_a.cell_y = src->cell_y;
    cell->grid_a.cell_z = src->cell_z;
    cell->grid_a.dim_x = src->dim_x;
    cell->grid_a.dim_y = src->dim_y;
    cell->grid_a.dim_z = src->dim_z;
    cell->grid_a.cells = (u32*)((u32)src + src->cells_ofs);
    cell->grid_a.polys = (u32*)((u32)src + src->polys_ofs);
    for (x = 0; x < cell->grid_a.dim_x; x++) {
        for (y = 0; y < cell->grid_a.dim_y; y++) {
            for (z = 0; z < cell->grid_a.dim_z; z++) {
                u32 at = (y + cell->grid_a.dim_y * x) * cell->grid_a.dim_z;
                poly = (u32*)(cell->grid_a.cells[at + z] += (u32)src);
                while ((word = *poly) != 0xFFFFFFFF) {
                    *poly = word + (u32)cell->grid_a.polys;
                    poly++;
                }
            }
        }
    }
}

/* 0x8029163C - the ground query without the hit flag: forwards to `GetGroundHit2`. */
f32 GetGroundHit(nw4r::math::VEC3* pos, u32 ground, u8 flag) {
    u8 hit;

    return GetGroundHit2(pos, ground, flag, &hit);
}

/* 0x80291664 (0x398): the ground height under `pos` in area `area` (0xFF the current one) from the polygons whose
 * attributes meet `mask`: the highest one at most 50 above `pos`, else the lowest; `*hit` is 0 when none is found
 * (the height is then `pos->y`). */
f32 GetGroundHit2(nw4r::math::VEC3* pos, u32 mask, u8 area, u8* hit) {
    f32 heights[5];
    f32 below[5];
    u32 grid_x;
    u32 grid_z;
    PlCollPoly** taken;
    PlCollPoly** list;
    s8 count;
    PlLandCell* cell;
    u32 y;
    f32 height;
    PlCollPoly* poly;
    s32 found;
    s8 i;
    s8 kept;

    count = 0;
    if (area == 0xFF) {
        area = get_now_areano();
    }
    cell = &pl_land_data[area];
    *hit = 1;
    if (cell->loaded_b == 0) {
        *hit = 0;
        return pos->y;
    }
    heights[0] = pos->y;
    if (pl_land_box_ck(pos, cell) == 1) {
        VEC3 flat_pos;
        VEC3 tri[3];
        pl_land_cell_index(pos, cell, &grid_x, &grid_z);
        flat_pos.x = pos->x;
        flat_pos.y = pos->z;
        y = 0;
        taken = pl_hit_id_list;
        for (; y < cell->grid_b.dim_y; y++) {
            for (list = (PlCollPoly**)pl_land_grid_b_cell(grid_x, y, grid_z, cell); (poly = *list) != (PlCollPoly*)-1;
                 list++) {
                if ((poly->area_mask & stage_area_bit_get(area)) != 0) {
                    continue;
                }
                if (poly->attr == 0) {
                    poly->attr |= 0x20;
                }
                if ((mask & poly->attr) == 0 || poly->normal.y <= 0.0f || pl_hit_id_listed(poly, count) == 1) {
                    continue;
                }
                tri[0].x = poly->vtx0.x;
                tri[0].y = poly->vtx0.z;
                tri[1].x = poly->vtx1.x;
                tri[1].y = poly->vtx1.z;
                tri[2].x = poly->vtx2.x;
                tri[2].y = poly->vtx2.z;
                if (pl_tri2d_point_in(tri, &flat_pos) != 1) {
                    continue;
                }
                height = -(poly->dist + (poly->normal.x * pos->x + poly->normal.z * pos->z)) / poly->normal.y;
                *taken++ = poly;
                heights[count++] = height;
                if (count >= 5) {
                    break;
                }
            }
            if (count >= 5) {
                break;
            }
        }
    } else {
        height = pos->y;
        *hit = 0;
        return height;
    }
    if (count > 1) {
        found = 0;
        kept = 0;
        for (i = 0; i < count; i++) {
            if (heights[i] <= 50.0f + pos->y) {
                found = 1;
                below[kept++] = heights[i];
            }
        }
        if (found == 1) {
            if (kept == 1) {
                return below[0];
            }
            heights[0] = below[0];
            for (i = 1; i < kept; i++) {
                if (heights[0] < below[i]) {
                    heights[0] = below[i];
                }
            }
            return heights[0];
        }
        below[0] = heights[0];
        for (i = 1; i < count; i++) {
            if (below[0] > heights[i]) {
                below[0] = heights[i];
            }
        }
        return below[0];
    }
    height = heights[0];
    if (count == 0) {
        *hit = 0;
    }
    return height;
}

/* 0x802919FC - the 2-layer ground query: resets the caller's `LandData` and asks for the region
 * byte's layers. */
s32 pl_ground_check(_PLW* self, VEC3* pos, LandData* land, f32* out, u32 kind) {
    pl_land_data_reset(land);
    return hit_ground_comon(pos, self->area_0x16, land, out, kind, 2, 0, 0, 0);
}

/* 0x80291A70 (0x98): the enemy's ground query: resets `land` and the ceiling record, then asks for the floor,
 * surface, extra and ceiling heights in the enemy's area, skipping layer 4. */
s32 pl_ground_probe(_ENEMY_WORK* self, VEC3* pos, LandData* land, f32* out, f32* surface, f32* extra,
                    _HIT_TENJO_DATA* tenjo, u32 kind) {
    pl_land_data_reset(land);
    if (tenjo != NULL) {
        tenjo->hit = 0;
        tenjo->kind = 0;
        tenjo->height = 0.0f;
    }
    return hit_ground_comon(pos, self->area_no, land, out, kind, 4, surface, extra, tenjo);
}

/* 0x80291B08 - the fixed-layer ground query: forwards the caller's `LandData`/out/kind and the
 * `_PLW`'s effect key, asking for all 32 layers and no surface data. */
s32 pl_ground_check_all_layers(_PLW* self, VEC3* pos, LandData* land, f32* out, u32 kind) {
    return hit_ground_comon(pos, self->effect_key_0x1A4, land, out, kind, 32, 0, 0, 0);
}

/* 0x80291B48 (0x74): the NPC's ground query: resets `land` and asks for the floor in the NPC's area, every layer. */
s32 npc_ground_check(_PLW* self, VEC3* pos, LandData* land, f32* out, u32 kind) {
    pl_land_data_reset(land);
    return hit_ground_comon(pos, self->field_0x004, land, out, kind, 0, 0, 0, 0);
}

/* 0x80291BBC - the single-layer probe: builds a scratch `LandData` for the query and reports both
 * the query's verdict and the layer it hit. */
s32 stage_water_surface_get(VEC3* pos, u8 flag, u16* hit_layer, f32* out, u32 kind) {
    LandData land;
    s32 found;

    pl_land_data_reset(&land);
    *hit_layer = 0;
    found = hit_ground_comon(pos, flag, &land, out, kind, 1, 0, 0, 0);
    *hit_layer = land.field_0x00;
    return found == 1;
}

/* 0x80291C50 - the kind-only query: no layer mask, no surface data. */
void pl_area_ground_probe(VEC3* pos, u8 flag, LandData* land, u32 kind) {
    f32 out;

    pl_land_data_reset(land);
    hit_ground_comon(pos, flag, land, &out, kind, 0, 0, 0, 0);
}

/* 0x80291CD0 (0x3BC): the ground query under `pos` in area `area` (0xFF, or game mode 2, the current one): of the
 * polygons meeting `kind` and not excluded by `layers`, a surface, extra or ceiling polygon fills its own output and
 * the floors (up to five) go to `pl_ground_select`, which fills `land` and `*out`.  -1 when the area has no land
 * data, 0 when no floor is found (outside the grid `*out` is `pos->y`), else 1. */
s32 hit_ground_comon(nw4r::math::VEC3* pos, u8 area, LandData* land, f32* out, u32 kind, u16 layers, f32* surface,
                     f32* extra, _HIT_TENJO_DATA* tenjo) {
    LandData lands[5];
    f32 heights[5];
    u32 grid_x;
    u32 grid_z;
    f32* height_at;
    LandData* land_at;
    PlCollPoly** taken;
    PlCollPoly** list;
    f32 height;
    s8 count;
    s32 found;
    PlLandCell* cell;
    u32 y;
    PlCollPoly* poly;

    count = 0;
    found = 0;
    if (area == 0xFF || GameMode_ck() == 2) {
        area = get_now_areano();
    }
    cell = &pl_land_data[area];
    if (cell->loaded_b == 0) {
        return -1;
    }
    if (pl_land_box_ck(pos, cell) == 1) {
        VEC3 flat_pos;
        VEC3 tri[3];
        pl_land_cell_index(pos, cell, &grid_x, &grid_z);
        flat_pos.x = pos->x;
        flat_pos.y = pos->z;
        y = 0;
        height_at = heights;
        land_at = lands;
        taken = pl_hit_id_list;
        for (; y < cell->grid_b.dim_y; y++) {
            for (list = pl_land_grid_b_cell(grid_x, y, grid_z, cell); (poly = *list) != (PlCollPoly*)-1; list++) {
                if (poly->attr == 0) {
                    poly->attr |= 0x20;
                }
                if ((kind & poly->attr) == 0) {
                    continue;
                }
                if ((poly->area_mask & stage_area_bit_get(area)) != 0) {
                    continue;
                }
                if ((layers & poly->layer) != 0) {
                    continue;
                }
                if (pl_hit_id_listed(poly, count) == 1) {
                    continue;
                }
                tri[0].x = poly->vtx0.x;
                tri[0].y = poly->vtx0.z;
                tri[1].x = poly->vtx1.x;
                tri[1].y = poly->vtx1.z;
                tri[2].x = poly->vtx2.x;
                tri[2].y = poly->vtx2.z;
                if (pl_tri2d_point_in(tri, &flat_pos) != 1) {
                    continue;
                }
                height = -(poly->dist + (poly->normal.x * pos->x + poly->normal.z * pos->z)) / poly->normal.y;
                if ((poly->attr & 0x4) && surface != NULL) {
                    *surface = height;
                    continue;
                }
                if ((poly->attr & 0x2000) && extra != NULL) {
                    *extra = height;
                    continue;
                }
                if ((poly->attr & 0x10) && tenjo != NULL) {
                    tenjo->hit = 1;
                    tenjo->kind = poly->kind;
                    tenjo->height = height;
                    continue;
                }
                *height_at = height;
                land_at->field_0x00 = poly->attr;
                land_at->field_0x02 = poly->kind;
                land_at->field_0x03 = poly->field_0x03;
                land_at->field_0x04 = poly->layer;
                land_at->field_0x06 = poly->field_0x06;
                land_at->field_0x07 = poly->area_mask & 0xF;
                land_at->vec_0x08 = poly->normal;
                *taken = poly;
                height_at++;
                land_at++;
                taken++;
                count++;
                found = 1;
                if (count >= 5) {
                    break;
                }
            }
            if (count >= 5) {
                break;
            }
        }
    } else {
        *out = pos->y;
        land->field_0x00 = 0;
        land->field_0x02 = 0;
        land->field_0x03 = 0;
        land->field_0x04 = 0;
        land->field_0x06 = 0;
        land->field_0x07 = 0;
        setVector3(&land->vec_0x08, 0.0f, 1.0f, 0.0f);
        return 0;
    }
    if (found == 0) {
        return 0;
    }
    pl_ground_select(land, pos, lands, heights, out, count);
    if (-0.0f == *out) {
        *out = 0.0f;
    }
    return 1;
}

/* 0x8029208C (0x3DC): sweeps a sphere of `radius` from `from` to `to` through the wall polygons of area `area` (0xFF,
 * or game mode 2, the current one) that meet `kind` and not `layers`, and resolves the hits into `pos`; -1 when the
 * area has no wall grid or the move leaves it. */
s32 coll_wall_slide(VEC3* to, VEC3* from, f32 radius, VEC3* pos, u32 kind, u16 layers, u8 mode, u8 area) {
    VEC3 clipped;
    VEC3 mid;
    VEC3 dir;
    VEC3 lo;
    VEC3 hi;
    PlSlideWork work;
    PlCollPoly* seen[10];
    s32 start_x;
    s32 start_y;
    s32 start_z;
    s32 count_x;
    s32 count_y;
    s32 count_z;
    u8 flag;
    PlCollPoly* poly;
    s32 x;
    PlCollPoly** list;
    s32 y;
    s32 z;
    s32 seen_count;
    s32 taken_count;
    PlLandCell* cell;
    s32 grid_y;
    s32 grid_x;
    s32 grid_z;

    if (area == 0xFF || GameMode_ck() == 2) {
        area = get_now_areano();
    }
    cell = &pl_land_data[area];
    if (cell->loaded_a == 0) {
        return -1;
    }
    if (pl_land_grid_a_clip(&clipped, from, to, cell) < 0) {
        return -1;
    }
    for (x = 0; x < 10; x++) {
        pl_coll_slot_kind[x] = 0;
    }
    seen_count = 0;
    taken_count = 0;
    pl_coll_free_count = 0;
    pl_coll_hit_count = 0;
    work.ends[0] = *from;
    work.ends[1] = *to;
    work.radius = radius;
    switch (mode) {
    case 0:
        work.skip_below = 0;
        work.slide = 0;
        break;
    case 1:
        work.slide = 1;
        work.skip_below = 1;
        break;
    case 2:
        work.slide = 1;
        work.skip_below = 0;
        break;
    }
    work.length = sqrtf_thunk(ef_vec3_dist_sq(&work.ends[0], &work.ends[1]));
    dir = work.ends[0] - work.ends[1];
    mid = work.ends[1] + (dir * 0.5f);
    if (vec3_length_sq(&dir.x) > 0.001f) {
        vec3_normalize_into(&dir, &dir);
    }
    lo = *from + (dir * radius);
    hi = *to + (dir * -radius);
    pl_land_grid_a_span(&lo, &hi, &count_x, &count_y, &count_z, cell, &start_x, &start_y, &start_z);
    for (x = 0; x < count_x; x++) {
        for (y = 0; y < count_y; y++) {
            for (z = 0; z < count_z; z++) {
                grid_x = start_x + x;
                grid_y = start_y + y;
                grid_z = start_z + z;
                if (pl_land_grid_a_range_ck(grid_x, grid_y, grid_z, cell) != 1) {
                    continue;
                }
                for (list = pl_land_grid_a_cell(grid_x, grid_y, grid_z, cell); (poly = *list) != (PlCollPoly*)-1;
                     list++) {
                    if (pl_coll_poly_usable(poly, (u32*)seen, seen_count, kind, layers, taken_count, area) == 0) {
                        continue;
                    }
                    if (pl_wall_poly_hit(&work, poly, pos) != 0) {
                        pl_hit_id_list[taken_count++] = poly;
                    }
                    seen[seen_count++] = poly;
                    if (seen_count >= 10) {
                        seen_count = 0;
                    }
                }
            }
        }
    }
    return pl_wall_resolve(&work, pos, &flag);
}

/* 0x802924C0 (0x480): records polygon `poly` in the next hit-box slot when the swept sphere of `work` touches it:
 * the slot keeps the polygon, the sweep start, the touch point (and, for an edge touch, the plane point), and the
 * side/crossing flags; 1 when recorded. */
s32 pl_wall_poly_hit(PlSlideWork* work, PlCollPoly* poly, VEC3* pos) {
    VEC3 touch;
    VEC3 rel;
    VEC3 unused;
    VEC3* edges[4];
    f32 dist;
    f32 best;
    f32 dd;
    s8 flags;
    VEC3** edge;
    s32 i;

    if (pl_coll_hit_count >= 9) {
        return 0;
    }
    if (work->skip_below == 1 && pos->y >= poly->vtx0.y && pos->y >= poly->vtx1.y && pos->y >= poly->vtx2.y) {
        return 0;
    }
    rel = work->ends[1] - poly->vtx0;
    if (vec3_dot(&rel.x, &poly->normal.x) < 0.0f) {
        return 0;
    }
    rel = work->ends[0] - poly->vtx0;
    dist = vec3_dot(&rel.x, &poly->normal.x);
    flags = !(dist > 0.0f);
    if (dist > work->radius) {
        return 0;
    }
    if (dist >= -work->radius) {
        touch = poly->normal * dist;
        touch = work->ends[0] - touch;
    } else {
        rel = work->ends[0] - work->ends[1];
        dd = vec3_dot(&rel.x, &poly->normal.x);
        if (dd == 0.0f) {
            return 0;
        }
        rel = rel * (-(dist / dd));
        touch = work->ends[0] + rel;
        flags |= 2;
    }
    if (pl_tri_point_inside(&touch, &poly->vtx0, &poly->normal) != 0) {
        pl_hit_box_set(&pl_hit_box[pl_coll_hit_count], poly);
        pl_vec3_snap_zero(&pl_hit_box[pl_coll_hit_count].normal, &pl_hit_box[pl_coll_hit_count].normal);
        pl_coll_closest_0[pl_coll_hit_count] = work->ends[0];
        pl_coll_slot_kind[pl_coll_hit_count] = flags;
        pl_coll_slot_used[pl_coll_hit_count] = 0;
        pl_coll_closest_1[pl_coll_hit_count] = touch;
        pl_coll_hit_count++;
        return 1;
    }
    pl_coll_slot_used[pl_coll_hit_count] = 1;
    VEC3 edge_best;
    VEC3 on_plane;
    VEC3 probe;
    on_plane = touch;
    edges[0] = &poly->vtx0;
    edges[1] = &poly->vtx1;
    edges[2] = &poly->vtx2;
    edges[3] = &poly->vtx0;
    edge = edges;
    best = 8000.0f;
    if ((flags & 2) == 0) {
        probe = work->ends[0];
    } else {
        probe = on_plane;
    }
    for (i = 3; i > 0; i--, edge++) {
        dd = sqrt_f32(pl_segment_closest_point(edge[0], edge[1], &probe, &touch));
        if (dd < best) {
            best = dd;
            edge_best = touch;
        }
    }
    if (best < work->radius) {
        pl_hit_box_set(&pl_hit_box[pl_coll_hit_count], poly);
        pl_vec3_snap_zero(&pl_hit_box[pl_coll_hit_count].normal, &pl_hit_box[pl_coll_hit_count].normal);
        pl_coll_closest_0[pl_coll_hit_count] = work->ends[0];
        pl_coll_slot_kind[pl_coll_hit_count] = flags;
        pl_coll_closest_1[pl_coll_hit_count] = edge_best;
        pl_coll_closest_2[pl_coll_hit_count] = on_plane;
        pl_coll_hit_count++;
        return 1;
    }
    return 0;
}

/* A polygon as a plain record: copied as one aggregate, field by field with the vectors word by word (retail's
 * copy).  size: 0x3C */
struct PlCollPolyImage {
    /* +0x00 */ u16 attr;
    /* +0x02 */ u8 kind;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u16 layer;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 area_mask;
    /* +0x08 */ nw4r::math::_VEC3 vtx0;
    /* +0x14 */ nw4r::math::_VEC3 vtx1;
    /* +0x20 */ nw4r::math::_VEC3 vtx2;
    /* +0x2C */ nw4r::math::_VEC3 normal;
    /* +0x38 */ f32 dist;
};

/* 0x80292940 (0x9C): copies one polygon into a hit box. */
void pl_hit_box_set(PlCollPoly* dst, PlCollPoly* src) {
    *(PlCollPolyImage*)dst = *(PlCollPolyImage*)src;
}

/* 0x802929DC (0xB28): resolves the hit boxes the sweep recorded into one push of `pos`: face hits push along their
 * normals, edge hits are dropped when a face hit shares their plane or a corner they lie behind, the per-axis
 * pushes are merged and the total is capped at 1.8 sweep lengths; returns the number of hits. */
s32 pl_wall_resolve(PlSlideWork* work, VEC3* pos, u8* flag) {
    VEC3 push;
    s8 faces[10];
    s8 edges[10];
    s8 drop_kind[10];
    s8 dropped[10];
    PlCollPoly* a;
    PlCollPoly* b;
    nw4r::math::VEC3* corner;
    s8* face_at;
    s8* edge_at;
    s8* drop_at;
    s8* kind_at;
    s8 face_count;
    s8 edge_count;
    s8 drop_count;
    s32 i;
    s32 j;
    s32 n;
    s32 lim;
    s8 slot;
    f32 len;
    f32 cap;
    s32 k;
    s32 m;
    s32 found;
    u8 mask;
    nw4r::math::VEC3* other;
    nw4r::math::VEC3* first;

    if (pl_coll_hit_count != 0) {
        if (pl_coll_hit_count == 1) {
            if (pl_wall_slot_push(work, &push, 0, flag) != 0) {
                if (work->length != 0.0f) {
                    len = vec3_len(&push.x);
                    cap = 1.8f * work->length;
                    if (len > cap) {
                        vec3_scale_in_place(&push, cap / len);
                    }
                }
                *pos += push;
                pl_coll_slot_free[pl_coll_free_count++] = 0;
            }
        } else {
            PlPushBound bound;
            drop_count = 0;
            face_count = 0;
            edge_count = 0;
            for (i = 0; i < pl_coll_hit_count; i++) {
                if (pl_coll_slot_used[i] == 0) {
                    faces[face_count++] = i;
                } else {
                    edges[edge_count++] = i;
                }
            }
            for (i = 0, face_at = faces; i < face_count; i++, face_at++) {
                for (j = 0, edge_at = edges; j < edge_count; j++, edge_at++) {
                    found = 0;
                    for (k = 0; k < drop_count; k++) {
                        if (*edge_at == dropped[k]) {
                            found = 1;
                            break;
                        }
                    }
                    if (found == 1) {
                        continue;
                    }
                    b = &pl_hit_box[*edge_at];
                    a = &pl_hit_box[*face_at];
                    if (a->normal.x == b->normal.x && a->normal.y == b->normal.y && a->normal.z == b->normal.z) {
                        drop_kind[drop_count] = 1;
                        dropped[drop_count++] = *edge_at;
                        continue;
                    }
                    VEC3 face_rel;
                    a = &pl_hit_box[*face_at];
                    if (a->normal.y == 1.0f || a->normal.y == -1.0f) {
                        continue;
                    }
                    b = &pl_hit_box[*edge_at];
                    if (b->normal.y == 1.0f || b->normal.y == -1.0f) {
                        continue;
                    }
                    mask = 0;
                    for (k = 0, first = &a->vtx0, corner = first; k < 3; k++, corner++) {
                        for (m = 0, other = &b->vtx0; m < 3; m++, other++) {
                            if (corner->x == other->x && corner->y == other->y && corner->z == other->z) {
                                mask += (u8)(1 << m);
                                break;
                            }
                        }
                    }
                    switch (pl_coll_shared_vtx_table[mask]) {
                    case 0:
                        corner = &b->vtx0;
                        break;
                    case 1:
                        corner = &b->vtx1;
                        break;
                    case 2:
                        corner = &b->vtx2;
                        break;
                    default:
                        continue;
                    }
                    face_rel = *corner - *first;
                    if (vec3_dot(&face_rel.x, &pl_hit_box[*face_at].normal.x) <= 0.0f) {
                        drop_kind[drop_count] = 2;
                        dropped[drop_count++] = *edge_at;
                    }
                }
            }
            if (edge_count >= 2) {
                kind_at = &drop_kind[drop_count];
                for (i = 0, edge_at = edges; i < edge_count - 1; i++, edge_at++) {
                    found = 0;
                    for (k = 0; k < drop_count; k++) {
                        if (*edge_at == dropped[k]) {
                            found = 1;
                            break;
                        }
                    }
                    if (found == 1) {
                        continue;
                    }
                    for (j = 1, drop_at = &edges[1]; j < edge_count; j++, drop_at++) {
                        if (*edge_at == *drop_at) {
                            continue;
                        }
                        found = 0;
                        for (k = 0; k < drop_count; k++) {
                            if (*drop_at == dropped[k]) {
                                found = 1;
                                break;
                            }
                        }
                        if (found == 1) {
                            continue;
                        }
                        if (pl_coll_closest_1[*edge_at].x == pl_coll_closest_1[*drop_at].x &&
                            pl_coll_closest_1[*edge_at].y == pl_coll_closest_1[*drop_at].y &&
                            pl_coll_closest_1[*edge_at].z == pl_coll_closest_1[*drop_at].z) {
                            *kind_at++ = 3;
                            dropped[drop_count++] = *drop_at;
                        }
                    }
                }
            }
            if (drop_count > 0 && edge_count > drop_count) {
                kind_at = &drop_kind[drop_count];
                for (i = 0, drop_at = drop_kind, face_at = dropped; drop_count > i; i++, drop_at++, face_at++) {
                    if (*drop_at != 1) {
                        continue;
                    }
                    for (j = 0, edge_at = edges; j < edge_count; j++, edge_at++) {
                        if (*edge_at == *face_at) {
                            continue;
                        }
                        VEC3 corner_rel;
                        a = &pl_hit_box[*face_at];
                        if (a->normal.y == 1.0f || a->normal.y == -1.0f) {
                            continue;
                        }
                        b = &pl_hit_box[*edge_at];
                        if (b->normal.y == 1.0f || b->normal.y == -1.0f) {
                            continue;
                        }
                        mask = 0;
                    for (k = 0, first = &a->vtx0, corner = first; k < 3; k++, corner++) {
                        for (m = 0, other = &b->vtx0; m < 3; m++, other++) {
                            if (corner->x == other->x && corner->y == other->y && corner->z == other->z) {
                                mask += (u8)(1 << m);
                                break;
                            }
                        }
                    }
                    switch (pl_coll_shared_vtx_table[mask]) {
                        case 0:
                            corner = &b->vtx0;
                            break;
                        case 1:
                            corner = &b->vtx1;
                            break;
                        case 2:
                            corner = &b->vtx2;
                            break;
                        default:
                            continue;
                        }
                        corner_rel = *corner - *first;
                        if (vec3_dot(&corner_rel.x, &pl_hit_box[*face_at].normal.x) <= 0.0f) {
                            *kind_at++ = 2;
                            dropped[drop_count++] = *edge_at;
                        }
                    }
                }
            }
            setVector3(&bound.pos, 0.0f, 0.0f, 0.0f);
            setVector3(&bound.neg, 0.0f, 0.0f, 0.0f);
            setVector3(&bound.pos_rest, 0.0f, 0.0f, 0.0f);
            setVector3(&bound.neg_rest, 0.0f, 0.0f, 0.0f);
            for (i = 0, face_at = faces; i < face_count; i++, face_at++) {
                slot = *face_at;
                if (pl_wall_slot_push(work, &push, slot, flag) != 0) {
                    pl_push_bound_add(&push, &bound, work->radius);
                    pl_coll_slot_free[pl_coll_free_count++] = slot;
                }
            }
            for (i = 0, edge_at = edges; i < edge_count; i++, edge_at++) {
                slot = *edge_at;
                found = 0;
                for (k = 0; k < drop_count; k++) {
                    if (slot == dropped[k]) {
                        found = 1;
                        break;
                    }
                }
                if (found == 0 && pl_wall_slot_push(work, &push, slot, flag) != 0) {
                    pl_push_bound_add(&push, &bound, work->radius);
                    pl_coll_slot_free[pl_coll_free_count++] = slot;
                }
            }
            if (bound.neg.x != 0.0f) {
                if (bound.pos.x != 0.0f) {
                    push.x = bound.neg_rest.x + (bound.pos_rest.x + 0.5f * (bound.pos.x + bound.neg.x));
                } else {
                    push.x = bound.neg.x + bound.neg_rest.x;
                }
            } else {
                push.x = bound.pos.x + bound.pos_rest.x;
            }
            if (bound.neg.y != 0.0f) {
                if (bound.pos.y != 0.0f) {
                    push.y = bound.neg_rest.y + (bound.pos_rest.y + 0.5f * (bound.pos.y + bound.neg.y));
                } else {
                    push.y = bound.neg.y + bound.neg_rest.y;
                }
            } else {
                push.y = bound.pos.y + bound.pos_rest.y;
            }
            if (bound.neg.z != 0.0f) {
                if (bound.pos.z != 0.0f) {
                    push.z = bound.neg_rest.z + (bound.pos_rest.z + 0.5f * (bound.pos.z + bound.neg.z));
                } else {
                    push.z = bound.neg.z + bound.neg_rest.z;
                }
            } else {
                push.z = bound.pos.z + bound.pos_rest.z;
            }
            if (work->length != 0.0f) {
                len = vec3_len(&push.x);
                cap = 1.8f * work->length;
                if (len > cap) {
                    vec3_scale_in_place(&push, cap / len);
                }
            }
            *pos += push;
        }
    }
    return pl_coll_hit_count;
}

/* 0x80293504 (0x578): the push hit-box `slot` asks for: a face hit pushes the sweep end out along the normal (when
 * sliding, flattened and kept off the face's edges), an edge hit pushes away from the touch point; 0 when the push
 * is zero.  `*flag` is set for a crossing hit. */
s8 pl_wall_slot_push(PlSlideWork* work, VEC3* push, s8 slot, u8* flag) {
    PlBox unused_box;
    PlCollPoly* box;
    nw4r::math::VEC3* touch;
    nw4r::math::VEC3* from;
    f32 len;
    f32 dy;
    f32 side;

    VEC3 normal;
    VEC3 offset;
    VEC3 point;
    VEC3 flat;
    VEC3 probe;
    VEC3 nearest;
    if (pl_coll_slot_used[slot] != 1) {
        box = &pl_hit_box[slot];
        normal = box->normal;
        offset = normal * work->radius;
        point = pl_coll_closest_1[slot] + offset;
        from = &pl_coll_closest_0[slot];
        *push = point - *from;
        if (work->slide == 1) {
            push->y = 0.0f;
            if (abs_f32(normal.x) >= 0.001f || abs_f32(normal.z) >= 0.001f) {
                normal.y = 0.0f;
                if ((len = vec3_length_sq(&normal.x)) > 0.001f) {
                    vec3_scale_inv(push, len);
                    probe = *from + *push;
                    if ((u8)pl_triangle_closest_point(&probe, &box->vtx0, &box->vtx1, &box->vtx2, &nearest) != 6) {
                        offset = nearest - *from;
                        if (offset.x * normal.x + offset.z * normal.z > 0.0f) {
                            offset = probe - nearest;
                            if (offset.x * normal.x + offset.z * normal.z > 0.0f) {
                                if (vec3_length_sq(&offset.x) > work->radius * work->radius) {
                                    dy = nearest.y - probe.y;
                                    vec3_sub_assign((Vec*)push,
                                                    (Vec*)&static_cast<const VEC3&>(vec3_scale_to(
                                                        sqrt_f32(work->radius * work->radius - dy * dy), normal)));
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        if (pl_coll_slot_kind[slot] == 0) {
            touch = &pl_coll_closest_1[slot];
            point = pl_coll_closest_0[slot] - *touch;
        } else {
            touch = &pl_coll_closest_1[slot];
            point = pl_coll_closest_2[slot] - *touch;
        }
        if (work->slide == 0) {
            normal = point;
            if (vec3_length_sq(&normal.x) > 0.001f) {
                vec3_normalize_into(&normal, &normal);
            } else {
                normal += pl_hit_box[slot].normal;
            }
            offset = normal * work->radius;
            point = *touch + offset;
            VEC3 start;
            setVector3(&start, pl_coll_closest_0[slot].x, pl_coll_closest_0[slot].y, pl_coll_closest_0[slot].z);
            offset = point - start;
        } else {
            flat = point;
            normal = flat;
            normal.y = 0.0f;
            if (vec3_length_sq(&normal.x) > 0.001f) {
                vec3_normalize_into(&normal, &normal);
            } else {
                normal = pl_hit_box[slot].normal;
                normal.y = 0.0f;
                if (vec3_length_sq(&normal.x) > 0.001f) {
                    vec3_normalize_into(&normal, &normal);
                }
            }
            side = sqrt_f32(work->radius * work->radius - flat.y * flat.y);
            point.x = side * normal.x;
            point.z = side * normal.z;
            point.y = flat.y;
            if (pl_coll_slot_kind[slot] == 0) {
                offset = point - flat;
            } else {
                offset = (point + *touch) - pl_coll_closest_0[slot];
            }
            offset.y = 0.0f;
        }
        if (vec3_len(&offset.x) == 0.0f) {
            return 0;
        }
        *push = offset;
    }
    if (pl_coll_slot_kind[slot] == 1) {
        *flag = 1;
    }
    return 1;
}

/* 0x80293A7C - scale `v` by the reciprocal of `divisor` through the in-place scale `vec3_scale_in_place`;
 * the tail call is the whole body. */
void vec3_scale_inv(VEC3* v, f32 divisor) {
    return vec3_scale_in_place(v, 1.0f / divisor);
}

/* 0x80293A88 (0x5B4): sweeps a sphere of `radius` from `from` to `to` through the
 * polygons of area `area` (0xFF the current one) in grid A (`flags` bit 0) and grid B (bit 1); with `hit` null it
 * answers 1 on the first hit, else it collects every hit and returns `pl_line_hit_nearest`'s answer (the nearest one
 * in `hit`); -1 when the area has no land data. */
s32 coll_line_check(VEC3* to, VEC3* from, f32 radius, VEC3* hit, u8 flags, u32 kind, u8 area, u16 layers) {
    VEC3 lo_copy;
    VEC3 lo_copy2;
    PlSlideWork work;
    PlCollPoly* seen[10];
    s32 start_x;
    s32 start_y;
    s32 start_z;
    s32 count_x;
    s32 count_y;
    s32 count_z;
    PlCollPoly* poly;
    s32 y;
    s32 z;
    s32 x;
    PlCollPoly** list;
    s32 grid_x;
    s32 grid_y;
    s32 grid_z;
    s32 i;
    s32 seen_count;
    s32 collect;
    PlLandCell* cell;
    s32 taken_count;

    collect = hit != NULL;
    if (area == 0xFF) {
        area = get_now_areano();
    }
    cell = &pl_land_data[area];
    VEC3 lo;
    VEC3 hi;
    VEC3 dir;
    VEC3 mid;
    if (cell->loaded_b == 0 && cell->loaded_a == 0) {
        return -1;
    }
    dir = *to - *from;
    if (vec3_length_sq(&dir.x) > 0.001f) {
        vec3_normalize_into(&dir, &dir);
    }
    lo = *to + (dir * radius);
    hi = *from + (dir * -radius);
    lo_copy = lo;
    lo_copy2 = lo;
    for (i = 0; i < 10; i++) {
        seen[i] = 0;
        pl_coll_slot_dist[i] = 0.0f;
        pl_coll_slot_free[i] = 0;
    }
    seen_count = 0;
    taken_count = 0;
    pl_coll_free_count = 0;
    pl_coll_hit_count = 0;
    work.ends[0] = *to;
    work.ends[1] = *from;
    work.radius = radius;
    work.slide = 0;
    work.skip_below = 0;
    work.length = sqrtf_thunk(ef_vec3_dist_sq(&work.ends[0], &work.ends[1]));
    dir = work.ends[0] - work.ends[1];
    mid = work.ends[1] + (dir * 0.5f);
    if ((u8)flags & 1) {
        pl_land_grid_a_span(&lo, &hi, &count_x, &count_y, &count_z, cell, &start_x, &start_y, &start_z);
        for (x = 0; x < count_x; x++) {
            for (y = 0; y < count_y; y++) {
                for (z = 0; z < count_z; z++) {
                    grid_x = start_x + x;
                    grid_y = start_y + y;
                    grid_z = start_z + z;
                    if (pl_land_grid_a_range_ck(grid_x, grid_y, grid_z, cell) != 1) {
                        continue;
                    }
                    for (list = pl_land_grid_a_cell(grid_x, grid_y, grid_z, cell); (poly = *list) != (PlCollPoly*)-1;
                         list++) {
                        if (pl_coll_poly_usable(poly, (u32*)seen, seen_count, kind, layers, taken_count, area) == 0) {
                            continue;
                        }
                        if (pl_line_poly_hit(&work, poly, collect) != 0) {
                            if (collect == 0) {
                                return 1;
                            }
                            pl_hit_id_list[taken_count++] = poly;
                        }
                        seen[seen_count++] = poly;
                        if (seen_count >= 10) {
                            seen_count = 0;
                        }
                    }
                }
            }
        }
    }
    if ((u8)flags & 2) {
        pl_land_grid_b_span(&lo, &hi, &count_x, &count_y, &count_z, cell, &start_x, &start_y, &start_z);
        for (x = 0; x < count_x; x++) {
            for (y = 0; y < count_y; y++) {
                for (z = 0; z < count_z; z++) {
                    grid_x = start_x + x;
                    grid_y = start_y + y;
                    grid_z = start_z + z;
                    if (pl_land_grid_b_range_ck(grid_x, grid_y, grid_z, cell) != 1) {
                        continue;
                    }
                    for (list = pl_land_grid_b_cell(grid_x, grid_y, grid_z, cell); (poly = *list) != (PlCollPoly*)-1;
                         list++) {
                        if (pl_coll_poly_usable(poly, (u32*)seen, seen_count, kind, layers, taken_count, area) == 0) {
                            continue;
                        }
                        if (pl_line_poly_hit(&work, poly, collect) != 0) {
                            if (collect == 0) {
                                return 1;
                            }
                            pl_hit_id_list[taken_count++] = poly;
                        }
                        seen[seen_count++] = poly;
                        if (seen_count >= 10) {
                            seen_count = 0;
                        }
                    }
                }
            }
        }
    }
    if (hit != NULL) {
        return pl_line_hit_nearest(&work, hit);
    }
    return 0;
}

/* 0x8029403C (0x4FC): the yes/no form of the sweep: 1 on the first polygon the sphere of
 * `radius` meets between `from` and `to` in grid A (`flags` bit 0) or grid B (bit 1) of area `area`. */
s32 pl_coll_sweep_ck(VEC3* to, VEC3* from, f32 radius, u32 flags, u32 kind, u8 area, u16 layers) {
    VEC3 lo_copy;
    VEC3 lo_copy2;
    PlSlideWork work;
    PlCollPoly* seen[10];
    s32 start_x;
    s32 start_y;
    s32 start_z;
    s32 count_x;
    s32 count_y;
    s32 count_z;
    PlLandCell* cell;
    s32 x;
    s32 y;
    PlCollPoly** list;
    PlCollPoly* poly;
    s32 z;
    s32 grid_x;
    s32 grid_y;
    s32 grid_z;
    s32 i;

    if (area == 0xFF) {
        area = get_now_areano();
    }
    cell = &pl_land_data[area];
    VEC3 lo;
    VEC3 hi;
    VEC3 dir;
    VEC3 mid;
    if (cell->loaded_b == 0 && cell->loaded_a == 0) {
        return 0;
    }
    dir = *to - *from;
    if (vec3_length_sq(&dir.x) > 0.001f) {
        vec3_normalize_into(&dir, &dir);
    }
    lo = *to + (dir * radius);
    hi = *from + (dir * -radius);
    lo_copy = lo;
    lo_copy2 = lo;
    for (i = 0; i < 10; i++) {
        seen[i] = 0;
        pl_coll_slot_dist[i] = 0.0f;
        pl_coll_slot_free[i] = 0;
    }
    pl_coll_free_count = 0;
    pl_coll_hit_count = 0;
    work.ends[0] = *to;
    work.ends[1] = *from;
    work.radius = radius;
    work.slide = 0;
    work.skip_below = 0;
    work.length = sqrtf_thunk(ef_vec3_dist_sq(&work.ends[0], &work.ends[1]));
    dir = work.ends[0] - work.ends[1];
    mid = work.ends[1] + (dir * 0.5f);
    if ((u8)flags & 1) {
        pl_land_grid_a_span(&lo, &hi, &count_x, &count_y, &count_z, cell, &start_x, &start_y, &start_z);
        for (x = 0; x < count_x; x++) {
            for (y = 0; y < count_y; y++) {
                for (z = 0; z < count_z; z++) {
                    grid_x = start_x + x;
                    grid_y = start_y + y;
                    grid_z = start_z + z;
                    if (pl_land_grid_a_range_ck(grid_x, grid_y, grid_z, cell) != 1) {
                        continue;
                    }
                    for (list = pl_land_grid_a_cell(grid_x, grid_y, grid_z, cell); (poly = *list) != (PlCollPoly*)-1;
                         list++) {
                        if (pl_coll_poly_usable(poly, (u32*)seen, 0, kind, layers, 0, area) == 0) {
                            continue;
                        }
                        if (pl_sweep_poly_hit(&work, poly) != 0) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    if ((u8)flags & 2) {
        pl_land_grid_b_span(&lo, &hi, &count_x, &count_y, &count_z, cell, &start_x, &start_y, &start_z);
        for (x = 0; x < count_x; x++) {
            for (y = 0; y < count_y; y++) {
                for (z = 0; z < count_z; z++) {
                    grid_x = start_x + x;
                    grid_y = start_y + y;
                    grid_z = start_z + z;
                    if (pl_land_grid_b_range_ck(grid_x, grid_y, grid_z, cell) != 1) {
                        continue;
                    }
                    for (list = pl_land_grid_b_cell(grid_x, grid_y, grid_z, cell); (poly = *list) != (PlCollPoly*)-1;
                         list++) {
                        if (pl_coll_poly_usable(poly, (u32*)seen, 0, kind, layers, 0, area) == 0) {
                            continue;
                        }
                        if (pl_sweep_poly_hit(&work, poly) != 0) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/* 0x80294538 (0x62C): records polygon `poly` in the next hit-box slot when the swept sphere of `work` crosses it: a
 * face crossing keeps the crossing point and its distance along the sweep; otherwise the sweep is tested as a capsule
 * against the three edges and the nearest edge hit is kept (kind 127).  With `collect` 0 it only answers. */
s32 pl_line_poly_hit(PlSlideWork* work, PlCollPoly* poly, u8 collect) {
    PlCapsuleBox edge_box;
    f32 dist;
    f32 best;
    s8 flags;
    s8 slot;
    s32 hits;
    u32 i;

    PlCapsule edge;
    VEC3 corners[4];
    PlSegment seg;
    nw4r::math::PLANE plane;
    VEC3 point;
    VEC3 unused_a;
    VEC3 rel;
    VEC3 unused_b;
    if (pl_coll_hit_count >= 9) {
        return 0;
    }
    rel = work->ends[1] - poly->vtx0;
    if (vec3_dot(&rel.x, &poly->normal.x) < 0.0f) {
        return 0;
    }
    rel = work->ends[0] - poly->vtx0;
    dist = vec3_dot(&rel.x, &poly->normal.x);
    flags = !(dist > 0.0f);
    if (dist > work->radius) {
        return 0;
    }
    if (dist < -work->radius) {
        flags |= 2;
    }
    VEC3 back = poly->normal * -1.0f;
    point = work->ends[1] + (back * work->radius);
    VEC3 dir = work->ends[0] - work->ends[1];
    seg.start = point;
    seg.end = work->ends[0] + (back * work->radius);
    plane.N = poly->normal;
    plane.d = vec3_dot(&poly->normal.x, &poly->vtx0.x);
    if (pl_segment_plane_hit(&seg, &plane, &point) == 1 && pl_tri_point_inside(&point, &poly->vtx0, &poly->normal) != 0) {
        if (collect == 0) {
            return 1;
        }
        VEC3 travel = point - seg.start;
        pl_hit_box_set(&pl_hit_box[pl_coll_hit_count], poly);
        pl_vec3_snap_zero(&pl_hit_box[pl_coll_hit_count].normal, &pl_hit_box[pl_coll_hit_count].normal);
        pl_coll_closest_0[pl_coll_hit_count] = work->ends[1];
        slot = pl_coll_hit_count;
        pl_coll_slot_kind[slot] = flags;
        pl_coll_slot_used[slot] = 0;
        pl_coll_slot_dist[slot] = vec3_len(&travel.x);
        pl_coll_closest_1[slot] = point;
        pl_coll_hit_count++;
        return 1;
    }
    pl_coll_slot_used[pl_coll_hit_count] = 1;
    VEC3 best_seg;
    VEC3 best_cap;
    VEC3 on_seg;
    VEC3 on_cap;
    VEC3 unused_c;
    VEC3 unused_d;
    VEC3 best_dir;
    VEC3 rev = -dir;
    PlSegmentBox sweep_box;
    sweep_box.start = work->ends[1];
    sweep_box.end = work->ends[0];
    sweep_box.dir = sweep_box.end - sweep_box.start;
    VEC3 half = sweep_box.dir * 0.5f;
    sweep_box.center = work->ends[1] + half;
    sweep_box.half_length = vec3_len(&half.x);
    corners[0] = poly->vtx0;
    corners[1] = poly->vtx1;
    corners[2] = poly->vtx2;
    corners[3] = poly->vtx0;
    hits = 0;
    best = 1000000.0f;
    for (i = 0; i < 3; i++) {
        edge.start = corners[i];
        edge.end = corners[i + 1];
        edge.radius = work->radius;
        pl_capsule_box_set(&edge, &edge_box);
        if (pl_segment_capsule_hit(&sweep_box, &edge_box, &on_seg, &on_cap) != 0) {
            hits++;
            dir = on_seg - work->ends[1];
            dist = vec3_len(&dir.x);
            if (best > dist) {
                best = dist;
                best_seg = on_seg;
                best_cap = on_cap;
                best_dir = dir;
            }
        }
    }
    if (hits != 0) {
        if (collect == 0) {
            return 1;
        }
        pl_hit_box_set(&pl_hit_box[pl_coll_hit_count], poly);
        if (vec3_length_sq(&best_dir.x) > 0.001f) {
            vec3_normalize_into(&best_dir, &best_dir);
        }
        pl_hit_box[pl_coll_hit_count].normal = best_dir;
        pl_vec3_snap_zero(&pl_hit_box[pl_coll_hit_count].normal, &pl_hit_box[pl_coll_hit_count].normal);
        pl_coll_closest_0[pl_coll_hit_count] = work->ends[1];
        pl_coll_slot_kind[pl_coll_hit_count] = 127;
        pl_coll_closest_1[pl_coll_hit_count] = best_seg;
        pl_coll_closest_2[pl_coll_hit_count] = best_seg;
        pl_coll_slot_dist[pl_coll_hit_count] = best;
        pl_coll_hit_count++;
        return 1;
    }
    return 0;
}

/* 0x80294B64 (0x398): 1 when the swept sphere of `work` crosses polygon `poly`, through its face or against one of its
 * edges (tested as capsules). */
s32 pl_sweep_poly_hit(PlSlideWork* work, PlCollPoly* poly) {
    VEC3 point;
    VEC3 unused_a;
    VEC3 rel;
    VEC3 unused_b;
    u32 i;

    rel = work->ends[1] - poly->vtx0;
    if (vec3_dot(&rel.x, &poly->normal.x) < 0.0f) {
        return 0;
    }
    rel = work->ends[0] - poly->vtx0;
    if (vec3_dot(&rel.x, &poly->normal.x) > work->radius) {
        return 0;
    }
    VEC3 back = poly->normal * -1.0f;
    point = work->ends[1] + (back * work->radius);
    VEC3 dir = work->ends[0] - work->ends[1];
    PlSegment seg;
    nw4r::math::PLANE plane;
    seg.start = point;
    seg.end = work->ends[0] + (back * work->radius);
    plane.N = poly->normal;
    plane.d = vec3_dot(&poly->normal.x, &poly->vtx0.x);
    if (pl_segment_plane_hit(&seg, &plane, &point) == 1 && pl_tri_point_inside(&point, &poly->vtx0, &poly->normal) != 0) {
        return 1;
    }
    VEC3 unused_c;
    VEC3 unused_d;
    VEC3 unused_e;
    VEC3 unused_f;
    VEC3 unused_g;
    VEC3 unused_h;
    VEC3 unused_i;
    VEC3 rev = -dir;
    PlSegmentBox sweep_box;
    sweep_box.start = work->ends[1];
    sweep_box.end = work->ends[0];
    sweep_box.dir = sweep_box.end - sweep_box.start;
    VEC3 half = sweep_box.dir * 0.5f;
    sweep_box.center = work->ends[1] + half;
    sweep_box.half_length = vec3_len(&half.x);
    VEC3 corners[4];
    corners[0] = poly->vtx0;
    corners[1] = poly->vtx1;
    corners[2] = poly->vtx2;
    corners[3] = poly->vtx0;
    for (i = 0; i < 3; i++) {
        PlCapsuleBox edge_box;
        edge_box.start = corners[i];
        edge_box.end = corners[i + 1];
        edge_box.radius = work->radius;
        edge_box.dir = edge_box.end - edge_box.start;
        edge_box.center = edge_box.start + (edge_box.dir * 0.5f);
        edge_box.reach = vec3_len(&edge_box.dir.x);
        if (pl_segment_capsule_hit(&sweep_box, &edge_box, NULL, NULL) != 0) {
            return 1;
        }
    }
    return 0;
}

/* 0x80294EFC (0x1DC): the nearest of the line check's hits into `hit` (a face hit's point pushed out by the radius, an
 * edge hit's sweep point at its distance); returns the hit count. */
s32 pl_line_hit_nearest(PlSlideWork* work, VEC3* hit) {
    VEC3 unused;
    s32 nearest;
    f32 best;
    s32 count;
    s32 i;

    count = pl_coll_hit_count;
    if (count != 0) {
        if (count == 1) {
            if (pl_coll_slot_kind[0] == 127) {
                *hit = pl_coll_closest_0[0] + (pl_hit_box[0].normal * pl_coll_slot_dist[0]);
            } else {
                *hit = pl_coll_closest_1[0];
                *hit += pl_hit_box[0].normal * work->radius;
            }
        } else {
            best = 100000.0f;
            for (i = 0; i < count; i++) {
                if (pl_coll_slot_dist[i] < best) {
                    nearest = i;
                    best = pl_coll_slot_dist[i];
                }
            }
            if (pl_coll_slot_kind[nearest] == 127) {
                *hit = pl_coll_closest_0[nearest] + (pl_hit_box[nearest].normal * pl_coll_slot_dist[nearest]);
            } else {
                *hit = pl_coll_closest_1[nearest];
                *hit += pl_hit_box[nearest].normal * work->radius;
            }
        }
    }
    return pl_coll_hit_count;
}

/* 0x802950D8 (0x1B8): sweeps the player's body shape from its old position (+0x48) to its new one (+0x3C) through
 * the walls; `mode` 0 first clears the wall-direction bits. */
void pl_coll_shape_set(_PLW* self, u8 mode, u16 layers) {
    PlBodySegment seg;
    HitSphere* shape;
    f32 radius;
    f32 lift;

    VEC3 scale;
    if (mode == 0) {
        self->field_0x364 = 0;
    }
    shape = pl_coll_body_shape;
    if (self->kind_0x09 == 3) {
        lift = 50.0f;
    } else {
        lift = 0.0f;
    }
    setVector3(&scale, 1.0f, 1.0f, 1.0f);
    for (; shape->radius != -1.0f; shape++) {
        radius = shape->radius * scale.x;
        seg.start.x = self->vec_0x048.x + shape->pos.x * scale.x;
        seg.start.y = self->vec_0x048.y + scale.y * (shape->pos.y - lift);
        seg.start.z = self->vec_0x048.z + shape->pos.z * scale.z;
        seg.end.x = self->vec_0x03C.x + shape->pos.x * scale.x;
        seg.end.y = self->vec_0x03C.y + scale.y * (shape->pos.y - lift);
        seg.end.z = self->vec_0x03C.z + shape->pos.z * scale.z;
        VEC3 start_copy = seg.start;
        pl_coll_shape_hits(&seg, radius, 0xFFFF, layers, self);
    }
}

/* 0x80295290 (0x2B4): slides the player's sphere `radius` along `seg` against the walls, then records the walls it
 * touched in its move-work chunk (a floor-kind wall moved to the front) and their directions in +0x364. */
void pl_coll_shape_hits(PlBodySegment* seg, f32 radius, u32 kind, u16 layers, _PLW* self) {
    VEC3 unused;
    PlCollPoly* poly;
    s8* free_at;
    u32 i;
    u32 angle;
    u8 count;
    s32 swapped;
    s32 last;

    coll_wall_slide(&seg->start, &seg->end, radius, &self->vec_0x03C, kind, layers, self->kind_0x09 != 3, self->area_0x16);
    if (self->chunk_ofs < 4) {
        count = 0;
        swapped = 0;
        pl_move_work[self->chunk_ofs][0].kind = 0;
        if (pl_coll_free_count != 0) {
            for (i = 0, free_at = pl_coll_slot_free; i < pl_coll_free_count; i++, free_at++) {
                poly = pl_hit_id_list[*free_at];
                angle = calc_xz_angle(self->vec_0x03C.x, self->vec_0x03C.z, pl_coll_closest_1[*free_at].x,
                                      pl_coll_closest_1[*free_at].z);
                angle = (u16)((u16)angle - self->field_0x058);
                pl_move_work[self->chunk_ofs][count].kind = 1;
                if (poly->attr & 8) {
                    pl_move_work[self->chunk_ofs][count].kind |= 2;
                    swapped = 1;
                }
                pl_move_work[self->chunk_ofs][count].carve_0x14 = poly->field_0x03;
                pl_move_work[self->chunk_ofs][count].angle = angle;
                pl_move_work[self->chunk_ofs][count++].pos = poly->normal;
                if (swapped) {
                    swapped = 0;
                    if (count != 1) {
                        last = count - 1;
                        PlMoveEntry swap;
                        swap.kind = pl_move_work[self->chunk_ofs][0].kind;
                        swap.carve_0x14 = pl_move_work[self->chunk_ofs][0].carve_0x14;
                        swap.angle = pl_move_work[self->chunk_ofs][0].angle;
                        swap.pos = pl_move_work[self->chunk_ofs][0].pos;
                        pl_move_work[self->chunk_ofs][0].kind = pl_move_work[self->chunk_ofs][last].kind;
                        pl_move_work[self->chunk_ofs][0].carve_0x14 = pl_move_work[self->chunk_ofs][last].carve_0x14;
                        pl_move_work[self->chunk_ofs][0].angle = pl_move_work[self->chunk_ofs][last].angle;
                        pl_move_work[self->chunk_ofs][0].pos = pl_move_work[self->chunk_ofs][last].pos;
                        pl_move_work[self->chunk_ofs][last].kind = swap.kind;
                        pl_move_work[self->chunk_ofs][last].carve_0x14 = swap.carve_0x14;
                        pl_move_work[self->chunk_ofs][last].angle = swap.angle;
                        pl_move_work[self->chunk_ofs][last].pos = swap.pos;
                    }
                }
                self->field_0x364 |= 1 << (angle >> 11);
            }
            pl_move_work[self->chunk_ofs][count].kind = 0;
        }
    }
}

/* 0x80295578 (0x1C4): sweeps the enemy's body sphere (its move-parameter offset turned by its yaw, sized from its
 * data and scale) from its previous position to its current one through the walls; `mode` 0 first clears the
 * wall-direction bits. */
void em_coll_shape_set(_ENEMY_WORK* self, u8 mode, u16 layers) {
    PlBodySegment seg;
    VEC3 unused;
    PlCollSphere body;
    f32* params;
    f32 radius;

    if (mode == 0) {
        self->field_0x218 = 0;
    }
    params = get_enemy_data(self)->move_params_0x50;
    body.pos = self->move_param_0x21C;
    rotVecY(&body.pos, self->field_0x1C0);
    body.radius = params[9];
    if (self->team == 18 && stage_map_kind_get(self->field_0x1E0) == 3 && self->area_no == 3 &&
        self->field_0x1E2 == 2) {
        body.radius = 300.0f;
    }
    if (self->field_0x1D0 >= 0.6f && self->field_0x1D0 <= 2.0f) {
        if ((self->field_0x1C8 & 0x20000) && self->field_0x1E2 == 1 && self->field_0x1E3 == 0) {
            body.radius *= get_em_chg_scale(self);
        }
    } else {
        body.radius = body.radius * self->field_0x1D0;
    }
    radius = body.radius;
    seg.start.x = self->vec_0x194.x + body.pos.x;
    seg.start.y = self->vec_0x194.y + body.pos.y;
    seg.start.z = self->vec_0x194.z + body.pos.z;
    seg.end.x = self->pos.x + body.pos.x;
    seg.end.y = self->pos.y + body.pos.y;
    seg.end.z = self->pos.z + body.pos.z;
    em_coll_shape_hits(&seg, radius, 0xFFFF, layers, self);
}

/* 0x8029576C (0x1B8): slides the enemy's sphere `radius` along `seg` against the walls, then records the walls it
 * touched in its slot table (up to ten, ended by a zero flag word) and their directions in +0x218. */
void em_coll_shape_hits(PlBodySegment* seg, f32 radius, u32 kind, u16 layers, _ENEMY_WORK* self) {
    VEC3 unused;
    EmMotionSlot* slot;
    PlCollPoly* poly;
    s8* free_at;
    u32 i;
    u32 angle;
    u8 count;
    s32 mode;

    count = 0;
    unused = self->pos;
    if (self->field_0x1E2 == 2 || (self->field_0x1E2 == 1 && self->field_0x1E3 == 1)) {
        mode = 0;
    } else {
        mode = (self->field_0x1E2 == 1) + 1;
    }
    coll_wall_slide(&seg->start, &seg->end, radius, &self->pos, kind, layers, mode, self->area_no);
    self->slots_0x244[0].flags = 0;
    if (pl_coll_free_count != 0) {
        for (i = 0, free_at = pl_coll_slot_free; i < pl_coll_free_count; free_at++, i++) {
            poly = pl_hit_id_list[*free_at];
            angle = calc_xz_angle(self->pos.x, self->pos.z, pl_coll_closest_1[*free_at].x,
                                  pl_coll_closest_1[*free_at].z);
            angle = (u16)((u16)angle - self->field_0x1C0);
            slot = &self->slots_0x244[count];
            slot->flags = poly->attr;
            slot->kind = poly->kind;
            slot->value = angle;
            count++;
            slot->vec = poly->normal;
            self->field_0x218 |= 1 << (angle >> 11);
        }
        if (count < 10) {
            self->slots_0x244[count].flags = 0;
        }
    }
}

/* 0x80295924 - the intersection query without a land record: `tenjo` 0, no `LandData`. */
s32 findInterSection(nw4r::math::VEC3* from, nw4r::math::VEC3* to, nw4r::math::VEC3* out, u8 group,
                     u32 ground, u8 area_no, u16 angle, u8* out_flags) {
    return findInterSection3(from, to, out, group, ground, area_no, angle, out_flags, 0, 0);
}

/* 0x8029595C - the same query with a `LandData` record, tagged `tenjo` 1. */
s32 findInterSection2(nw4r::math::VEC3* from, nw4r::math::VEC3* to, nw4r::math::VEC3* out, u8 group,
                      u32 ground, u8 area_no, u16 angle, u8* out_flags, LandData* land) {
    return findInterSection3(from, to, out, group, ground, area_no, angle, out_flags, land, 1);
}

/* 0x80295998 (0x55C): the segment `from`..`to` against the polygons of area `area` (0xFF the current one) in grid A
 * (`flags` bit 0) and grid B (bit 1): the crossings are sorted by distance, the nearest goes to `out` and its grid to
 * `*grid_hit`, and up to `max` of their records to `land`; returns the number of crossings. */
s32 findInterSection3(nw4r::math::VEC3* from, nw4r::math::VEC3* to, nw4r::math::VEC3* out, u8 flags, u32 kind, u8 area,
                      u16 layers, u8* grid_hit, LandData* land, u8 max) {
    PlCollPoly* seen[10];
    VEC3 copy_a;
    VEC3 copy_b;
    s32 start_x;
    s32 start_y;
    s32 start_z;
    s32 count_x;
    s32 count_y;
    s32 count_z;
    PlCollPoly** taken;
    PlCollPoly* poly;
    s32 y;
    s32 z;
    s32 x;
    PlCollPoly** list;
    s32 grid_x;
    s32 grid_y;
    s32 grid_z;
    s32 i;
    s32 seen_count;
    PlLandCell* cell;

    if (area == 0xFF) {
        area = get_now_areano();
    }
    cell = &pl_land_data[area];
    VEC3 seg_to;
    VEC3 seg_from;
    VEC3 dir;
    VEC3 mid;
    dir = *to - *from;
    if (vec3_length_sq(&dir.x) > 0.001f) {
        vec3_normalize_into(&dir, &dir);
    }
    seg_to = *to;
    seg_from = *from;
    copy_a = copy_b = seg_to;
    for (i = 0; i < 10; i++) {
        pl_coll_slot_dist[i] = 0.0f;
        pl_coll_slot_free[i] = 0;
        pl_coll_slot_hit[i] = 0;
    }
    seen_count = 0;
    pl_coll_free_count = 0;
    pl_coll_hit_count = 0;
    sqrtf_thunk(ef_vec3_dist_sq(&seg_to, &seg_from));
    dir = seg_to - seg_from;
    mid = seg_from + (dir * 0.5f);
    if (flags & 1) {
        pl_land_grid_a_span(&seg_to, &seg_from, &count_x, &count_y, &count_z, cell, &start_x, &start_y, &start_z);
        for (x = 0; x < count_x; x++) {
            for (y = 0; y < count_y; y++) {
                for (z = 0; z < count_z; z++) {
                    grid_x = start_x + x;
                    grid_y = start_y + y;
                    grid_z = start_z + z;
                    if (pl_land_grid_a_range_ck(grid_x, grid_y, grid_z, cell) != 1) {
                        continue;
                    }
                    for (list = pl_land_grid_a_cell(grid_x, grid_y, grid_z, cell); (poly = *list) != (PlCollPoly*)-1;
                         list++) {
                        if (pl_coll_poly_usable(poly, (u32*)seen, seen_count, kind, layers, 0, area) == 0) {
                            continue;
                        }
                        pl_inter_poly_hit(&seg_to, &seg_from, poly, 1);
                        seen[seen_count++] = poly;
                        if (seen_count >= 10) {
                            seen_count = 0;
                        }
                    }
                }
            }
        }
    }
    if (flags & 2) {
        pl_land_grid_b_span(&seg_to, &seg_from, &count_x, &count_y, &count_z, cell, &start_x, &start_y, &start_z);
        for (x = 0; x < count_x; x++) {
            for (y = 0; y < count_y; y++) {
                for (z = 0; z < count_z; z++) {
                    grid_x = start_x + x;
                    grid_y = start_y + y;
                    grid_z = start_z + z;
                    if (pl_land_grid_b_range_ck(grid_x, grid_y, grid_z, cell) != 1) {
                        continue;
                    }
                    for (list = pl_land_grid_b_cell(grid_x, grid_y, grid_z, cell); (poly = *list) != (PlCollPoly*)-1;
                         list++) {
                        if (pl_coll_poly_usable(poly, (u32*)seen, seen_count, kind, layers, 0, area) == 0) {
                            continue;
                        }
                        pl_inter_poly_hit(&seg_to, &seg_from, poly, 2);
                        seen[seen_count++] = poly;
                        if (seen_count >= 10) {
                            seen_count = 0;
                        }
                    }
                }
            }
        }
    }
    if (pl_coll_hit_count != 0) {
        if (max > pl_coll_hit_count) {
            max = pl_coll_hit_count;
        }
        for (i = 0, taken = pl_hit_id_list; i < max; i++, taken++) {
            if (land != NULL) {
                land->field_0x00 = (*taken)->attr;
                land->field_0x02 = (*taken)->kind;
                land->field_0x03 = (*taken)->field_0x03;
                land->field_0x04 = (*taken)->layer;
                land->field_0x06 = (*taken)->field_0x06;
                land->field_0x07 = (*taken)->area_mask & 0xF;
                land++;
            }
        }
        *out = pl_coll_closest_1[0];
        if (grid_hit != NULL) {
            *grid_hit = pl_coll_slot_hit[0];
        }
    }
    return pl_coll_hit_count;
}

/* 0x80295EF4 (0x304): records polygon `poly` (from grid `grid`) when the segment `from`..`to` crosses it, keeping the
 * crossings sorted by their distance from `from`; 1 when recorded. */
s32 pl_inter_poly_hit(VEC3* to, VEC3* from, PlCollPoly* poly, u8 grid) {
    VEC3 point;
    VEC3 unused_a;
    VEC3 rel;
    VEC3 unused_b;
    VEC3 unused_c;
    f32 dist;
    s8 count;
    s32 i;
    s32 j;

    if (pl_coll_hit_count >= 9) {
        return 0;
    }
    rel = *from - poly->vtx0;
    if (vec3_dot(&rel.x, &poly->normal.x) < 0.0f) {
        return 0;
    }
    rel = *to - poly->vtx0;
    if (vec3_dot(&rel.x, &poly->normal.x) > 1e-05f) {
        return 0;
    }
    PlSegment seg;
    nw4r::math::PLANE plane;
    seg.start = *from;
    seg.end = *to;
    plane.N = poly->normal;
    plane.d = vec3_dot(&poly->normal.x, &poly->vtx0.x);
    if (pl_segment_plane_hit(&seg, &plane, &point) == 1 && pl_tri_point_inside(&point, &poly->vtx0, &poly->normal) != 0) {
        VEC3 sub_hit = point - seg.start;
        dist = vec3_len(&sub_hit.x);
        count = pl_coll_hit_count;
        if (count == 0) {
            pl_coll_slot_dist[count] = dist;
            pl_hit_id_list[count] = poly;
            pl_coll_closest_1[count] = point;
            pl_coll_slot_hit[pl_coll_hit_count] = grid;
        } else {
            for (i = 0; i < count; i++) {
                if (pl_coll_slot_dist[i] > dist) {
                    break;
                }
            }
            for (j = count; j > i; j--) {
                pl_coll_slot_dist[j] = pl_coll_slot_dist[j - 1];
                pl_hit_id_list[j] = pl_hit_id_list[j - 1];
                pl_coll_closest_1[j] = pl_coll_closest_1[j - 1];
                pl_coll_slot_hit[j] = pl_coll_slot_hit[j - 1];
            }
            pl_coll_slot_dist[i] = dist;
            pl_hit_id_list[i] = poly;
            pl_coll_closest_1[i] = point;
            pl_coll_slot_hit[i] = grid;
        }
        pl_coll_hit_count++;
        return 1;
    }
    return 0;
}

/* 0x802961F8 (0x30): whether `key` occurs in the first `count` words of `list`. */
extern "C" u32 pl_id_list_find(u32 key, u32* list, s32 count)
{
    u32 i;

    for (i = 0; i < count; i++) {
        if (key == list[i]) {
            return 1;
        }
    }
    return 0;
}

/* 0x80296228 (0x38): whether the ground query has already taken polygon `poly` (the first `count` entries of
 * `pl_hit_id_list`). */
extern "C" u32 pl_hit_id_listed(PlCollPoly* poly, s32 count)
{
    PlCollPoly** list = pl_hit_id_list;
    u32 i;

    for (i = 0; i < count; i++) {
        if (poly == list[i]) {
            return 1;
        }
    }
    return 0;
}

/* 0x80296260 (0x108): whether polygon `poly` passes a query's filters: attributes meeting `kind`, no layer in
 * `layers`, enabled in area `area`, and neither in `list` (`list_count` entries) nor already taken (`taken_count`). */
extern "C" u32 pl_coll_poly_usable(PlCollPoly* poly, u32* list, s32 list_count, u32 kind, u16 layers, s32 taken_count,
                                   u8 area)
{
    if (poly->attr == 0) {
        poly->attr |= 0x20;
    }
    if ((kind & poly->attr) == 0) {
        return 0;
    }
    if ((layers & poly->layer) != 0) {
        return 0;
    }
    if ((poly->area_mask & stage_area_bit_get(area)) != 0) {
        return 0;
    }
    if (list_count != 0 && pl_id_list_find((u32)poly, list, list_count) == 1) {
        return 0;
    }
    if (taken_count != 0 && pl_hit_id_listed(poly, taken_count) == 1) {
        return 0;
    }
    return 1;
}

/* Whether `pos` lies strictly inside the record's horizontal box. */
extern "C" u32 pl_land_box_ck(nw4r::math::VEC3* pos, PlLandCell* land)
{
    if (pos->x < land->grid_b.max.x && pos->x > land->grid_b.min.x && pos->z < land->grid_b.max.z &&
        pos->z > land->grid_b.min.z) {
        return 1;
    }
    return 0;
}

/* Whether a grid-B coordinate triple is inside the record's second grid. */
extern "C" s32 pl_land_grid_b_range_ck(s32 x, s32 y, s32 z, PlLandCell* land)
{
    if (x < land->grid_b.dim_x && x >= 0 && y < land->grid_b.dim_y && y >= 0 && z < land->grid_b.dim_z && z >= 0) {
        return 1;
    }
    return 0;
}

/* The same range check for the record's first grid. */
extern "C" s32 pl_land_grid_a_range_ck(s32 x, s32 y, s32 z, PlLandCell* land)
{
    if (x < land->grid_a.dim_x && x >= 0 && y < land->grid_a.dim_y && y >= 0 && z < land->grid_a.dim_z && z >= 0) {
        return 1;
    }
    return 0;
}

/* 0x80296448 (0x428): clips the point `from` against grid A's box along the line to `to` (which must lie inside the
 * box) into `out`; 1 when it was clipped, 0 when it was inside, -1 when `to` is outside. */
extern "C" s32 pl_land_grid_a_clip(nw4r::math::VEC3* out, nw4r::math::VEC3* from, nw4r::math::VEC3* to, PlLandCell* land)
{
    VEC3 dir;
    VEC3 clip;
    s32 clipped;

    VEC3 hi = land->grid_a.max;
    VEC3 lo = land->grid_a.min;
    VEC3 a;
    VEC3 b;
    a = *from;
    b = *to;
    if (b.x > hi.x || b.x < lo.x) {
        return -1;
    }
    if (b.y > hi.y || b.y < lo.y) {
        return -1;
    }
    if (b.z > hi.z || b.z < lo.z) {
        return -1;
    }
    clipped = 0;
    clip = a;
    if (clip.x < lo.x) {
        dir = clip - b;
        dir = dir * ((lo.x - b.x) / dir.x);
        clip = dir + b;
        clip.x = lo.x;
        clipped = 1;
    }
    if (clip.x > hi.x) {
        dir = clip - b;
        dir = dir * ((hi.x - b.x) / dir.x);
        clip = dir + b;
        clip.x = hi.x;
        clipped = 1;
    }
    if (clip.y < lo.y) {
        dir = clip - b;
        dir = dir * ((lo.y - b.y) / dir.y);
        clip = dir + b;
        clip.y = lo.y;
        clipped = 1;
    }
    if (clip.y > hi.y) {
        dir = clip - b;
        dir = dir * ((hi.y - b.y) / dir.y);
        clip = dir + b;
        clip.y = hi.y;
        clipped = 1;
    }
    if (clip.z < lo.z) {
        dir = clip - b;
        dir = dir * ((lo.z - b.z) / dir.z);
        clip = dir + b;
        clip.z = lo.z;
        clipped = 1;
    }
    if (clip.z > hi.z) {
        dir = clip - b;
        dir = dir * ((hi.z - b.z) / dir.z);
        clip = dir + b;
        clip.z = hi.z;
        clipped = 1;
    }
    *out = clip;
    return clipped;
}

/* 0x80296870 (0x138): the grid-B column (`*grid_x`, `*grid_z`) under `pos`; an off-grid box divides the offset from
 * its corner in floating point. */
extern "C" void pl_land_cell_index(nw4r::math::VEC3* pos, PlLandCell* land, u32* grid_x, u32* grid_z)
{
    VEC3 rel;
    s32 x;
    s32 z;
    s32 min_x;
    s32 min_z;

    if (land->grid_b.off_grid == 0) {
        x = (s32)floorf_thunk(pos->x);
        z = (s32)floorf_thunk(pos->z);
        min_x = (s32)land->grid_b.min.x;
        min_z = (s32)land->grid_b.min.z;
        x -= min_x;
        z -= min_z;
        *grid_x = x / land->grid_b.cell_x;
        *grid_z = z / land->grid_b.cell_z;
    } else {
        rel = *pos - land->grid_b.min;
        *grid_x = (s32)(rel.x / (f32)land->grid_b.cell_x);
        *grid_z = (s32)(rel.z / (f32)land->grid_b.cell_z);
    }
}

/* One cell of the record's second grid, indexed [x][y][z]. */
extern "C" PlCollPoly** pl_land_grid_b_cell(s32 x, s32 y, s32 z, PlLandCell* land)
{
    s32 index = (x * land->grid_b.dim_y + y) * land->grid_b.dim_z;
    return (PlCollPoly**)land->grid_b.cells[index + z];
}

/* One cell of the record's first grid, indexed [x][y][z]. */
extern "C" PlCollPoly** pl_land_grid_a_cell(s32 x, s32 y, s32 z, PlLandCell* land)
{
    s32 index = (x * land->grid_a.dim_y + y) * land->grid_a.dim_z;
    return (PlCollPoly**)land->grid_a.cells[index + z];
}

/* 0x802969F8 (0xCC): 1 when the x/y point `point` lies inside the flattened triangle `tri`, whichever way it winds. */
extern "C" u32 pl_tri2d_point_in(nw4r::math::VEC3* tri, nw4r::math::VEC3* point)
{
    f32 cross;
    s32 clockwise;
    s8 side;
    s32 where;

    cross = pl_tri2d_cross(&tri[0], &tri[1], &tri[2]);
    clockwise = !(cross > 0.0f);
    side = 0;
    where = pl_tri2d_point_ck(&tri[0], &tri[1], &tri[2], point, cross);
    if (where != 0) {
        if (clockwise) {
            if (where == 1) {
                side = 1;
            } else {
                side = -1;
            }
        } else {
            if (where == 2) {
                side = 1;
            } else {
                side = -1;
            }
        }
    }
    return side > 0;
}

/* 0x80296AC4 (0x7C): the z of the 2-D cross product `(b - a) x (c - a)` of three x/y points; two products one unit in
 * the last place apart count as equal. */
extern "C" f32 pl_tri2d_cross(nw4r::math::VEC3* a, nw4r::math::VEC3* b, nw4r::math::VEC3* c)
{
    f32 products[2]; /* [0] (b - a).x * (c - a).y, [1] (c - a).x * (b - a).y */
    u32 lhs_bits;
    u32 rhs_bits;

    products[0] = (b->x - a->x) * (c->y - a->y);
    products[1] = (c->x - a->x) * (b->y - a->y);
    lhs_bits = *(u32*)&products[0];
    rhs_bits = *(u32*)&products[1];
    if (lhs_bits == rhs_bits + 1) {
        products[0] = products[1];
    }
    if (lhs_bits == rhs_bits - 1) {
        products[0] = products[1];
    }
    return products[0] - products[1];
}

/* 0x80296B40 (0x140): where x/y point `point` lies against triangle `a`/`b`/`c` of winding `cross`: 0 outside (or a
 * degenerate triangle), 1 inside a clockwise one, 2 inside a counter-clockwise one. */
extern "C" u8 pl_tri2d_point_ck(nw4r::math::VEC3* a, nw4r::math::VEC3* b, nw4r::math::VEC3* c, nw4r::math::VEC3* point,
                                f32 cross)
{
    if (cross == 0.0f) {
        return 0;
    }
    if (cross < 0.0f) {
        if (pl_tri2d_cross(a, b, point) > 0.0f) {
            return 0;
        }
        if (pl_tri2d_cross(b, c, point) > 0.0f) {
            return 0;
        }
        return !(pl_tri2d_cross(c, a, point) > 0.0f);
    }
    if (pl_tri2d_cross(a, b, point) < 0.0f) {
        return 0;
    }
    if (pl_tri2d_cross(b, c, point) < 0.0f) {
        return 0;
    }
    if (pl_tri2d_cross(c, a, point) < 0.0f) {
        return 0;
    }
    return 2;
}

/* 0x80296C80 (0xE4): the unit normal of triangle `a`/`b`/`c` into `out`; 0 for a degenerate triangle. */
extern "C" u32 pl_tri_normal(nw4r::math::VEC3* a, nw4r::math::VEC3* b, nw4r::math::VEC3* c, nw4r::math::VEC3* out)
{
    VEC3 edge_ba;
    VEC3 edge_bc;

    edge_ba = *a - *b;
    edge_bc = *c - *b;
    vec3_cross(&out->x, &edge_bc.x, &edge_ba.x);
    if (vec3_length_sq(&out->x) > 0.001f) {
        vec3_normalize_into(out, out);
    }
    if (vec3_length_sq(&out->x) == 0.0f) {
        return 0;
    }
    return 1;
}

/* 0x80296D64 (0xE8): 1 when `point` lies inside the prism of triangle `tri` along `normal` (on the inner side of all
 * three edges). */
extern "C" u32 pl_tri_point_inside(nw4r::math::VEC3* point, nw4r::math::VEC3* tri, nw4r::math::VEC3* normal)
{
    VEC3 edge_normal;

    pl_tri_normal(&tri[0], &tri[1], point, &edge_normal);
    if (vec3_dot(&edge_normal.x, &normal->x) < 0.0f) {
        return 0;
    }
    pl_tri_normal(&tri[1], &tri[2], point, &edge_normal);
    if (vec3_dot(&edge_normal.x, &normal->x) < 0.0f) {
        return 0;
    }
    pl_tri_normal(&tri[2], &tri[0], point, &edge_normal);
    return !(vec3_dot(&edge_normal.x, &normal->x) < 0.0f);
}

/* 0x80296E4C (0x8C): copies `in` to `out` with every component under 0.001 in magnitude cleared. */
extern "C" void pl_vec3_snap_zero(nw4r::math::VEC3* out, nw4r::math::VEC3* in)
{
    *out = *in;
    if (abs_f32(in->x) < 0.001f) {
        out->x = 0.0f;
    }
    if (abs_f32(in->y) < 0.001f) {
        out->y = 0.0f;
    }
    if (abs_f32(in->z) < 0.001f) {
        out->z = 0.0f;
    }
}

/* 0x80296ED8 (0x1B4): adds the push `move` to the per-axis bounds `bound`, each direction capped at `limit` with the
 * excess kept in its `rest` slot. */
extern "C" void pl_push_bound_add(nw4r::math::VEC3* move, PlPushBound* bound, f32 limit)
{
    if (move->x >= 0.0f) {
        if (bound->pos.x + bound->pos_rest.x < move->x) {
            if (move->x <= limit) {
                bound->pos.x = move->x;
            } else {
                bound->pos.x = limit;
                bound->pos_rest.x = move->x - limit;
            }
        }
    } else if (bound->neg.x + bound->neg_rest.x > move->x) {
        if (-move->x <= limit) {
            bound->neg.x = move->x;
        } else {
            bound->neg.x = -limit;
            bound->neg_rest.x = move->x + limit;
        }
    }
    if (move->y >= 0.0f) {
        if (bound->pos.y + bound->pos_rest.y < move->y) {
            if (move->y <= limit) {
                bound->pos.y = move->y;
            } else {
                bound->pos.y = limit;
                bound->pos_rest.y = move->y - limit;
            }
        }
    } else if (bound->neg.y + bound->neg_rest.y > move->y) {
        if (-move->y <= limit) {
            bound->neg.y = move->y;
        } else {
            bound->neg.y = -limit;
            bound->neg_rest.y = move->y + limit;
        }
    }
    if (move->z >= 0.0f) {
        if (bound->pos.z + bound->pos_rest.z < move->z) {
            if (move->z <= limit) {
                bound->pos.z = move->z;
            } else {
                bound->pos.z = limit;
                bound->pos_rest.z = move->z - limit;
            }
        }
    } else if (bound->neg.z + bound->neg_rest.z > move->z) {
        if (-move->z <= limit) {
            bound->neg.z = move->z;
        } else {
            bound->neg.z = -limit;
            bound->neg_rest.z = move->z + limit;
        }
    }
}

/* Clears the whole table. */
extern "C" void pl_coll_table_clear(void)
{
    memset(pl_land_data, 0, 0x7F8);
}

/* 0x802970A0 (0x9C): 1 when any component of `v` has a fractional part. */
extern "C" u32 pl_vec3_fraction_ck(nw4r::math::VEC3* v)
{
    VEC3 whole;

    whole.x = floorf_thunk(v->x);
    whole.y = floorf_thunk(v->y);
    whole.z = floorf_thunk(v->z);
    if (v->x != whole.x) {
        return 1;
    }
    if (v->y != whole.y) {
        return 1;
    }
    return v->z != whole.z;
}

/* 0x8029713C (0x354): the grid-A cells the box `from`..`to` spans: the first cell in `*start_x`/`*start_y`/`*start_z` and the
 * cell counts in `*count_x`/`*count_y`/`*count_z`, capped at the grid's dimensions. */
extern "C" void pl_land_grid_a_span(nw4r::math::VEC3* from, nw4r::math::VEC3* to, s32* count_x, s32* count_y, s32* count_z,
                                  PlLandCell* land, s32* start_x, s32* start_y, s32* start_z)
{
    VEC3 rel_from;
    VEC3 rel_to;
    s32 from_x;
    s32 from_y;
    s32 from_z;
    s32 to_x;
    s32 to_y;
    s32 to_z;
    s32 min_x;
    s32 min_y;
    s32 min_z;
    s32 end_x;
    s32 end_y;
    s32 end_z;

    if (land->grid_a.off_grid == 0) {
        from_x = (s32)floorf_thunk(from->x);
        from_y = (s32)floorf_thunk(from->y);
        from_z = (s32)floorf_thunk(from->z);
        to_x = (s32)floorf_thunk(to->x);
        to_y = (s32)floorf_thunk(to->y);
        to_z = (s32)floorf_thunk(to->z);
        min_x = (s32)land->grid_a.min.x;
        min_y = (s32)land->grid_a.min.y;
        min_z = (s32)land->grid_a.min.z;
        from_x -= min_x;
        from_y -= min_y;
        from_z -= min_z;
        to_x -= min_x;
        to_y -= min_y;
        to_z -= min_z;
        *start_x = from_x / (s32)land->grid_a.cell_x;
        *start_y = from_y / (s32)land->grid_a.cell_y;
        *start_z = from_z / (s32)land->grid_a.cell_z;
        end_x = to_x / (s32)land->grid_a.cell_x;
        end_y = to_y / (s32)land->grid_a.cell_y;
        end_z = to_z / (s32)land->grid_a.cell_z;
    } else {
        rel_from = *from;
        rel_to = *to;
        vec3_sub_assign((Vec*)&rel_from, (Vec*)&land->grid_a.min);
        vec3_sub_assign((Vec*)&rel_to, (Vec*)&land->grid_a.min);
        *start_x = (s32)(rel_from.x / (f32)land->grid_a.cell_x);
        *start_y = (s32)(rel_from.y / (f32)land->grid_a.cell_y);
        *start_z = (s32)(rel_from.z / (f32)land->grid_a.cell_z);
        end_x = (s32)(rel_from.x / (f32)land->grid_a.cell_x);
        end_y = (s32)(rel_from.y / (f32)land->grid_a.cell_y);
        end_z = (s32)(rel_from.z / (f32)land->grid_a.cell_z);
    }
    end_x -= *start_x;
    end_y -= *start_y;
    end_z -= *start_z;
    if (end_x < 0) {
        *start_x += end_x;
        end_x -= end_x * 2;
    }
    if (end_y < 0) {
        *start_y += end_y;
        end_y -= end_y * 2;
    }
    if (end_z < 0) {
        *start_z += end_z;
        end_z -= end_z * 2;
    }
    *count_x = end_x + 1;
    *count_y = end_y + 1;
    *count_z = end_z + 1;
    if (land->grid_a.dim_x < *count_x) {
        *count_x = land->grid_a.dim_x;
    }
    if (land->grid_a.dim_y < *count_y) {
        *count_y = land->grid_a.dim_y;
    }
    if (land->grid_a.dim_z < *count_z) {
        *count_z = land->grid_a.dim_x;
    }
}

/* 0x80297490 (0x354): the grid-B cells the box `from`..`to` spans: the first cell in `*start_x`/`*start_y`/`*start_z` and the
 * cell counts in `*count_x`/`*count_y`/`*count_z`, capped at the grid's dimensions. */
extern "C" void pl_land_grid_b_span(nw4r::math::VEC3* from, nw4r::math::VEC3* to, s32* count_x, s32* count_y, s32* count_z,
                                  PlLandCell* land, s32* start_x, s32* start_y, s32* start_z)
{
    VEC3 rel_from;
    VEC3 rel_to;
    s32 from_x;
    s32 from_y;
    s32 from_z;
    s32 to_x;
    s32 to_y;
    s32 to_z;
    s32 min_x;
    s32 min_y;
    s32 min_z;
    s32 end_x;
    s32 end_y;
    s32 end_z;

    if (land->grid_b.off_grid == 0) {
        from_x = (s32)floorf_thunk(from->x);
        from_y = (s32)floorf_thunk(from->y);
        from_z = (s32)floorf_thunk(from->z);
        to_x = (s32)floorf_thunk(to->x);
        to_y = (s32)floorf_thunk(to->y);
        to_z = (s32)floorf_thunk(to->z);
        min_x = (s32)land->grid_b.min.x;
        min_y = (s32)land->grid_b.min.y;
        min_z = (s32)land->grid_b.min.z;
        from_x -= min_x;
        from_y -= min_y;
        from_z -= min_z;
        to_x -= min_x;
        to_y -= min_y;
        to_z -= min_z;
        *start_x = from_x / (s32)land->grid_b.cell_x;
        *start_y = from_y / (s32)land->grid_b.cell_y;
        *start_z = from_z / (s32)land->grid_b.cell_z;
        end_x = to_x / (s32)land->grid_b.cell_x;
        end_y = to_y / (s32)land->grid_b.cell_y;
        end_z = to_z / (s32)land->grid_b.cell_z;
    } else {
        rel_from = *from;
        rel_to = *to;
        vec3_sub_assign((Vec*)&rel_from, (Vec*)&land->grid_b.min);
        vec3_sub_assign((Vec*)&rel_to, (Vec*)&land->grid_b.min);
        *start_x = (s32)(rel_from.x / (f32)land->grid_b.cell_x);
        *start_y = (s32)(rel_from.y / (f32)land->grid_b.cell_y);
        *start_z = (s32)(rel_from.z / (f32)land->grid_b.cell_z);
        end_x = (s32)(rel_from.x / (f32)land->grid_b.cell_x);
        end_y = (s32)(rel_from.y / (f32)land->grid_b.cell_y);
        end_z = (s32)(rel_from.z / (f32)land->grid_b.cell_z);
    }
    end_x -= *start_x;
    end_y -= *start_y;
    end_z -= *start_z;
    if (end_x < 0) {
        *start_x += end_x;
        end_x -= end_x * 2;
    }
    if (end_y < 0) {
        *start_y += end_y;
        end_y -= end_y * 2;
    }
    if (end_z < 0) {
        *start_z += end_z;
        end_z -= end_z * 2;
    }
    *count_x = end_x + 1;
    *count_y = end_y + 1;
    *count_z = end_z + 1;
    if (land->grid_b.dim_x < *count_x) {
        *count_x = land->grid_b.dim_x;
    }
    if (land->grid_b.dim_y < *count_y) {
        *count_y = land->grid_b.dim_y;
    }
    if (land->grid_b.dim_z < *count_z) {
        *count_z = land->grid_b.dim_x;
    }
}

/* Clears the header fields and points the record's vector up by one. */
extern "C" void pl_land_data_reset(LandData* self)
{
    self->field_0x00 = 0;
    self->field_0x02 = 0;
    self->field_0x03 = 0;
    self->field_0x04 = 0;
    self->field_0x06 = 0;
    self->field_0x07 = 0;
    setVector3(&self->vec_0x08, 0.0f, 1.0f, 0.0f);
}

/* Copies one land record field by field (the hit normal through `VEC3::operator=`). */
inline void pl_land_set(LandData* dst, LandData* src) {
    dst->field_0x00 = src->field_0x00;
    dst->field_0x02 = src->field_0x02;
    dst->field_0x03 = src->field_0x03;
    dst->field_0x04 = src->field_0x04;
    dst->field_0x06 = src->field_0x06;
    dst->field_0x07 = src->field_0x07;
    dst->vec_0x08 = src->vec_0x08;
}

/* 0x80297814 (0x3D0): picks the ground among `count` candidates (`lands`/`heights`): the highest at most 50 above
 * `pos`, else the lowest; the pick goes to `out` and `*height`. */
extern "C" void pl_ground_select(LandData* out, nw4r::math::VEC3* pos, LandData* lands, f32* heights, f32* height,
                                 s8 count)
{
    LandData near[5];
    f32 near_heights[5];
    s32 found;
    s8 i;
    s8 kept;

    found = 0;
    kept = 0;
    if (count != 1) {
        for (i = 0; i < count; i++) {
            if (heights[i] <= 50.0f + pos->y) {
                found = 1;
                pl_land_copy(&near[kept], &lands[i]);
                near_heights[kept++] = heights[i];
            }
        }
        if (found == 1) {
            if (kept == 1) {
                *height = near_heights[0];
                pl_land_set(out, &near[0]);
            } else {
                for (i = 0; i < kept; i++) {
                    if (i == 0) {
                        heights[0] = near_heights[i];
                        pl_land_set(&lands[0], &near[i]);
                    } else if (heights[0] < near_heights[i]) {
                        heights[0] = near_heights[i];
                        pl_land_set(&lands[0], &near[i]);
                    }
                }
                *height = heights[0];
                pl_land_set(out, &lands[0]);
            }
        } else {
            for (i = 0; i < count; i++) {
                if (i == 0) {
                    near_heights[0] = heights[i];
                    pl_land_set(&near[0], &lands[i]);
                } else if (near_heights[0] > heights[i]) {
                    near_heights[0] = heights[i];
                    pl_land_set(&near[0], &lands[i]);
                }
            }
            *height = near_heights[0];
            pl_land_set(out, &near[0]);
        }
    } else {
        *height = heights[0];
        pl_land_set(out, &lands[0]);
    }
}

/* A position record as a plain record (copied field by field, the vector word by word).  size: 0x14 */
struct LandDataImage {
    /* +0x00 */ u16 field_0x00;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u16 field_0x04;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ nw4r::math::_VEC3 vec_0x08;
};

/* Copies one position record. */
extern "C" void pl_land_copy(LandData* dst, LandData* src)
{
    *(LandDataImage*)dst = *(LandDataImage*)src;
}

/* The unit's data, in address order, after every body.  `__sinit` constructs the arrays and builds the two
 * body-shape lists' centres in this order. */
#pragma pool_data off
PlMoveEntry pl_move_work[4][11];
s8 pl_coll_slot_free[0x10];
PlCollPoly pl_hit_box[0x258 / 0x3C];
nw4r::math::VEC3 pl_coll_closest_0[0x78 / 0xC];
nw4r::math::VEC3 pl_coll_closest_1[0x78 / 0xC];
nw4r::math::VEC3 pl_coll_closest_2[0x78 / 0xC];
HitSphere pl_coll_body_shape[2] = {
    {nw4r::math::VEC3(0.0f, 60.0f, 0.0f), 48.0f},
    {nw4r::math::VEC3(0.0f, 0.0f, 0.0f), -1.0f},
};
HitSphere pl_coll_body_shape_alt[2] = {
    {nw4r::math::VEC3(0.0f, 40.0f, 0.0f), 60.0f},
    {nw4r::math::VEC3(0.0f, 0.0f, 0.0f), -1.0f},
};
u8 pl_coll_shared_vtx_table[8] = {0xFF, 0xFF, 0xFF, 2, 0xFF, 1, 0, 0xFF};
f32 pl_coll_slot_dist[0x28 / 4];
u8 pl_coll_slot_hit[0x0C];
s8 pl_coll_slot_kind[0x0C];
s8 pl_coll_slot_used[0x10];
PlLandCell pl_land_data[0x7F8 / 0x88];
PlCollPoly* pl_hit_id_list[0x28 / 4];
