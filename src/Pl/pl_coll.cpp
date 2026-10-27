/*
 * Pl/pl_coll.cpp - the player/enemy collision band: the sphere, capsule, segment and triangle tests, the stage land
 *   grids (ground, wall, line and sweep queries over the polygons of `pl_land_data`), the wall resolver and the
 *   body-shape sweeps that feed the move work.
 * RANGE. .text 0x8028F66C-0x80297E34 (81 functions, in address order below); .ctors 0x8056F370, .data
 *   0x805CDC48-0x805CDC88, .bss 0x806AB848-0x806AC8A8 (`Pl/pl_coll.h`'s arrays), .sdata 0x80792270-0x80792278,
 *   .sbss 0x80794B58-0x80794B60, .sdata2 0x8079A318-0x8079A380, extab, extabindex.
 * SEAM. Two literal pools: `.sdata2` 0x8079A310-0x8079A32C (0.5, 0.001, 0, 1, 1e-5, -1, 10.01) is read by
 *   `Pl/pl_hit_sphere.cpp` and this unit's functions up to `pl_land_grid_a_load` (0x8029163C), and a second pool
 *   from 0x8079A330 (0, 50, 1, -0, 0.5, 0.001, ... the 0x4330000000000000 double) by `GetGroundHit` (0x8029163C) on;
 *   the repeated values mean two TUs: 0x8028F44C-0x8029163C (with `Pl/pl_hit_sphere.cpp`) and 0x8029163C-0x80297E34.
 * FLAGS. The `Pl` library's settled set (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on -fp_contract off`).
 * NAMES. `GetGroundHit`, `GetGroundHit2`, `hit_ground_comon` and `findInterSection*` are the map's (mangled) names;
 *   every other function is a C-linkage name derived from its body.  GUESS (from each body and its callers): every
 *   `pl_*`/`em_coll_*` function name, `vec3_scale_inv`, `vec3_scale_to`, `coll_wall_slide`, `coll_line_check`,
 *   `npc_ground_check`, the data names `pl_coll_hit_count`, `pl_coll_free_count`, `pl_coll_shared_vtx_table`,
 *   `pl_coll_body_shape(_alt)`, and the registry names `floorf_thunk`, `sqrtf_thunk`, `stage_area_bit_get`,
 *   `calc_xz_angle`.
 * RESIDUALS.
 *  - `fn_80297C30` (0x16C), the static initialiser, is unwritten: it constructs the `.bss` arrays with
 *    `__construct_array` (the element constructors are the C-named `pl_move_entry_ctor`/`pl_hit_box_ctor`/`VEC3_ctor`/
 *    `pl_land_cell_ctor`) and fills the body-shape centres (0,60,0)/(0,40,0) through `setVec3`+`assignVec3`; MWCC only
 *    emits that from classes whose constructors are those functions, so the `.ctors` word and the centres are missing.
 *  - `GetGroundHit2`, `hit_ground_comon`: the five-hit cap leaves both loops with one branch in retail (`goto`-shaped);
 *    the conformant double `break` costs three instructions in each (100 % with a `goto`, refused by rule 8).
 *  - `pl_wall_resolve` (0x802929DC): register colouring and the order of the shared-corner loop's constants; the
 *    resolver spills `pos` to the stack where ours keeps it in a register.
 *  - `pl_coll_shape_hits`, `em_coll_shape_hits`: the yaw `(u16)calc_xz_angle(...) - yaw` loads the yaw before the
 *    mask in retail; `pl_coll_shape_hits` also computes the swap entry's address differently.
 *  - `pl_tri2d_cross`: retail keeps both products in memory (reloaded after each store); ours forwards registers.
 *  - `pl_land_grid_a/b_load`, `pl_land_grid_a/b_cell`: the cell index's final `add` has its operands swapped.
 *  - register colouring only: `coll_wall_slide`, `coll_line_check`, `pl_coll_sweep_ck`, `findInterSection3`,
 *    `pl_wall_slot_push`.
 *  - Data: `.data`, `.sdata` and `.sbss` are defined here; the `.bss` arrays stay declared in `Pl/pl_coll.h` (defined
 *    here, MWCC addresses them from one section base where retail relocates each by name: 4 rows dropped to 61/81);
 *    the `.sdata2` pool differs (the first pool belongs with `Pl/pl_hit_sphere.cpp`, see SEAM); `.ctors` is missing
 *    (the initialiser above), and the body-shape centres stay zero without it.
 *  - Consumer declarations that disagree with the definitions (C linkage, so the code is unaffected): `coll_wall_slide`
 *    (`Pl/coll_wall_slide.h`, `ai/fn_802D0F34.h`: no radius), `pl_ground_probe` (`Pl/pl_segment_sphere_hit.h`: seven
 *    arguments), `pl_coll_tri_hit_ck` (`Pl/pl_coll_tri_hit_ck.h`: radius last), `npc_ground_check`, `pl_coll_shape_set`.
 */

/* ==== 0x8028F66C-0x80295EF4: the collision queries ==== */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "pl.h"              /* the Pl module header; pulls in `ef.h`'s `VEC3_ctor` */
#include "fn_8004CAD8.h"     /* the vector helpers this unit calls (rule 2) */
#include "ef/fn_800AEE48.h"  /* `vec3_sub_assign` (rule 2) */
#include "Pl/fn_80288CEC.h"
#include "Pl/fn_8028F66C.h"  /* `LandData` and the record initialisers this unit calls (rule 2) */
#include "Pl/pl_hit_sphere.h" /* the capsule boxes and `hit_point_sphr` (rule 2) */
#include "ef/effect.h"    /* `eft_vec_ctor` (rule 2) */
#include "Runtime.PPCEABI.H/memset.h"
#include "ef/fn_800CDB2C.h"   /* PlayMode_ck (rule 2: the owner is `ef/system_core.cpp`) */
#include "menu/hit_attack_list_push.h"
#include "Pl/pl_coll.h"   /* this unit's `.bss` arrays and record types */
#include "stage/stg_w.h" /* `get_now_areano` (rule 2) */
#include "stage/stage_area_bit_get.h"
#include "g3d/floorf_thunk.h"
#include "sqrtf_thunk.h"
#include "ef/ef_vec3_dist_sq.h"
#include "g3d/g3d_calcworld.h" /* `addVec3To` (rule 2) */
#include "g3d/g3d_draw.h"      /* `vec3_negate` (rule 2) */
#include "g3d/vec3_ctor_return.h"
#include "ef/eft_capsule_ctor.h"
#include "ef/pl_segment_init.h"
#include "ef/pl_segment_box_init.h"
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
 * caller picked (`coll_wall_slide`'s `mode`: 0 neither, 1 both, 2 `slide` only).  `pl_vec3_pair_ctor` constructs
 * the two ends.  size: 0x24 */
struct PlSlideWork {
    /* +0x00 */ nw4r::math::VEC3 start;
    /* +0x0C */ nw4r::math::VEC3 end;
    /* +0x18 */ f32 radius;
    /* +0x1C */ f32 length;
    /* +0x20 */ u16 slide;              /* the resolver lets the move slide along the walls */
    /* +0x22 */ u16 skip_below;         /* polygons wholly below the query point are ignored */
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
s32 pl_sphere_contact(HitSphere* a, HitSphere* b, VEC3* out);
u32 pl_sphere_push(HitSphere* a, HitSphere* b, VEC3* out);
f32 pl_clampf(f32 value, f32 low, f32 high);
f32 pl_box_point_dist_sq(PlBox* box, const VEC3* point, f32* param);
f32 pl_segment_closest_point(VEC3* start, VEC3* end, VEC3* point, VEC3* out);
f32 pl_segment_closest_sq(PlBox* a, PlBox* b, f32* s_out, f32* t_out);
s32 pl_capsule_capsule_hit(PlCapsuleBox* a, PlCapsuleBox* b, VEC3* on_b, VEC3* on_a, VEC3* normal, f32* depth);
s32 pl_capsule_capsule_contact(PlCapsuleBox* a, PlCapsuleBox* b, VEC3* out);
u32 pl_capsule_capsule_push(PlCapsuleBox* a, PlCapsuleBox* b, VEC3* push_a, VEC3* push_b);
s32 pl_capsule_sphere_hit(PlCapsuleBox* a, HitSphere* sphere, VEC3* on_axis, VEC3* normal, f32* depth);
s32 pl_capsule_sphere_contact(PlCapsuleBox* a, HitSphere* sphere, VEC3* out);
u32 pl_capsule_sphere_push(PlCapsuleBox* a, HitSphere* sphere, VEC3* push_a, VEC3* push_sphere);
s32 pl_sphere_capsule_contact(HitSphere* sphere, PlCapsuleBox* a, VEC3* out);
void pl_plane_project(VEC3* point, PlPlane* plane, VEC3* out);
void vec3_scale_to(VEC3* out, f32 scale, const VEC3* in);
u32 pl_coll_tri_hit_ck(VEC3* pos, f32 radius, VEC3* a, VEC3* b, VEC3* c, f32* height);
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
PlMoveEntry* pl_move_entry_ctor(PlMoveEntry* self);
void pl_coll_shape_hits(PlSegment* seg, f32 radius, u32 kind, u16 layers, _PLW* self);
void em_coll_shape_hits(PlSegment* seg, f32 radius, u32 kind, u16 layers, _ENEMY_WORK* self);
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
VEC3* pl_vec3_pair_ctor(VEC3* pair);
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
s32 fn_80291B08(_PLW* self, VEC3* pos, LandData* land, f32* out, u32 kind);
s32 stage_water_surface_get(VEC3* pos, u8 flag, u16* hit_layer, f32* out, u32 kind);
void pl_area_ground_probe(VEC3* pos, u8 flag, LandData* land, u32 kind);
void vec3_scale_inv(VEC3* v, f32 divisor);
VEC3* pl_vec3_ctor_ret(VEC3* v);
s32 pl_segment_sphere_hit(PlSegmentBox* seg, HitSphere* sphere, VEC3* enter, VEC3* exit);
s32 pl_segment_capsule_hit(PlSegmentBox* seg, PlCapsuleBox* capsule, VEC3* enter, VEC3* exit);
u32 pl_segment_plane_hit(PlSegment* seg, PlPlane* plane, VEC3* out);
s32 pl_triangle_closest_point(VEC3* point, VEC3* a, VEC3* b, VEC3* c, VEC3* out);
s32 coll_wall_slide(VEC3* to, VEC3* from, f32 radius, VEC3* pos, u32 kind, u16 layers, u8 mode, u8 area);
s32 coll_line_check(VEC3* to, VEC3* from, f32 radius, VEC3* hit, u8 flags, u32 kind, u8 area, u16 layers);
void pl_coll_shape_set(_PLW* self, u8 mode, u16 layers);
void em_coll_shape_set(_ENEMY_WORK* self, u8 mode, u16 layers);
}

HitSphere pl_coll_body_shape[2] = {
    {{0.0f, 0.0f, 0.0f}, 48.0f},
    {{0.0f, 0.0f, 0.0f}, -1.0f},
};
HitSphere pl_coll_body_shape_alt[2] = {
    {{0.0f, 0.0f, 0.0f}, 60.0f},
    {{0.0f, 0.0f, 0.0f}, -1.0f},
};
u8 pl_coll_shared_vtx_table[8] = {0xFF, 0xFF, 0xFF, 2, 0xFF, 1, 0, 0xFF};
s8 pl_coll_hit_count;
s8 pl_coll_free_count;

/* ------------------------------------------------------------------------------------------------ *
 * Bodies, in address order.
 * ------------------------------------------------------------------------------------------------ */

/* 0x8028F66C - place `out` on sphere `b`'s surface toward sphere `a` when the two overlap. */
s32 pl_sphere_contact(HitSphere* a, HitSphere* b, VEC3* out) {
    VEC3 sep;
    f32 dist;
    f32 reach;

    VEC3_ctor(&sep);
    dist = vec3_dist_sq(a, b);
    reach = a->radius + b->radius;
    if (dist <= reach * reach) {
        if (dist > 0.001f) {
            PSVECSubtract(&sep.x, &a->pos.x, &b->pos.x);
            vec3_scale_by(&sep.x, &sep.x, b->radius / sqrt_f32(dist));
            vec3_add_ps(out, &sep, &b->pos);
        } else {
            vec3_copy(out, &b->pos);
        }
        return 1;
    }
    return 0;
}

/* 0x8028F758 - the same overlap test, but `out` gets the penetration vector: the direction `a - b`
 * scaled by `(radius_a + radius_b - |a - b|) / |a - b|`, or zero when they touch. */
u32 pl_sphere_push(HitSphere* a, HitSphere* b, VEC3* out) {
    VEC3 sep;
    f32 dist;
    f32 reach;

    VEC3_ctor(&sep);
    dist = vec3_dist_sq(a, b);
    reach = a->radius + b->radius;
    if (dist <= reach * reach) {
        if (dist > 0.001f) {
            f32 mag;
            f32 ratio;

            PSVECSubtract(&sep.x, &a->pos.x, &b->pos.x);
            mag = sqrt_f32(dist);
            ratio = (reach - mag) / mag;
            vec3_scale_by(&out->x, &sep.x, ratio);
        } else {
            out->x = 0.0f;
            out->y = 0.0f;
            out->z = 0.0f;
        }
        return 1;
    }
    return 0;
}

/* 0x8028F84C - clamp `value` into `[low, high]`. */
f32 pl_clampf(f32 value, f32 low, f32 high) {
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

/* 0x8028F86C - the parametric position of `point` along `box`'s axis in `*param` (clamped to 0..1),
 * and the squared distance from `point` to that position. */
f32 pl_box_point_dist_sq(PlBox* box, const VEC3* point, f32* param) {
    VEC3 sep;
    f32 axis_len2;
    f32 t;

    subVec3(&sep, point, &box->vec_0x00);
    axis_len2 = vec3_length_sq(&box->vec_0x18.x);
    t = 0.0f;
    if (axis_len2 >= 0.001f) {
        VEC3 scaled;

        t = pl_clampf(vec3_dot(&sep.x, &box->vec_0x18.x) / axis_len2, 0.0f, 1.0f);
        vec3_scale(&scaled, &box->vec_0x18, t);
        /* `vec3_sub_assign` is declared with the `ef` module's `Vec`, a distinct 0xC record with the same
         * layout (`ef.h`); the conversion is a view, not arithmetic. */
        vec3_sub_assign((Vec*)&sep, (Vec*)&scaled);
    }
    *param = t;
    return vec3_length_sq(&sep.x);
}

/* 0x8028F938 - build the box `start` -> `end`, then hand `out` the point on it closest to `point`;
 * the return is that point's squared distance from `point`. */
f32 pl_segment_closest_point(VEC3* start, VEC3* end, VEC3* point, VEC3* out) {
    PlBox box;
    VEC3 diff;
    VEC3 pos;
    VEC3 scaled;
    f32 param;
    f32 dist;

    em_pl_box_init(&box);
    param = 0.0f;
    copyVec3(&box.vec_0x00, start);
    copyVec3(&box.vec_0x0C, end);
    subVec3(&diff, end, start);
    copyVec3(&box.vec_0x18, &diff);
    dist = pl_box_point_dist_sq(&box, point, &param);
    vec3_scale(&scaled, &box.vec_0x18, param);
    addVec3(&pos, &scaled, start);
    copyVec3(out, &pos);
    return dist;
}

/* 0x8028FA20 (0x29C): the closest points of segment boxes `a` and `b` as parameters `*s_out`/`*t_out` along their
 * axes; returns the squared distance between the two points. */
f32 pl_segment_closest_sq(PlBox* a, PlBox* b, f32* s_out, f32* t_out) {
    VEC3 rel;
    VEC3 pt_a;
    VEC3 pt_b;
    VEC3 sep;
    VEC3 step_a;
    VEC3 step_b;
    f32 len_a;
    f32 len_b;
    f32 dot_b;
    f32 dot_a;
    f32 dot_ab;
    f32 denom;
    f32 s;
    f32 t;

    subVec3(&rel, &a->vec_0x00, &b->vec_0x00);
    len_a = vec3_length_sq(&a->vec_0x18.x);
    len_b = vec3_length_sq(&b->vec_0x18.x);
    dot_b = vec3_dot(&b->vec_0x18.x, &rel.x);
    if (len_a <= 0.001f && len_b <= 0.001f) {
        *s_out = *t_out = 0.0f;
        return vec3_length_sq(&rel.x);
    }
    if (len_a <= 0.001f) {
        s = 0.0f;
        t = pl_clampf(dot_b / len_b, s, 1.0f);
    } else {
        dot_a = vec3_dot(&a->vec_0x18.x, &rel.x);
        if (len_b <= 0.001f) {
            t = 0.0f;
            s = pl_clampf(-dot_a / len_a, t, 1.0f);
        } else {
            dot_ab = vec3_dot(&a->vec_0x18.x, &b->vec_0x18.x);
            denom = len_a * len_b - dot_ab * dot_ab;
            if (denom != 0.0f) {
                s = pl_clampf((dot_ab * dot_b - dot_a * len_b) / denom, 0.0f, 1.0f);
            } else {
                s = 0.0f;
            }
            t = (dot_b + dot_ab * s) / len_b;
            if (t < 0.0f) {
                t = 0.0f;
                s = pl_clampf(-dot_a / len_a, 0.0f, 1.0f);
            } else if (t > 1.0f) {
                t = 1.0f;
                s = pl_clampf((dot_ab - dot_a) / len_a, 0.0f, 1.0f);
            }
        }
    }
    *s_out = s;
    *t_out = t;
    vec3_scale(&step_a, &a->vec_0x18, s);
    addVec3(&pt_a, &a->vec_0x00, &step_a);
    vec3_scale(&step_b, &b->vec_0x18, t);
    addVec3(&pt_b, &b->vec_0x00, &step_b);
    subVec3(&sep, &pt_b, &pt_a);
    return vec3_length_sq(&sep.x);
}

/* 0x8028FCBC (0x280): capsule `a` against capsule `b`: the closest point on each (`on_b`, `on_a`), the unit normal from
 * `a` toward `b` and the signed gap `*depth`; returns 1 while they overlap. */
s32 pl_capsule_capsule_hit(PlCapsuleBox* a, PlCapsuleBox* b, VEC3* on_b, VEC3* on_a, VEC3* normal, f32* depth) {
    PlBox box_a;
    PlBox box_b;
    VEC3 up;
    VEC3 fwd;
    VEC3 pos_a;
    VEC3 step_a;
    VEC3 pos_b;
    VEC3 step_b;
    VEC3 sep;
    f32 s;
    f32 t;
    f32 dist;
    f32 len;
    f32 gap;

    s = 0.0f;
    t = 0.0f;
    em_pl_box_init(&box_a);
    em_pl_box_init(&box_b);
    if (hit_point_sphr(&a->center, &b->center, a->reach + b->reach) == 0) {
        return 0;
    }
    copyVec3(&box_a.vec_0x00, &a->start);
    copyVec3(&box_a.vec_0x0C, &a->end);
    copyVec3(&box_a.vec_0x18, &a->dir);
    copyVec3(&box_b.vec_0x00, &b->start);
    copyVec3(&box_b.vec_0x0C, &b->end);
    copyVec3(&box_b.vec_0x18, &b->dir);
    dist = pl_segment_closest_sq(&box_a, &box_b, &s, &t);
    len = 0.0f;
    vec3_scale(&step_a, &box_a.vec_0x18, s);
    addVec3(&pos_a, &box_a.vec_0x00, &step_a);
    copyVec3(on_a, &pos_a);
    vec3_scale(&step_b, &box_b.vec_0x18, t);
    addVec3(&pos_b, &box_b.vec_0x00, &step_b);
    copyVec3(on_b, &pos_b);
    subVec3(&sep, on_b, on_a);
    copyVec3(normal, &sep);
    if (dist < 1e-05f) {
        vec3_cross(&normal->x, &a->dir.x, &b->dir.x);
        if (vec3_length_sq(&normal->x) < 1e-05f) {
            VEC3_ctor(&up);
            setVector3(&up, 0.0f, 1.0f, 0.0f);
            vec3_cross(&normal->x, &a->dir.x, &up.x);
            if (vec3_length_sq(&normal->x) < 1e-05f) {
                VEC3_ctor(&fwd);
                setVector3(&fwd, 0.0f, 0.0f, 1.0f);
                vec3_cross(&normal->x, &a->dir.x, &fwd.x);
            }
        }
    } else {
        len = sqrt_f32(dist);
    }
    gap = len - (a->radius + b->radius);
    *depth = gap;
    if (vec3_length_sq(&normal->x) > 0.001f) {
        vec3_normalize_into(normal, normal);
    }
    if (gap < 0.0f) {
        return 1;
    }
    return 0;
}

/* 0x8028FF3C (0xC4): the contact point of two overlapping capsules: `b`'s closest point pulled back along the normal by
 * `b`'s radius. */
s32 pl_capsule_capsule_contact(PlCapsuleBox* a, PlCapsuleBox* b, VEC3* out) {
    VEC3 normal;
    VEC3 on_b;
    VEC3 on_a;
    VEC3 pos;
    VEC3 back;
    VEC3 step;
    f32 depth;

    VEC3_ctor(&normal);
    VEC3_ctor(&on_b);
    VEC3_ctor(&on_a);
    if (pl_capsule_capsule_hit(a, b, &on_b, &on_a, &normal, &depth) != 0) {
        vec3_scale(&step, &normal, b->radius);
        vec3_scale(&back, &step, -1.0f);
        addVec3(&pos, &on_b, &back);
        copyVec3(out, &pos);
        return 1;
    }
    return 0;
}

/* 0x80290000 (0xF8): the push-out vectors of two overlapping capsules: `push_a` moves `a` out of `b`, `push_b` moves `b`
 * out of `a`. */
u32 pl_capsule_capsule_push(PlCapsuleBox* a, PlCapsuleBox* b, VEC3* push_a, VEC3* push_b) {
    VEC3 normal;
    VEC3 on_b;
    VEC3 on_a;
    VEC3 unused_a;
    VEC3 unused_b;
    VEC3 back;
    VEC3 step;
    VEC3 fwd;
    f32 depth;

    VEC3_ctor(&normal);
    VEC3_ctor(&on_b);
    VEC3_ctor(&on_a);
    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    if (pl_capsule_capsule_hit(a, b, &on_b, &on_a, &normal, &depth) != 0) {
        depth = abs_f32(depth);
        vec3_scale(&step, &normal, depth);
        vec3_scale(&back, &step, -1.0f);
        copyVec3(push_a, &back);
        vec3_scale(&fwd, &normal, depth);
        copyVec3(push_b, &fwd);
        return 1;
    }
    return 0;
}

/* 0x802900F8 (0x1F8): capsule `a` against sphere `sphere`: the closest point on the capsule axis, the unit normal toward
 * the sphere and the signed gap `*depth`; returns 1 while they overlap. */
s32 pl_capsule_sphere_hit(PlCapsuleBox* a, HitSphere* sphere, VEC3* on_axis, VEC3* normal, f32* depth) {
    PlBox box;
    VEC3 side;
    VEC3 up;
    VEC3 pos;
    VEC3 step;
    VEC3 sep;
    f32 t;
    f32 dist;
    f32 len;
    f32 gap;

    t = 0.0f;
    em_pl_box_init(&box);
    if (hit_point_sphr(&a->center, &sphere->pos, a->reach + sphere->radius) == 0) {
        return 0;
    }
    copyVec3(&box.vec_0x00, &a->start);
    copyVec3(&box.vec_0x0C, &a->end);
    copyVec3(&box.vec_0x18, &a->dir);
    dist = pl_box_point_dist_sq(&box, &sphere->pos, &t);
    len = 0.0f;
    vec3_scale(&step, &box.vec_0x18, t);
    addVec3(&pos, &step, &box.vec_0x00);
    copyVec3(on_axis, &pos);
    subVec3(&sep, &sphere->pos, on_axis);
    copyVec3(normal, &sep);
    if (dist < 1e-05f) {
        VEC3_ctor(&side);
        setVector3(&side, 1.0f, 0.0f, 0.0f);
        vec3_cross(&normal->x, &a->dir.x, &side.x);
        if (vec3_length_sq(&normal->x) < 1e-05f) {
            VEC3_ctor(&up);
            setVector3(&up, 0.0f, 1.0f, 0.0f);
            vec3_cross(&normal->x, &a->dir.x, &up.x);
        }
    } else {
        len = sqrt_f32(dist);
    }
    gap = len - (a->radius + sphere->radius);
    *depth = gap;
    if (vec3_length_sq(&normal->x) > 0.001f) {
        vec3_normalize_into(normal, normal);
    }
    if (gap < 0.0f) {
        return 1;
    }
    return 0;
}

/* 0x802902F0 (0xA8): the contact point of an overlapping capsule and sphere: the sphere's centre pulled back along the
 * normal by its radius. */
s32 pl_capsule_sphere_contact(PlCapsuleBox* a, HitSphere* sphere, VEC3* out) {
    VEC3 normal;
    VEC3 on_axis;
    VEC3 pos;
    VEC3 step;
    f32 depth;

    VEC3_ctor(&normal);
    VEC3_ctor(&on_axis);
    if (pl_capsule_sphere_hit(a, sphere, &on_axis, &normal, &depth) != 0) {
        vec3_scale(&step, &normal, sphere->radius);
        subVec3(&pos, &sphere->pos, &step);
        copyVec3(out, &pos);
        return 1;
    }
    return 0;
}

/* 0x80290398 (0xE4): the push-out vectors of an overlapping capsule and sphere: `push_a` moves the capsule out,
 * `push_sphere` moves the sphere out. */
u32 pl_capsule_sphere_push(PlCapsuleBox* a, HitSphere* sphere, VEC3* push_a, VEC3* push_sphere) {
    VEC3 normal;
    VEC3 on_axis;
    VEC3 unused_a;
    VEC3 unused_b;
    VEC3 fwd;
    VEC3 back;
    f32 depth;

    VEC3_ctor(&normal);
    VEC3_ctor(&on_axis);
    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    if (pl_capsule_sphere_hit(a, sphere, &on_axis, &normal, &depth) != 0) {
        depth = abs_f32(depth);
        vec3_scale(&fwd, &normal, depth);
        copyVec3(push_sphere, &fwd);
        vec3_scale(&back, &normal, -1.0f * depth);
        copyVec3(push_a, &back);
        return 1;
    }
    return 0;
}

/* 0x8029047C (0xA8): the contact point of sphere `sphere` against capsule `a`: the closest axis point pushed out along
 * the normal by the capsule's radius. */
s32 pl_sphere_capsule_contact(HitSphere* sphere, PlCapsuleBox* a, VEC3* out) {
    VEC3 normal;
    VEC3 on_axis;
    VEC3 pos;
    VEC3 step;
    f32 depth;

    VEC3_ctor(&normal);
    VEC3_ctor(&on_axis);
    if (pl_capsule_sphere_hit(a, sphere, &on_axis, &normal, &depth) != 0) {
        vec3_scale(&step, &normal, a->radius);
        addVec3(&pos, &on_axis, &step);
        copyVec3(out, &pos);
        return 1;
    }
    return 0;
}

/* 0x80290524 (0x74): projects `point` onto `plane` into `out`. */
void pl_plane_project(VEC3* point, PlPlane* plane, VEC3* out) {
    VEC3 pos;
    VEC3 step;

    vec3_scale(&step, &plane->normal, vec3_dot(&point->x, &plane->normal.x) - plane->dist);
    subVec3(&pos, point, &step);
    copyVec3(out, &pos);
}

/* 0x80290598 (0x224): segment box `seg` against sphere `sphere`: 0 for a miss, else 1 plus 2 when the segment enters the
 * sphere (the entry point in `enter`) and 2 more when it leaves it (the exit point in `exit`); an endpoint inside the
 * sphere is copied out instead. */
s32 pl_segment_sphere_hit(PlSegmentBox* seg, HitSphere* sphere, VEC3* enter, VEC3* exit) {
    VEC3 nearest;
    VEC3 dir;
    PlBox box;
    VEC3 pos;
    VEC3 step;
    VEC3 enter_pos;
    VEC3 enter_step;
    VEC3 exit_pos;
    VEC3 exit_step;
    f32 t;
    f32 dist;
    f32 radius_sq;
    s32 start_in;
    s32 end_in;
    s32 result;

    VEC3_ctor(&nearest);
    VEC3_ctor(&dir);
    em_pl_box_init(&box);
    if (hit_point_sphr(&seg->center, &sphere->pos, sphere->radius + seg->half_length) == 0) {
        return 0;
    }
    copyVec3(&box.vec_0x00, &seg->start);
    copyVec3(&box.vec_0x0C, &seg->end);
    copyVec3(&box.vec_0x18, &seg->dir);
    dist = pl_box_point_dist_sq(&box, &sphere->pos, &t);
    radius_sq = sphere->radius * sphere->radius;
    if (dist > radius_sq) {
        return 0;
    }
    vec3_normalize_into(&dir, &seg->dir);
    vec3_scale(&step, &seg->dir, t);
    addVec3(&pos, &seg->start, &step);
    copyVec3(&nearest, &pos);
    sqrt_f32(radius_sq - dist);
    start_in = hit_point_sphr(&seg->start, &sphere->pos, sphere->radius);
    end_in = hit_point_sphr(&seg->end, &sphere->pos, sphere->radius);
    dist = sqrt_f32(dist);
    dist += 10.01f;
    result = 1;
    if (start_in == 0) {
        if (enter != NULL) {
            vec3_scale(&enter_step, &dir, -dist);
            addVec3(&enter_pos, &nearest, &enter_step);
            copyVec3(enter, &enter_pos);
            result = 2;
        }
    } else if (enter != NULL) {
        copyVec3(enter, &seg->start);
    }
    if (end_in == 0) {
        if (exit != NULL) {
            vec3_scale(&exit_step, &dir, dist);
            addVec3(&exit_pos, &nearest, &exit_step);
            copyVec3(exit, &exit_pos);
            result += 2;
        }
    } else if (exit != NULL) {
        copyVec3(exit, &seg->end);
    }
    return result;
}

/* 0x802907BC (0x84): 1 when `point` lies inside capsule `capsule`. */
u32 pl_capsule_point_ck(VEC3* point, PlCapsuleBox* capsule) {
    PlBox box;
    f32 t;

    em_pl_box_init(&box);
    copyVec3(&box.vec_0x00, &capsule->start);
    copyVec3(&box.vec_0x0C, &capsule->end);
    copyVec3(&box.vec_0x18, &capsule->dir);
    return pl_box_point_dist_sq(&box, point, &t) < capsule->radius * capsule->radius;
}

/* 0x80290840 (0x3E8): segment box `seg` against capsule `capsule`: 0 for a miss, else 1 with the points where the
 * segment enters and leaves the capsule in `enter`/`exit` (either may be null). */
s32 pl_segment_capsule_hit(PlSegmentBox* seg, PlCapsuleBox* capsule, VEC3* enter, VEC3* exit) {
    PlBox box_seg;
    PlBox box_cap;
    VEC3 on_seg;
    VEC3 on_cap;
    VEC3 normal;
    VEC3 up;
    VEC3 fwd;
    HitSphere cap_end;
    VEC3 side;
    VEC3 across;
    VEC3 step_seg;
    VEC3 step_cap;
    VEC3 enter_pos;
    VEC3 enter_step;
    VEC3 exit_pos;
    VEC3 exit_step;
    f32 s;
    f32 t;
    f32 dist;
    f32 len;
    f32 cos_a;
    s32 start_in;
    s32 end_in;

    s = 0.0f;
    t = 0.0f;
    em_pl_box_init(&box_seg);
    em_pl_box_init(&box_cap);
    if (hit_point_sphr(&seg->center, &capsule->center, seg->half_length + capsule->reach) == 0) {
        return 0;
    }
    copyVec3(&box_seg.vec_0x00, &seg->start);
    copyVec3(&box_seg.vec_0x0C, &seg->end);
    copyVec3(&box_seg.vec_0x18, &seg->dir);
    copyVec3(&box_cap.vec_0x00, &capsule->start);
    copyVec3(&box_cap.vec_0x0C, &capsule->end);
    copyVec3(&box_cap.vec_0x18, &capsule->dir);
    dist = pl_segment_closest_sq(&box_seg, &box_cap, &s, &t);
    len = 0.0f;
    vec3_scale(&step_seg, &box_seg.vec_0x18, s);
    addVec3(&on_seg, &box_seg.vec_0x00, &step_seg);
    vec3_scale(&step_cap, &box_cap.vec_0x18, t);
    addVec3(&on_cap, &box_cap.vec_0x00, &step_cap);
    subVec3(&normal, &on_cap, &on_seg);
    if (dist < 1e-05f) {
        vec3_cross(&normal.x, &seg->dir.x, &capsule->dir.x);
        if (vec3_length_sq(&normal.x) < 1e-05f) {
            VEC3_ctor(&up);
            setVector3(&up, 0.0f, 1.0f, 0.0f);
            vec3_cross(&normal.x, &seg->dir.x, &up.x);
            if (vec3_length_sq(&normal.x) < 1e-05f) {
                VEC3_ctor(&fwd);
                setVector3(&fwd, 0.0f, 0.0f, 1.0f);
                vec3_cross(&normal.x, &seg->dir.x, &fwd.x);
            }
        }
    } else {
        len = sqrt_f32(dist);
    }
    len -= capsule->radius;
    if (vec3_length_sq(&normal.x) > 0.001f) {
        vec3_normalize_into(&normal, &normal);
    }
    if (len > 0.0f) {
        return 0;
    }
    start_in = pl_capsule_point_ck(&seg->start, capsule);
    end_in = pl_capsule_point_ck(&seg->end, capsule);
    if (start_in == 0 || end_in == 0) {
        if (t > 1.0f || t < 0.0f) {
            eft_vec_ctor(&cap_end.pos);
            copyVec3(&cap_end.pos, &on_cap);
            cap_end.radius = capsule->radius;
            pl_segment_sphere_hit(seg, &cap_end, enter, exit);
        } else if (enter != NULL || exit != NULL) {
            dist = sqrt_f32(capsule->radius * capsule->radius - dist);
            VEC3_ctor(&side);
            VEC3_ctor(&across);
            vec3_cross(&side.x, &seg->dir.x, &capsule->dir.x);
            vec3_cross(&across.x, &side.x, &capsule->dir.x);
            vec3_normalize_into(&normal, &seg->dir);
            if (vec3_length_sq(&across.x) > 0.001f) {
                vec3_normalize_into(&across, &across);
            }
            cos_a = vec3_dot(&normal.x, &across.x);
            if (enter != NULL) {
                vec3_scale(&enter_step, &normal, dist / cos_a);
                addVec3(&enter_pos, &on_seg, &enter_step);
                copyVec3(enter, &enter_pos);
            }
            if (exit != NULL) {
                vec3_scale(&exit_step, &normal, dist / cos_a);
                subVec3(&exit_pos, &on_seg, &exit_step);
                copyVec3(exit, &exit_pos);
            }
        }
    } else {
        if (enter != NULL) {
            copyVec3(enter, &seg->start);
        }
        if (exit != NULL) {
            copyVec3(exit, &seg->end);
        }
    }
    return 1;
}

/* 0x80290C28 (0xE0): where segment `seg` crosses `plane`, into `out`; 0 when it does not. */
u32 pl_segment_plane_hit(PlSegment* seg, PlPlane* plane, VEC3* out) {
    VEC3 dir;
    VEC3 pos;
    VEC3 step;
    f32 denom;
    f32 t;

    subVec3(&dir, &seg->end, &seg->start);
    denom = vec3_dot(&plane->normal.x, &dir.x);
    t = (plane->dist - vec3_dot(&plane->normal.x, &seg->start.x)) / denom;
    if (t >= 0.0f && t <= 1.0f) {
        vec3_scale(&step, &dir, t);
        addVec3(&pos, &seg->start, &step);
        copyVec3(out, &pos);
        return 1;
    }
    return 0;
}

/* 0x80290D08 (0x3B8): the point of triangle `a`/`b`/`c` closest to `point`, into `out`; returns the region it lies in
 * (0-2 a vertex, 3-5 an edge, 6 the face). */
s32 pl_triangle_closest_point(VEC3* point, VEC3* a, VEC3* b, VEC3* c, VEC3* out) {
    VEC3 ab;
    VEC3 ac;
    VEC3 ap;
    VEC3 bp;
    VEC3 cp;
    VEC3 pos_ab;
    VEC3 step_ab;
    VEC3 pos_ac;
    VEC3 step_ac;
    VEC3 pos_bc;
    VEC3 step_bc;
    VEC3 bc;
    VEC3 pos_face;
    VEC3 pos_edge;
    VEC3 face_ab;
    VEC3 face_ac;
    f32 d1;
    f32 d2;
    f32 d3;
    f32 d4;
    f32 d5;
    f32 d6;
    f32 vb;
    f32 va;
    f32 vc;
    f32 denom;
    f32 v;
    f32 w;

    subVec3(&ab, b, a);
    subVec3(&ac, c, a);
    subVec3(&ap, point, a);
    d1 = vec3_dot(&ab.x, &ap.x);
    d2 = vec3_dot(&ac.x, &ap.x);
    if (d1 <= 0.0f && d2 <= 0.0f) {
        copyVec3(out, a);
        return 0;
    }
    subVec3(&bp, point, b);
    d3 = vec3_dot(&ab.x, &bp.x);
    d4 = vec3_dot(&ac.x, &bp.x);
    if (d3 >= 0.0f && d4 <= d3) {
        copyVec3(out, b);
        return 1;
    }
    vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        v = d1 / (d1 - d3);
        vec3_scale(&step_ab, &ab, v);
        addVec3(&pos_ab, a, &step_ab);
        copyVec3(out, &pos_ab);
        return 3;
    }
    subVec3(&cp, point, c);
    d5 = vec3_dot(&ab.x, &cp.x);
    d6 = vec3_dot(&ac.x, &cp.x);
    if (d6 >= 0.0f && d5 <= d6) {
        copyVec3(out, c);
        return 2;
    }
    vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        w = d2 / (d2 - d6);
        vec3_scale(&step_ac, &ac, w);
        addVec3(&pos_ac, a, &step_ac);
        copyVec3(out, &pos_ac);
        return 4;
    }
    va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && d4 - d3 >= 0.0f && d5 - d6 >= 0.0f) {
        w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        subVec3(&bc, c, b);
        vec3_scale_to(&step_bc, w, &bc);
        addVec3(&pos_bc, b, &step_bc);
        copyVec3(out, &pos_bc);
        return 5;
    }
    denom = 1.0f / (va + vb + vc);
    v = vb * denom;
    w = vc * denom;
    vec3_scale(&face_ac, &ac, w);
    vec3_scale(&face_ab, &ab, v);
    addVec3(&pos_edge, a, &face_ab);
    addVec3(&pos_face, &pos_edge, &face_ac);
    copyVec3(out, &pos_face);
    return 6;
}

/* 0x802910C0 (0x54): `out = in * scale`, constructing `out` first. */
void vec3_scale_to(VEC3* out, f32 scale, const VEC3* in) {
    VEC3_ctor(out);
    vec3_scale_by(&out->x, &in->x, scale);
}

/* 0x80291114 (0x248): 1 when `pos` (with the vertical reach `radius`) crosses the plane of triangle `a`/`b`/`c` inside
 * its x/z footprint; `*height` receives the plane's height under `pos`. */
u32 pl_coll_tri_hit_ck(VEC3* pos, f32 radius, VEC3* a, VEC3* b, VEC3* c, f32* height) {
    VEC3 edge_ab;
    VEC3 edge_ca;
    VEC3 normal;
    VEC3 flat_a;
    VEC3 flat_b;
    VEC3 flat_c;
    VEC3 flat_pos;
    VEC3 sub_ab;
    VEC3 sub_ca;
    f32 d;
    f32 h;

    VEC3_ctor(&edge_ab);
    VEC3_ctor(&edge_ca);
    VEC3_ctor(&normal);
    VEC3_ctor(&flat_a);
    VEC3_ctor(&flat_b);
    VEC3_ctor(&flat_c);
    VEC3_ctor(&flat_pos);
    flat_a.x = a->x;
    flat_a.y = a->z;
    flat_a.z = 0.0f;
    flat_b.x = b->x;
    flat_b.y = b->z;
    flat_b.z = 0.0f;
    flat_c.x = c->x;
    flat_c.y = c->z;
    flat_c.z = 0.0f;
    flat_pos.x = pos->x;
    flat_pos.y = pos->z;
    flat_pos.z = 0.0f;
    if (pl_tri2d_point_ck(&flat_a, &flat_b, &flat_c, &flat_pos, pl_tri2d_cross(&flat_a, &flat_b, &flat_c)) == 0) {
        return 0;
    }
    subVec3(&sub_ab, a, b);
    copyVec3(&edge_ab, &sub_ab);
    subVec3(&sub_ca, c, a);
    copyVec3(&edge_ca, &sub_ca);
    vec3_cross(&normal.x, &edge_ab.x, &edge_ca.x);
    if (ef_fabsf(normal.y) < 0.001f) {
        return 0;
    }
    if (vec3_length_sq(&normal.x)) {
        vec3_normalize_into(&normal, &normal);
    }
    d = -1.0f * vec3_dot(&a->x, &normal.x);
    h = -(d + (normal.x * pos->x + normal.z * pos->z)) / normal.y;
    *height = h;
    if (radius >= 0.0f) {
        if (pos->y + radius >= h && pos->y <= h) {
            return 1;
        }
    } else {
        if (pos->y + radius <= h && pos->y >= h) {
            return 1;
        }
    }
    return 0;
}

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
    copyVec3(&cell->grid_b.min, &src->min);
    copyVec3(&cell->grid_b.max, &src->max);
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
                poly = (u32*)(cell->grid_b.cells[(y + cell->grid_b.dim_y * x) * cell->grid_b.dim_z + z] += (u32)src);
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
    copyVec3(&cell->grid_a.min, &src->min);
    copyVec3(&cell->grid_a.max, &src->max);
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
                poly = (u32*)(cell->grid_a.cells[(y + cell->grid_a.dim_y * x) * cell->grid_a.dim_z + z] += (u32)src);
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
    VEC3 tri[3];
    f32 heights[5];
    f32 below[5];
    VEC3 flat_pos;
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
        VEC3_ctor(&flat_pos);
        VEC3_ctor(&tri[0]);
        VEC3_ctor(&tri[1]);
        VEC3_ctor(&tri[2]);
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
    fn_802977E4(land);
    return hit_ground_comon(pos, self->area_0x16, land, out, kind, 2, 0, 0, 0);
}

/* 0x80291A70 (0x98): the enemy's ground query: resets `land` and the ceiling record, then asks for the floor,
 * surface, extra and ceiling heights in the enemy's area, skipping layer 4. */
s32 pl_ground_probe(_ENEMY_WORK* self, VEC3* pos, LandData* land, f32* out, f32* surface, f32* extra,
                    _HIT_TENJO_DATA* tenjo, u32 kind) {
    fn_802977E4(land);
    if (tenjo != NULL) {
        tenjo->hit = 0;
        tenjo->kind = 0;
        tenjo->height = 0.0f;
    }
    return hit_ground_comon(pos, self->area_no, land, out, kind, 4, surface, extra, tenjo);
}

/* 0x80291B08 - the fixed-layer ground query: forwards the caller's `LandData`/out/kind and the
 * `_PLW`'s effect key, asking for all 32 layers and no surface data. */
s32 fn_80291B08(_PLW* self, VEC3* pos, LandData* land, f32* out, u32 kind) {
    return hit_ground_comon(pos, self->effect_key_0x1A4, land, out, kind, 32, 0, 0, 0);
}

/* 0x80291B48 (0x74): the NPC's ground query: resets `land` and asks for the floor in the NPC's area, every layer. */
s32 npc_ground_check(_PLW* self, VEC3* pos, LandData* land, f32* out, u32 kind) {
    fn_802977E4(land);
    return hit_ground_comon(pos, self->field_0x004, land, out, kind, 0, 0, 0, 0);
}

/* 0x80291BBC - the single-layer probe: builds a scratch `LandData` for the query and reports both
 * the query's verdict and the layer it hit. */
s32 stage_water_surface_get(VEC3* pos, u8 flag, u16* hit_layer, f32* out, u32 kind) {
    LandData land;
    s32 found;

    em_land_data_init(&land);
    fn_802977E4(&land);
    *hit_layer = 0;
    found = hit_ground_comon(pos, flag, &land, out, kind, 1, 0, 0, 0);
    *hit_layer = land.field_0x00;
    return found == 1;
}

/* 0x80291C50 - the kind-only query: no layer mask, no surface data. */
void pl_area_ground_probe(VEC3* pos, u8 flag, LandData* land, u32 kind) {
    f32 out;

    fn_802977E4(land);
    hit_ground_comon(pos, flag, land, &out, kind, 0, 0, 0, 0);
}

/* 0x80291CD0 (0x3BC): the ground query under `pos` in area `area` (0xFF, or game mode 2, the current one): of the
 * polygons meeting `kind` and not excluded by `layers`, a surface, extra or ceiling polygon fills its own output and
 * the floors (up to five) go to `pl_ground_select`, which fills `land` and `*out`.  -1 when the area has no land
 * data, 0 when no floor is found (outside the grid `*out` is `pos->y`), else 1. */
s32 hit_ground_comon(nw4r::math::VEC3* pos, u8 area, LandData* land, f32* out, u32 kind, u16 layers, f32* surface,
                     f32* extra, _HIT_TENJO_DATA* tenjo) {
    LandData lands[5];
    VEC3 tri[3];
    f32 heights[5];
    VEC3 flat_pos;
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

    em_land_data_init(&lands[0]);
    em_land_data_init(&lands[1]);
    em_land_data_init(&lands[2]);
    em_land_data_init(&lands[3]);
    em_land_data_init(&lands[4]);
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
        VEC3_ctor(&flat_pos);
        VEC3_ctor(&tri[0]);
        VEC3_ctor(&tri[1]);
        VEC3_ctor(&tri[2]);
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
                copyVec3(&land_at->vec_0x08, &poly->normal);
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
    PlSlideWork work;
    PlCollPoly* seen[10];
    VEC3 clipped;
    VEC3 mid;
    VEC3 dir;
    VEC3 lo;
    VEC3 hi;
    VEC3 sub;
    VEC3 mid_sum;
    VEC3 half;
    VEC3 lo_sum;
    VEC3 lo_step;
    VEC3 hi_sum;
    VEC3 hi_step;
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

    VEC3_ctor(&clipped);
    VEC3_ctor(&mid);
    VEC3_ctor(&dir);
    VEC3_ctor(&lo);
    VEC3_ctor(&hi);
    pl_vec3_pair_ctor(&work.start);
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
    copyVec3(&work.start, from);
    copyVec3(&work.end, to);
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
    work.length = sqrtf_thunk(ef_vec3_dist_sq(&work.start, &work.end));
    subVec3(&sub, &work.start, &work.end);
    copyVec3(&dir, &sub);
    vec3_scale(&half, &dir, 0.5f);
    addVec3(&mid_sum, &work.end, &half);
    copyVec3(&mid, &mid_sum);
    if (vec3_length_sq(&dir.x) > 0.001f) {
        vec3_normalize_into(&dir, &dir);
    }
    vec3_scale(&lo_step, &dir, radius);
    addVec3(&lo_sum, from, &lo_step);
    copyVec3(&lo, &lo_sum);
    vec3_scale(&hi_step, &dir, -radius);
    addVec3(&hi_sum, to, &hi_step);
    copyVec3(&hi, &hi_sum);
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

/* 0x80292468 - construct both vectors of a two-vector pair and return it. */
VEC3* pl_vec3_pair_ctor(VEC3* pair) {
    VEC3* p;

    p = pair;
    do {
        VEC3_ctor(p);
        p++;
    } while (p < pair + 2);
    return pair;
}

/* 0x802924C0 (0x480): records polygon `poly` in the next hit-box slot when the swept sphere of `work` touches it:
 * the slot keeps the polygon, the sweep start, the touch point (and, for an edge touch, the plane point), and the
 * side/crossing flags; 1 when recorded. */
s32 pl_wall_poly_hit(PlSlideWork* work, PlCollPoly* poly, VEC3* pos) {
    VEC3 touch;
    VEC3 rel;
    VEC3 unused;
    VEC3* edges[4];
    VEC3 edge_best;
    VEC3 on_plane;
    VEC3 probe;
    VEC3 sub_end;
    VEC3 sub_start;
    VEC3 step;
    VEC3 back;
    VEC3 sub_seg;
    VEC3 cross_step;
    VEC3 cross_sum;
    f32 dist;
    f32 best;
    f32 dd;
    s8 flags;
    VEC3** edge;
    s32 i;

    VEC3_ctor(&touch);
    VEC3_ctor(&rel);
    VEC3_ctor(&unused);
    if (pl_coll_hit_count >= 9) {
        return 0;
    }
    if (work->skip_below == 1 && pos->y >= poly->vtx0.y && pos->y >= poly->vtx1.y && pos->y >= poly->vtx2.y) {
        return 0;
    }
    subVec3(&sub_end, &work->end, &poly->vtx0);
    copyVec3(&rel, &sub_end);
    if (vec3_dot(&rel.x, &poly->normal.x) < 0.0f) {
        return 0;
    }
    subVec3(&sub_start, &work->start, &poly->vtx0);
    copyVec3(&rel, &sub_start);
    dist = vec3_dot(&rel.x, &poly->normal.x);
    flags = !(dist > 0.0f);
    if (dist > work->radius) {
        return 0;
    }
    if (dist >= -work->radius) {
        vec3_scale(&step, &poly->normal, dist);
        copyVec3(&touch, &step);
        subVec3(&back, &work->start, &touch);
        copyVec3(&touch, &back);
    } else {
        subVec3(&sub_seg, &work->start, &work->end);
        copyVec3(&rel, &sub_seg);
        dd = vec3_dot(&rel.x, &poly->normal.x);
        if (dd == 0.0f) {
            return 0;
        }
        vec3_scale(&cross_step, &rel, -(dist / dd));
        copyVec3(&rel, &cross_step);
        addVec3(&cross_sum, &work->start, &rel);
        copyVec3(&touch, &cross_sum);
        flags |= 2;
    }
    if (pl_tri_point_inside(&touch, &poly->vtx0, &poly->normal) != 0) {
        pl_hit_box_set(&pl_hit_box[pl_coll_hit_count], poly);
        pl_vec3_snap_zero(&pl_hit_box[pl_coll_hit_count].normal, &pl_hit_box[pl_coll_hit_count].normal);
        copyVec3(&pl_coll_closest_0[pl_coll_hit_count], &work->start);
        pl_coll_slot_kind[pl_coll_hit_count] = flags;
        pl_coll_slot_used[pl_coll_hit_count] = 0;
        copyVec3(&pl_coll_closest_1[pl_coll_hit_count], &touch);
        pl_coll_hit_count++;
        return 1;
    }
    pl_coll_slot_used[pl_coll_hit_count] = 1;
    VEC3_ctor(&edge_best);
    VEC3_ctor(&on_plane);
    VEC3_ctor(&probe);
    copyVec3(&on_plane, &touch);
    edges[0] = &poly->vtx0;
    edges[1] = &poly->vtx1;
    edges[2] = &poly->vtx2;
    edges[3] = &poly->vtx0;
    edge = edges;
    best = 8000.0f;
    if ((flags & 2) == 0) {
        copyVec3(&probe, &work->start);
    } else {
        copyVec3(&probe, &on_plane);
    }
    for (i = 3; i > 0; i--, edge++) {
        dd = sqrt_f32(pl_segment_closest_point(edge[0], edge[1], &probe, &touch));
        if (dd < best) {
            best = dd;
            copyVec3(&edge_best, &touch);
        }
    }
    if (best < work->radius) {
        pl_hit_box_set(&pl_hit_box[pl_coll_hit_count], poly);
        pl_vec3_snap_zero(&pl_hit_box[pl_coll_hit_count].normal, &pl_hit_box[pl_coll_hit_count].normal);
        copyVec3(&pl_coll_closest_0[pl_coll_hit_count], &work->start);
        pl_coll_slot_kind[pl_coll_hit_count] = flags;
        copyVec3(&pl_coll_closest_1[pl_coll_hit_count], &edge_best);
        copyVec3(&pl_coll_closest_2[pl_coll_hit_count], &on_plane);
        pl_coll_hit_count++;
        return 1;
    }
    return 0;
}

/* 0x80292940 (0x9C): copies one polygon into a hit box. */
void pl_hit_box_set(PlCollPoly* dst, PlCollPoly* src) {
    *dst = *src;
}

/* 0x802929DC (0xB28): resolves the hit boxes the sweep recorded into one push of `pos`: face hits push along their
 * normals, edge hits are dropped when a face hit shares their plane or a corner they lie behind, the per-axis
 * pushes are merged and the total is capped at 1.8 sweep lengths; returns the number of hits. */
s32 pl_wall_resolve(PlSlideWork* work, VEC3* pos, u8* flag) {
    PlPushBound bound;
    VEC3 push;
    s8 faces[10];
    s8 edges[10];
    s8 drop_kind[10];
    s8 dropped[10];
    VEC3 face_rel;
    VEC3 corner_rel;
    VEC3 sub_face;
    VEC3 sub_corner;
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

    VEC3_ctor(&push);
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
                addVec3To(pos, &push);
                pl_coll_slot_free[pl_coll_free_count++] = 0;
            }
        } else {
            VEC3_ctor(&bound.pos);
            VEC3_ctor(&bound.neg);
            VEC3_ctor(&bound.pos_rest);
            VEC3_ctor(&bound.neg_rest);
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
                    VEC3_ctor(&face_rel);
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
                    subVec3(&sub_face, corner, first);
                    copyVec3(&face_rel, &sub_face);
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
                        VEC3_ctor(&corner_rel);
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
                        subVec3(&sub_corner, corner, first);
                        copyVec3(&corner_rel, &sub_corner);
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
            addVec3To(pos, &push);
        }
    }
    return pl_coll_hit_count;
}

/* 0x80293504 (0x578): the push hit-box `slot` asks for: a face hit pushes the sweep end out along the normal (when
 * sliding, flattened and kept off the face's edges), an edge hit pushes away from the touch point; 0 when the push
 * is zero.  `*flag` is set for a crossing hit. */
s8 pl_wall_slot_push(PlSlideWork* work, VEC3* push, s8 slot, u8* flag) {
    PlBox unused_box;
    VEC3 normal;
    VEC3 offset;
    VEC3 point;
    VEC3 flat;
    VEC3 probe;
    VEC3 nearest;
    VEC3 start;
    VEC3 scaled;
    VEC3 sum_touch;
    VEC3 sub_push;
    VEC3 sum_probe;
    VEC3 sub_near;
    VEC3 sub_probe;
    VEC3 back;
    VEC3 sub_c0;
    VEC3 sub_c2;
    VEC3 scaled_edge;
    VEC3 sum_edge;
    VEC3 sub_edge;
    VEC3 sub_flat;
    VEC3 sub_rel;
    VEC3 sum_rel;
    PlCollPoly* box;
    nw4r::math::VEC3* touch;
    nw4r::math::VEC3* from;
    f32 len;
    f32 dy;
    f32 side;

    em_pl_box_init(&unused_box);
    VEC3_ctor(&normal);
    VEC3_ctor(&offset);
    VEC3_ctor(&point);
    VEC3_ctor(&flat);
    VEC3_ctor(&probe);
    VEC3_ctor(&nearest);
    if (pl_coll_slot_used[slot] != 1) {
        box = &pl_hit_box[slot];
        copyVec3(&normal, &box->normal);
        vec3_scale(&scaled, &normal, work->radius);
        copyVec3(&offset, &scaled);
        addVec3(&sum_touch, &pl_coll_closest_1[slot], &offset);
        copyVec3(&point, &sum_touch);
        from = &pl_coll_closest_0[slot];
        subVec3(&sub_push, &point, from);
        copyVec3(push, &sub_push);
        if (work->slide == 1) {
            push->y = 0.0f;
            if (abs_f32(normal.x) >= 0.001f || abs_f32(normal.z) >= 0.001f) {
                normal.y = 0.0f;
                if ((len = vec3_length_sq(&normal.x)) > 0.001f) {
                    vec3_scale_inv(push, len);
                    addVec3(&sum_probe, from, push);
                    copyVec3(&probe, &sum_probe);
                    if ((u8)pl_triangle_closest_point(&probe, &box->vtx0, &box->vtx1, &box->vtx2, &nearest) != 6) {
                        subVec3(&sub_near, &nearest, from);
                        copyVec3(&offset, &sub_near);
                        if (offset.x * normal.x + offset.z * normal.z > 0.0f) {
                            subVec3(&sub_probe, &probe, &nearest);
                            copyVec3(&offset, &sub_probe);
                            if (offset.x * normal.x + offset.z * normal.z > 0.0f) {
                                if (vec3_length_sq(&offset.x) > work->radius * work->radius) {
                                    dy = nearest.y - probe.y;
                                    vec3_scale_to(&back, sqrt_f32(work->radius * work->radius - dy * dy), &normal);
                                    vec3_sub_assign((Vec*)push, (Vec*)&back);
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
            subVec3(&sub_c0, &pl_coll_closest_0[slot], touch);
            copyVec3(&point, &sub_c0);
        } else {
            touch = &pl_coll_closest_1[slot];
            subVec3(&sub_c2, &pl_coll_closest_2[slot], touch);
            copyVec3(&point, &sub_c2);
        }
        if (work->slide == 0) {
            copyVec3(&normal, &point);
            if (vec3_length_sq(&normal.x) > 0.001f) {
                vec3_normalize_into(&normal, &normal);
            } else {
                addVec3To(&normal, &pl_hit_box[slot].normal);
            }
            vec3_scale(&scaled_edge, &normal, work->radius);
            copyVec3(&offset, &scaled_edge);
            addVec3(&sum_edge, touch, &offset);
            copyVec3(&point, &sum_edge);
            VEC3_ctor(&start);
            setVector3(&start, pl_coll_closest_0[slot].x, pl_coll_closest_0[slot].y, pl_coll_closest_0[slot].z);
            subVec3(&sub_edge, &point, &start);
            copyVec3(&offset, &sub_edge);
        } else {
            copyVec3(&flat, &point);
            copyVec3(&normal, &flat);
            normal.y = 0.0f;
            if (vec3_length_sq(&normal.x) > 0.001f) {
                vec3_normalize_into(&normal, &normal);
            } else {
                copyVec3(&normal, &pl_hit_box[slot].normal);
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
                subVec3(&sub_flat, &point, &flat);
                copyVec3(&offset, &sub_flat);
            } else {
                addVec3(&sum_rel, &point, touch);
                subVec3(&sub_rel, &sum_rel, &pl_coll_closest_0[slot]);
                copyVec3(&offset, &sub_rel);
            }
            offset.y = 0.0f;
        }
        if (vec3_len(&offset.x) == 0.0f) {
            return 0;
        }
        copyVec3(push, &offset);
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
    PlSlideWork work;
    PlCollPoly* seen[10];
    VEC3 lo_copy;
    VEC3 lo_copy2;
    VEC3 lo;
    VEC3 hi;
    VEC3 dir;
    VEC3 mid;
    VEC3 sub;
    VEC3 lo_sum;
    VEC3 lo_step;
    VEC3 hi_sum;
    VEC3 hi_step;
    VEC3 seg;
    VEC3 mid_sum;
    VEC3 half;
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

    VEC3_ctor(&lo_copy);
    VEC3_ctor(&lo_copy2);
    pl_vec3_pair_ctor(&work.start);
    collect = hit != NULL;
    if (area == 0xFF) {
        area = get_now_areano();
    }
    cell = &pl_land_data[area];
    VEC3_ctor(&lo);
    VEC3_ctor(&hi);
    VEC3_ctor(&dir);
    VEC3_ctor(&mid);
    if (cell->loaded_b == 0 && cell->loaded_a == 0) {
        return -1;
    }
    subVec3(&sub, to, from);
    copyVec3(&dir, &sub);
    if (vec3_length_sq(&dir.x) > 0.001f) {
        vec3_normalize_into(&dir, &dir);
    }
    vec3_scale(&lo_step, &dir, radius);
    addVec3(&lo_sum, to, &lo_step);
    copyVec3(&lo, &lo_sum);
    vec3_scale(&hi_step, &dir, -radius);
    addVec3(&hi_sum, from, &hi_step);
    copyVec3(&hi, &hi_sum);
    copyVec3(&lo_copy, &lo);
    copyVec3(&lo_copy2, &lo);
    for (i = 0; i < 10; i++) {
        seen[i] = 0;
        pl_coll_slot_dist[i] = 0.0f;
        pl_coll_slot_free[i] = 0;
    }
    seen_count = 0;
    taken_count = 0;
    pl_coll_free_count = 0;
    pl_coll_hit_count = 0;
    copyVec3(&work.start, to);
    copyVec3(&work.end, from);
    work.radius = radius;
    work.slide = 0;
    work.skip_below = 0;
    work.length = sqrtf_thunk(ef_vec3_dist_sq(&work.start, &work.end));
    subVec3(&seg, &work.start, &work.end);
    copyVec3(&dir, &seg);
    vec3_scale(&half, &dir, 0.5f);
    addVec3(&mid_sum, &work.end, &half);
    copyVec3(&mid, &mid_sum);
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
    PlSlideWork work;
    PlCollPoly* seen[10];
    VEC3 lo_copy;
    VEC3 lo_copy2;
    VEC3 lo;
    VEC3 hi;
    VEC3 dir;
    VEC3 mid;
    VEC3 sub;
    VEC3 lo_sum;
    VEC3 lo_step;
    VEC3 hi_sum;
    VEC3 hi_step;
    VEC3 seg;
    VEC3 mid_sum;
    VEC3 half;
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

    VEC3_ctor(&lo_copy);
    VEC3_ctor(&lo_copy2);
    pl_vec3_pair_ctor(&work.start);
    if (area == 0xFF) {
        area = get_now_areano();
    }
    cell = &pl_land_data[area];
    VEC3_ctor(&lo);
    VEC3_ctor(&hi);
    VEC3_ctor(&dir);
    VEC3_ctor(&mid);
    if (cell->loaded_b == 0 && cell->loaded_a == 0) {
        return 0;
    }
    subVec3(&sub, to, from);
    copyVec3(&dir, &sub);
    if (vec3_length_sq(&dir.x) > 0.001f) {
        vec3_normalize_into(&dir, &dir);
    }
    vec3_scale(&lo_step, &dir, radius);
    addVec3(&lo_sum, to, &lo_step);
    copyVec3(&lo, &lo_sum);
    vec3_scale(&hi_step, &dir, -radius);
    addVec3(&hi_sum, from, &hi_step);
    copyVec3(&hi, &hi_sum);
    copyVec3(&lo_copy, &lo);
    copyVec3(&lo_copy2, &lo);
    for (i = 0; i < 10; i++) {
        seen[i] = 0;
        pl_coll_slot_dist[i] = 0.0f;
        pl_coll_slot_free[i] = 0;
    }
    pl_coll_free_count = 0;
    pl_coll_hit_count = 0;
    copyVec3(&work.start, to);
    copyVec3(&work.end, from);
    work.radius = radius;
    work.slide = 0;
    work.skip_below = 0;
    work.length = sqrtf_thunk(ef_vec3_dist_sq(&work.start, &work.end));
    subVec3(&seg, &work.start, &work.end);
    copyVec3(&dir, &seg);
    vec3_scale(&half, &dir, 0.5f);
    addVec3(&mid_sum, &work.end, &half);
    copyVec3(&mid, &mid_sum);
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
    VEC3 corners[4];
    PlSegmentBox sweep_box;
    PlCapsule edge;
    PlSegment seg;
    PlPlane plane;
    VEC3 point;
    VEC3 unused_a;
    VEC3 rel;
    VEC3 unused_b;
    VEC3 back;
    VEC3 dir;
    VEC3 travel;
    VEC3 best_seg;
    VEC3 best_cap;
    VEC3 on_seg;
    VEC3 on_cap;
    VEC3 unused_c;
    VEC3 unused_d;
    VEC3 best_dir;
    VEC3 rev;
    VEC3 half;
    VEC3 sub_end;
    VEC3 sub_start;
    VEC3 sum_end;
    VEC3 step_end;
    VEC3 sum_start;
    VEC3 step_start;
    VEC3 sub_box;
    VEC3 sum_mid;
    VEC3 sub_hit;
    f32 dist;
    f32 best;
    s8 flags;
    s8 slot;
    s32 hits;
    u32 i;

    eft_shape_init((struct EftShape*)&edge_box);
    eft_capsule_ctor((nw4r::math::CAPSULE*)&edge);
    VEC3_ctor(&corners[0]);
    VEC3_ctor(&corners[1]);
    VEC3_ctor(&corners[2]);
    VEC3_ctor(&corners[3]);
    pl_segment_init(&seg);
    vec3_ctor_return(&plane.normal);
    VEC3_ctor(&point);
    VEC3_ctor(&unused_a);
    VEC3_ctor(&rel);
    VEC3_ctor(&unused_b);
    if (pl_coll_hit_count >= 9) {
        return 0;
    }
    subVec3(&sub_end, &work->end, &poly->vtx0);
    copyVec3(&rel, &sub_end);
    if (vec3_dot(&rel.x, &poly->normal.x) < 0.0f) {
        return 0;
    }
    subVec3(&sub_start, &work->start, &poly->vtx0);
    copyVec3(&rel, &sub_start);
    dist = vec3_dot(&rel.x, &poly->normal.x);
    flags = !(dist > 0.0f);
    if (dist > work->radius) {
        return 0;
    }
    if (dist < -work->radius) {
        flags |= 2;
    }
    vec3_scale(&back, &poly->normal, -1.0f);
    vec3_scale(&step_end, &back, work->radius);
    addVec3(&sum_end, &work->end, &step_end);
    copyVec3(&point, &sum_end);
    subVec3(&dir, &work->start, &work->end);
    copyVec3(&seg.start, &point);
    vec3_scale(&step_start, &back, work->radius);
    addVec3(&sum_start, &work->start, &step_start);
    copyVec3(&seg.end, &sum_start);
    copyVec3(&plane.normal, &poly->normal);
    plane.dist = vec3_dot(&poly->normal.x, &poly->vtx0.x);
    if (pl_segment_plane_hit(&seg, &plane, &point) == 1 && pl_tri_point_inside(&point, &poly->vtx0, &poly->normal) != 0) {
        if (collect == 0) {
            return 1;
        }
        subVec3(&travel, &point, &seg.start);
        pl_hit_box_set(&pl_hit_box[pl_coll_hit_count], poly);
        pl_vec3_snap_zero(&pl_hit_box[pl_coll_hit_count].normal, &pl_hit_box[pl_coll_hit_count].normal);
        copyVec3(&pl_coll_closest_0[pl_coll_hit_count], &work->end);
        slot = pl_coll_hit_count;
        pl_coll_slot_kind[slot] = flags;
        pl_coll_slot_used[slot] = 0;
        pl_coll_slot_dist[slot] = vec3_len(&travel.x);
        copyVec3(&pl_coll_closest_1[slot], &point);
        pl_coll_hit_count++;
        return 1;
    }
    pl_coll_slot_used[pl_coll_hit_count] = 1;
    VEC3_ctor(&best_seg);
    VEC3_ctor(&best_cap);
    VEC3_ctor(&on_seg);
    VEC3_ctor(&on_cap);
    VEC3_ctor(&unused_c);
    VEC3_ctor(&unused_d);
    VEC3_ctor(&best_dir);
    vec3_negate(&rev, &dir);
    pl_segment_box_init(&sweep_box);
    copyVec3(&sweep_box.start, &work->end);
    copyVec3(&sweep_box.end, &work->start);
    subVec3(&sub_box, &sweep_box.end, &sweep_box.start);
    copyVec3(&sweep_box.dir, &sub_box);
    vec3_scale(&half, &sweep_box.dir, 0.5f);
    addVec3(&sum_mid, &work->end, &half);
    copyVec3(&sweep_box.center, &sum_mid);
    sweep_box.half_length = vec3_len(&half.x);
    copyVec3(&corners[0], &poly->vtx0);
    copyVec3(&corners[1], &poly->vtx1);
    copyVec3(&corners[2], &poly->vtx2);
    copyVec3(&corners[3], &poly->vtx0);
    hits = 0;
    best = 1000000.0f;
    for (i = 0; i < 3; i++) {
        copyVec3(&edge.start, &corners[i]);
        copyVec3(&edge.end, &corners[i + 1]);
        edge.radius = work->radius;
        pl_capsule_box_set(&edge, &edge_box);
        if (pl_segment_capsule_hit(&sweep_box, &edge_box, &on_seg, &on_cap) != 0) {
            hits++;
            subVec3(&sub_hit, &on_seg, &work->end);
            copyVec3(&dir, &sub_hit);
            dist = vec3_len(&dir.x);
            if (best > dist) {
                best = dist;
                copyVec3(&best_seg, &on_seg);
                copyVec3(&best_cap, &on_cap);
                copyVec3(&best_dir, &dir);
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
        copyVec3(&pl_hit_box[pl_coll_hit_count].normal, &best_dir);
        pl_vec3_snap_zero(&pl_hit_box[pl_coll_hit_count].normal, &pl_hit_box[pl_coll_hit_count].normal);
        copyVec3(&pl_coll_closest_0[pl_coll_hit_count], &work->end);
        pl_coll_slot_kind[pl_coll_hit_count] = 127;
        copyVec3(&pl_coll_closest_1[pl_coll_hit_count], &best_seg);
        copyVec3(&pl_coll_closest_2[pl_coll_hit_count], &best_seg);
        pl_coll_slot_dist[pl_coll_hit_count] = best;
        pl_coll_hit_count++;
        return 1;
    }
    return 0;
}

/* 0x80294B64 (0x398): 1 when the swept sphere of `work` crosses polygon `poly`, through its face or against one of its
 * edges (tested as capsules). */
s32 pl_sweep_poly_hit(PlSlideWork* work, PlCollPoly* poly) {
    PlSegmentBox sweep_box;
    VEC3 corners[4];
    PlCapsuleBox edge_box;
    PlSegment seg;
    VEC3 point;
    VEC3 unused_a;
    VEC3 rel;
    VEC3 unused_b;
    VEC3 back;
    VEC3 dir;
    PlPlane plane;
    VEC3 unused_c;
    VEC3 unused_d;
    VEC3 unused_e;
    VEC3 unused_f;
    VEC3 unused_g;
    VEC3 unused_h;
    VEC3 unused_i;
    VEC3 rev;
    VEC3 half;
    VEC3 sub_end;
    VEC3 sub_start;
    VEC3 sum_end;
    VEC3 step_end;
    VEC3 sum_start;
    VEC3 step_start;
    VEC3 sub_box;
    VEC3 sum_mid;
    VEC3 sub_edge;
    VEC3 sum_edge;
    VEC3 half_edge;
    u32 i;

    VEC3_ctor(&point);
    VEC3_ctor(&unused_a);
    VEC3_ctor(&rel);
    VEC3_ctor(&unused_b);
    subVec3(&sub_end, &work->end, &poly->vtx0);
    copyVec3(&rel, &sub_end);
    if (vec3_dot(&rel.x, &poly->normal.x) < 0.0f) {
        return 0;
    }
    subVec3(&sub_start, &work->start, &poly->vtx0);
    copyVec3(&rel, &sub_start);
    if (vec3_dot(&rel.x, &poly->normal.x) > work->radius) {
        return 0;
    }
    vec3_scale(&back, &poly->normal, -1.0f);
    vec3_scale(&step_end, &back, work->radius);
    addVec3(&sum_end, &work->end, &step_end);
    copyVec3(&point, &sum_end);
    subVec3(&dir, &work->start, &work->end);
    pl_segment_init(&seg);
    vec3_ctor_return(&plane.normal);
    copyVec3(&seg.start, &point);
    vec3_scale(&step_start, &back, work->radius);
    addVec3(&sum_start, &work->start, &step_start);
    copyVec3(&seg.end, &sum_start);
    copyVec3(&plane.normal, &poly->normal);
    plane.dist = vec3_dot(&poly->normal.x, &poly->vtx0.x);
    if (pl_segment_plane_hit(&seg, &plane, &point) == 1 && pl_tri_point_inside(&point, &poly->vtx0, &poly->normal) != 0) {
        return 1;
    }
    VEC3_ctor(&unused_c);
    VEC3_ctor(&unused_d);
    VEC3_ctor(&unused_e);
    VEC3_ctor(&unused_f);
    VEC3_ctor(&unused_g);
    VEC3_ctor(&unused_h);
    VEC3_ctor(&unused_i);
    vec3_negate(&rev, &dir);
    pl_segment_box_init(&sweep_box);
    copyVec3(&sweep_box.start, &work->end);
    copyVec3(&sweep_box.end, &work->start);
    subVec3(&sub_box, &sweep_box.end, &sweep_box.start);
    copyVec3(&sweep_box.dir, &sub_box);
    vec3_scale(&half, &sweep_box.dir, 0.5f);
    addVec3(&sum_mid, &work->end, &half);
    copyVec3(&sweep_box.center, &sum_mid);
    sweep_box.half_length = vec3_len(&half.x);
    VEC3_ctor(&corners[0]);
    VEC3_ctor(&corners[1]);
    VEC3_ctor(&corners[2]);
    VEC3_ctor(&corners[3]);
    copyVec3(&corners[0], &poly->vtx0);
    copyVec3(&corners[1], &poly->vtx1);
    copyVec3(&corners[2], &poly->vtx2);
    copyVec3(&corners[3], &poly->vtx0);
    for (i = 0; i < 3; i++) {
        eft_shape_init((struct EftShape*)&edge_box);
        copyVec3(&edge_box.start, &corners[i]);
        copyVec3(&edge_box.end, &corners[i + 1]);
        edge_box.radius = work->radius;
        subVec3(&sub_edge, &edge_box.end, &edge_box.start);
        copyVec3(&edge_box.dir, &sub_edge);
        vec3_scale(&half_edge, &edge_box.dir, 0.5f);
        addVec3(&sum_edge, &edge_box.start, &half_edge);
        copyVec3(&edge_box.center, &sum_edge);
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
    VEC3 sum_one;
    VEC3 scale_one;
    VEC3 push_one;
    VEC3 sum_best;
    VEC3 scale_best;
    VEC3 push_best;
    s32 nearest;
    f32 best;
    s32 count;
    s32 i;

    VEC3_ctor(&unused);
    count = pl_coll_hit_count;
    if (count != 0) {
        if (count == 1) {
            if (pl_coll_slot_kind[0] == 127) {
                vec3_scale(&scale_one, &pl_hit_box[0].normal, pl_coll_slot_dist[0]);
                addVec3(&sum_one, &pl_coll_closest_0[0], &scale_one);
                copyVec3(hit, &sum_one);
            } else {
                copyVec3(hit, &pl_coll_closest_1[0]);
                vec3_scale(&push_one, &pl_hit_box[0].normal, work->radius);
                addVec3To(hit, &push_one);
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
                vec3_scale(&scale_best, &pl_hit_box[nearest].normal, pl_coll_slot_dist[nearest]);
                addVec3(&sum_best, &pl_coll_closest_0[nearest], &scale_best);
                copyVec3(hit, &sum_best);
            } else {
                copyVec3(hit, &pl_coll_closest_1[nearest]);
                vec3_scale(&push_best, &pl_hit_box[nearest].normal, work->radius);
                addVec3To(hit, &push_best);
            }
        }
    }
    return pl_coll_hit_count;
}

/* 0x802950D8 (0x1B8): sweeps the player's body shape from its old position (+0x48) to its new one (+0x3C) through
 * the walls; `mode` 0 first clears the wall-direction bits. */
void pl_coll_shape_set(_PLW* self, u8 mode, u16 layers) {
    PlSegment seg;
    VEC3 scale;
    VEC3 start_copy;
    HitSphere* shape;
    f32 radius;
    f32 lift;

    VEC3_ctor(&seg.start);
    VEC3_ctor(&seg.end);
    VEC3_ctor(&scale);
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
        assignVec3((Vec*)&start_copy, (Vec*)&seg.start);
        pl_coll_shape_hits(&seg, radius, 0xFFFF, layers, self);
    }
}

/* 0x80295290 (0x2B4): slides the player's sphere `radius` along `seg` against the walls, then records the walls it
 * touched in its move-work chunk (a floor-kind wall moved to the front) and their directions in +0x364. */
void pl_coll_shape_hits(PlSegment* seg, f32 radius, u32 kind, u16 layers, _PLW* self) {
    VEC3 unused;
    PlMoveEntry swap;
    PlCollPoly* poly;
    s8* free_at;
    u32 i;
    u32 angle;
    u8 count;
    s32 swapped;
    s32 last;

    VEC3_ctor(&unused);
    coll_wall_slide(&seg->start, &seg->end, radius, &self->vec_0x03C, kind, layers, self->kind_0x09 != 3, self->area_0x16);
    if (self->chunk_ofs < 4) {
        count = 0;
        swapped = 0;
        pl_move_work[self->chunk_ofs][0].kind = 0;
        if (pl_coll_free_count != 0) {
            for (i = 0, free_at = pl_coll_slot_free; i < pl_coll_free_count; i++, free_at++) {
                poly = pl_hit_id_list[*free_at];
                angle = (u16)((u16)calc_xz_angle(self->vec_0x03C.x, self->vec_0x03C.z, pl_coll_closest_1[*free_at].x,
                                                 pl_coll_closest_1[*free_at].z) - self->field_0x058);
                pl_move_work[self->chunk_ofs][count].kind = 1;
                if (poly->attr & 8) {
                    pl_move_work[self->chunk_ofs][count].kind |= 2;
                    swapped = 1;
                }
                pl_move_work[self->chunk_ofs][count].carve_0x14 = poly->field_0x03;
                pl_move_work[self->chunk_ofs][count].angle = angle;
                copyVec3(&pl_move_work[self->chunk_ofs][count++].pos, &poly->normal);
                if (swapped) {
                    swapped = 0;
                    if (count != 1) {
                        last = count - 1;
                        pl_move_entry_ctor(&swap);
                        swap.kind = pl_move_work[self->chunk_ofs][0].kind;
                        swap.carve_0x14 = pl_move_work[self->chunk_ofs][0].carve_0x14;
                        swap.angle = pl_move_work[self->chunk_ofs][0].angle;
                        copyVec3(&swap.pos, &pl_move_work[self->chunk_ofs][0].pos);
                        pl_move_work[self->chunk_ofs][0].kind = pl_move_work[self->chunk_ofs][last].kind;
                        pl_move_work[self->chunk_ofs][0].carve_0x14 = pl_move_work[self->chunk_ofs][last].carve_0x14;
                        pl_move_work[self->chunk_ofs][0].angle = pl_move_work[self->chunk_ofs][last].angle;
                        copyVec3(&pl_move_work[self->chunk_ofs][0].pos, &pl_move_work[self->chunk_ofs][last].pos);
                        pl_move_work[self->chunk_ofs][last].kind = swap.kind;
                        pl_move_work[self->chunk_ofs][last].carve_0x14 = swap.carve_0x14;
                        pl_move_work[self->chunk_ofs][last].angle = swap.angle;
                        copyVec3(&pl_move_work[self->chunk_ofs][last].pos, &swap.pos);
                    }
                }
                self->field_0x364 |= 1 << (angle >> 11);
            }
            pl_move_work[self->chunk_ofs][count].kind = 0;
        }
    }
}

/* 0x80295544 (0x34): constructs one move-work entry (its position vector). */
PlMoveEntry* pl_move_entry_ctor(PlMoveEntry* self) {
    VEC3_ctor(&self->pos);
    return self;
}

/* 0x80295578 (0x1C4): sweeps the enemy's body sphere (its move-parameter offset turned by its yaw, sized from its
 * data and scale) from its previous position to its current one through the walls; `mode` 0 first clears the
 * wall-direction bits. */
void em_coll_shape_set(_ENEMY_WORK* self, u8 mode, u16 layers) {
    PlSegment seg;
    VEC3 unused;
    HitSphere body;
    f32* params;
    f32 radius;

    VEC3_ctor(&seg.start);
    VEC3_ctor(&seg.end);
    VEC3_ctor(&unused);
    pl_vec3_ctor_ret(&body.pos);
    if (mode == 0) {
        self->field_0x218 = 0;
    }
    params = get_enemy_data(self)->move_params_0x50;
    copyVec3(&body.pos, &self->move_param_0x21C);
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

/* 0x8029573C - construct one vector in place and return it. */
VEC3* pl_vec3_ctor_ret(VEC3* v) {
    VEC3_ctor(v);
    return v;
}

/* 0x8029576C (0x1B8): slides the enemy's sphere `radius` along `seg` against the walls, then records the walls it
 * touched in its slot table (up to ten, ended by a zero flag word) and their directions in +0x218. */
void em_coll_shape_hits(PlSegment* seg, f32 radius, u32 kind, u16 layers, _ENEMY_WORK* self) {
    VEC3 unused;
    EmMotionSlot* slot;
    PlCollPoly* poly;
    s8* free_at;
    u32 i;
    u32 angle;
    u8 count;
    s32 mode;

    VEC3_ctor(&unused);
    count = 0;
    copyVec3(&unused, &self->pos);
    if (self->field_0x1E2 == 2 || (self->field_0x1E2 == 1 && self->field_0x1E3 == 1)) {
        mode = 0;
    } else {
        mode = (self->field_0x1E2 == 1) + 1;
    }
    coll_wall_slide(&seg->start, &seg->end, radius, &self->pos, kind, layers, mode, self->area_no);
    self->slots_0x244[0].flags = 0;
    if (pl_coll_free_count != 0) {
        for (i = 0, free_at = pl_coll_slot_free; i < pl_coll_free_count; i++, free_at++) {
            poly = pl_hit_id_list[*free_at];
            angle = (u16)((u16)calc_xz_angle(self->pos.x, self->pos.z, pl_coll_closest_1[*free_at].x,
                                             pl_coll_closest_1[*free_at].z) - self->field_0x1C0);
            slot = &self->slots_0x244[count];
            slot->flags = poly->attr;
            slot->kind = poly->kind;
            slot->value = angle;
            count++;
            copyVec3(&slot->vec, &poly->normal);
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
    VEC3 seg_to;
    VEC3 seg_from;
    VEC3 dir;
    VEC3 mid;
    VEC3 sub;
    VEC3 seg;
    VEC3 mid_sum;
    VEC3 half;
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

    VEC3_ctor(&copy_a);
    VEC3_ctor(&copy_b);
    if (area == 0xFF) {
        area = get_now_areano();
    }
    cell = &pl_land_data[area];
    VEC3_ctor(&seg_to);
    VEC3_ctor(&seg_from);
    VEC3_ctor(&dir);
    VEC3_ctor(&mid);
    subVec3(&sub, to, from);
    copyVec3(&dir, &sub);
    if (vec3_length_sq(&dir.x) > 0.001f) {
        vec3_normalize_into(&dir, &dir);
    }
    copyVec3(&seg_to, to);
    copyVec3(&seg_from, from);
    copyVec3(&copy_a, copyVec3(&copy_b, &seg_to));
    for (i = 0; i < 10; i++) {
        pl_coll_slot_dist[i] = 0.0f;
        pl_coll_slot_free[i] = 0;
        pl_coll_slot_hit[i] = 0;
    }
    seen_count = 0;
    pl_coll_free_count = 0;
    pl_coll_hit_count = 0;
    sqrtf_thunk(ef_vec3_dist_sq(&seg_to, &seg_from));
    subVec3(&seg, &seg_to, &seg_from);
    copyVec3(&dir, &seg);
    vec3_scale(&half, &dir, 0.5f);
    addVec3(&mid_sum, &seg_from, &half);
    copyVec3(&mid, &mid_sum);
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
        copyVec3(out, &pl_coll_closest_1[0]);
        if (grid_hit != NULL) {
            *grid_hit = pl_coll_slot_hit[0];
        }
    }
    return pl_coll_hit_count;
}

/* 0x80295EF4 (0x304): records polygon `poly` (from grid `grid`) when the segment `from`..`to` crosses it, keeping the
 * crossings sorted by their distance from `from`; 1 when recorded. */
s32 pl_inter_poly_hit(VEC3* to, VEC3* from, PlCollPoly* poly, u8 grid) {
    PlSegment seg;
    VEC3 point;
    VEC3 unused_a;
    VEC3 rel;
    VEC3 unused_b;
    VEC3 unused_c;
    PlPlane plane;
    VEC3 sub_hit;
    VEC3 sub_from;
    VEC3 sub_to;
    f32 dist;
    s8 count;
    s32 i;
    s32 j;

    VEC3_ctor(&point);
    VEC3_ctor(&unused_a);
    VEC3_ctor(&rel);
    VEC3_ctor(&unused_b);
    VEC3_ctor(&unused_c);
    if (pl_coll_hit_count >= 9) {
        return 0;
    }
    subVec3(&sub_from, from, &poly->vtx0);
    copyVec3(&rel, &sub_from);
    if (vec3_dot(&rel.x, &poly->normal.x) < 0.0f) {
        return 0;
    }
    subVec3(&sub_to, to, &poly->vtx0);
    copyVec3(&rel, &sub_to);
    if (vec3_dot(&rel.x, &poly->normal.x) > 1e-05f) {
        return 0;
    }
    pl_segment_init(&seg);
    vec3_ctor_return(&plane.normal);
    copyVec3(&seg.start, from);
    copyVec3(&seg.end, to);
    copyVec3(&plane.normal, &poly->normal);
    plane.dist = vec3_dot(&poly->normal.x, &poly->vtx0.x);
    if (pl_segment_plane_hit(&seg, &plane, &point) == 1 && pl_tri_point_inside(&point, &poly->vtx0, &poly->normal) != 0) {
        subVec3(&sub_hit, &point, &seg.start);
        dist = vec3_len(&sub_hit.x);
        count = pl_coll_hit_count;
        if (count == 0) {
            pl_coll_slot_dist[count] = dist;
            pl_hit_id_list[count] = poly;
            copyVec3(&pl_coll_closest_1[count], &point);
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
                copyVec3(&pl_coll_closest_1[j], &pl_coll_closest_1[j - 1]);
                pl_coll_slot_hit[j] = pl_coll_slot_hit[j - 1];
            }
            pl_coll_slot_dist[i] = dist;
            pl_hit_id_list[i] = poly;
            copyVec3(&pl_coll_closest_1[i], &point);
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
    VEC3 hi;
    VEC3 lo;
    VEC3 a;
    VEC3 b;
    VEC3 sub_x0;
    VEC3 scale_x0;
    VEC3 sum_x0;
    VEC3 sub_x1;
    VEC3 scale_x1;
    VEC3 sum_x1;
    VEC3 sub_y0;
    VEC3 scale_y0;
    VEC3 sum_y0;
    VEC3 sub_y1;
    VEC3 scale_y1;
    VEC3 sum_y1;
    VEC3 sub_z0;
    VEC3 scale_z0;
    VEC3 sum_z0;
    VEC3 sub_z1;
    VEC3 scale_z1;
    VEC3 sum_z1;
    s32 clipped;

    VEC3_ctor(&dir);
    VEC3_ctor(&clip);
    assignVec3((Vec*)&hi, (Vec*)&land->grid_a.max);
    assignVec3((Vec*)&lo, (Vec*)&land->grid_a.min);
    VEC3_ctor(&a);
    VEC3_ctor(&b);
    copyVec3(&a, from);
    copyVec3(&b, to);
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
    copyVec3(&clip, &a);
    if (clip.x < lo.x) {
        subVec3(&sub_x0, &clip, &b);
        copyVec3(&dir, &sub_x0);
        vec3_scale(&scale_x0, &dir, (lo.x - b.x) / dir.x);
        copyVec3(&dir, &scale_x0);
        addVec3(&sum_x0, &dir, &b);
        copyVec3(&clip, &sum_x0);
        clip.x = lo.x;
        clipped = 1;
    }
    if (clip.x > hi.x) {
        subVec3(&sub_x1, &clip, &b);
        copyVec3(&dir, &sub_x1);
        vec3_scale(&scale_x1, &dir, (hi.x - b.x) / dir.x);
        copyVec3(&dir, &scale_x1);
        addVec3(&sum_x1, &dir, &b);
        copyVec3(&clip, &sum_x1);
        clip.x = hi.x;
        clipped = 1;
    }
    if (clip.y < lo.y) {
        subVec3(&sub_y0, &clip, &b);
        copyVec3(&dir, &sub_y0);
        vec3_scale(&scale_y0, &dir, (lo.y - b.y) / dir.y);
        copyVec3(&dir, &scale_y0);
        addVec3(&sum_y0, &dir, &b);
        copyVec3(&clip, &sum_y0);
        clip.y = lo.y;
        clipped = 1;
    }
    if (clip.y > hi.y) {
        subVec3(&sub_y1, &clip, &b);
        copyVec3(&dir, &sub_y1);
        vec3_scale(&scale_y1, &dir, (hi.y - b.y) / dir.y);
        copyVec3(&dir, &scale_y1);
        addVec3(&sum_y1, &dir, &b);
        copyVec3(&clip, &sum_y1);
        clip.y = hi.y;
        clipped = 1;
    }
    if (clip.z < lo.z) {
        subVec3(&sub_z0, &clip, &b);
        copyVec3(&dir, &sub_z0);
        vec3_scale(&scale_z0, &dir, (lo.z - b.z) / dir.z);
        copyVec3(&dir, &scale_z0);
        addVec3(&sum_z0, &dir, &b);
        copyVec3(&clip, &sum_z0);
        clip.z = lo.z;
        clipped = 1;
    }
    if (clip.z > hi.z) {
        subVec3(&sub_z1, &clip, &b);
        copyVec3(&dir, &sub_z1);
        vec3_scale(&scale_z1, &dir, (hi.z - b.z) / dir.z);
        copyVec3(&dir, &scale_z1);
        addVec3(&sum_z1, &dir, &b);
        copyVec3(&clip, &sum_z1);
        clip.z = hi.z;
        clipped = 1;
    }
    copyVec3(out, &clip);
    return clipped;
}

/* 0x80296870 (0x138): the grid-B column (`*grid_x`, `*grid_z`) under `pos`; an off-grid box divides the offset from
 * its corner in floating point. */
extern "C" void pl_land_cell_index(nw4r::math::VEC3* pos, PlLandCell* land, u32* grid_x, u32* grid_z)
{
    VEC3 rel;
    VEC3 sub;
    s32 x;
    s32 z;
    s32 min_x;
    s32 min_z;

    VEC3_ctor(&rel);
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
        subVec3(&sub, pos, &land->grid_b.min);
        copyVec3(&rel, &sub);
        *grid_x = (s32)(rel.x / (f32)land->grid_b.cell_x);
        *grid_z = (s32)(rel.z / (f32)land->grid_b.cell_z);
    }
}

/* One cell of the record's second grid, indexed [x][y][z]. */
extern "C" PlCollPoly** pl_land_grid_b_cell(s32 x, s32 y, s32 z, PlLandCell* land)
{
    return (PlCollPoly**)land->grid_b.cells[(x * land->grid_b.dim_y + y) * land->grid_b.dim_z + z];
}

/* One cell of the record's first grid, indexed [x][y][z]. */
extern "C" PlCollPoly** pl_land_grid_a_cell(s32 x, s32 y, s32 z, PlLandCell* land)
{
    return (PlCollPoly**)land->grid_a.cells[(x * land->grid_a.dim_y + y) * land->grid_a.dim_z + z];
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
    f32 rhs;
    f32 lhs;
    u32 lhs_bits;
    u32 rhs_bits;

    lhs = (b->x - a->x) * (c->y - a->y);
    rhs = (c->x - a->x) * (b->y - a->y);
    lhs_bits = *(u32*)&lhs;
    rhs_bits = *(u32*)&rhs;
    if (lhs_bits == rhs_bits + 1) {
        lhs = rhs;
    }
    if (lhs_bits == rhs_bits - 1) {
        lhs = rhs;
    }
    return lhs - rhs;
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
    VEC3 sub_ba;
    VEC3 sub_bc;

    VEC3_ctor(&edge_ba);
    VEC3_ctor(&edge_bc);
    subVec3(&sub_ba, a, b);
    copyVec3(&edge_ba, &sub_ba);
    subVec3(&sub_bc, c, b);
    copyVec3(&edge_bc, &sub_bc);
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

    VEC3_ctor(&edge_normal);
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
    copyVec3(out, in);
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

    VEC3_ctor(&whole);
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

    VEC3_ctor(&rel_from);
    VEC3_ctor(&rel_to);
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
        copyVec3(&rel_from, from);
        copyVec3(&rel_to, to);
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

    VEC3_ctor(&rel_from);
    VEC3_ctor(&rel_to);
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
        copyVec3(&rel_from, from);
        copyVec3(&rel_to, to);
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
extern "C" void fn_802977E4(LandData* self)
{
    self->field_0x00 = 0;
    self->field_0x02 = 0;
    self->field_0x03 = 0;
    self->field_0x04 = 0;
    self->field_0x06 = 0;
    self->field_0x07 = 0;
    setVector3(&self->vec_0x08, 0.0f, 1.0f, 0.0f);
}

/* Copies one land record field by field (the hit normal through `copyVec3`). */
inline void pl_land_set(LandData* dst, LandData* src) {
    dst->field_0x00 = src->field_0x00;
    dst->field_0x02 = src->field_0x02;
    dst->field_0x03 = src->field_0x03;
    dst->field_0x04 = src->field_0x04;
    dst->field_0x06 = src->field_0x06;
    dst->field_0x07 = src->field_0x07;
    copyVec3(&dst->vec_0x08, &src->vec_0x08);
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

    em_land_data_init(&near[0]);
    em_land_data_init(&near[1]);
    em_land_data_init(&near[2]);
    em_land_data_init(&near[3]);
    em_land_data_init(&near[4]);
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

/* Copies one position record. */
extern "C" void pl_land_copy(LandData* dst, LandData* src)
{
    *dst = *src;
}

/* Zeroes the four vectors of one land record. */
extern "C" PlLandCell* pl_land_cell_ctor(PlLandCell* self)
{
    VEC3_ctor(&self->grid_a.min);
    VEC3_ctor(&self->grid_a.max);
    VEC3_ctor(&self->grid_b.min);
    VEC3_ctor(&self->grid_b.max);
    return self;
}

/* 0x80297DE8 (0x4C): constructs the vertices and the normal of one hit-box polygon. */
extern "C" PlCollPoly* pl_hit_box_ctor(PlCollPoly* self)
{
    VEC3_ctor(&self->vtx0);
    VEC3_ctor(&self->vtx1);
    VEC3_ctor(&self->vtx2);
    VEC3_ctor(&self->normal);
    return self;
}
