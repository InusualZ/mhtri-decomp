/*
 * Pl/pl_coll.cpp - the player actor's ground/hit collision band: the sphere and box tests, the ground and hit queries,
 *   the land table and the hit-id lists.
 * RANGE. .text 0x8028F66C-0x80297E34 (81 functions); .ctors 0x8056F370, .data 0x805CDC48-0x805CDC88, .bss
 *   0x806AB848-0x806AC8A8 (the `Pl/pl_coll.h` arrays the static initialiser `fn_80297C30` constructs), .sdata
 *   0x80792270-0x80792278, .sbss 0x80794B58-0x80794B60, .sdata2 0x8079A318-0x8079A380, extab, extabindex.  Two
 *   banner-marked sections: the collision queries (0x8028F66C) and the list scans (0x80295EF4); `menu/menu_item.cpp`
 *   follows.  `fn_8028F66C`'s first constant, `lbl_8079A314` = 0.001f, sits in `Pl/pl_hit_sphere.cpp`'s `.sdata2`.
 * NAMES. `GetGroundHit`, `GetGroundHit2`, `hit_ground_comon`, `findInterSection`, `findInterSection2` and
 *   `findInterSection3` are the map's (mangled) names; every other function keeps its unmangled map stem in an
 *   `extern "C"` block.
 * RESIDUALS. 53 functions unwritten (objdiff scores them 0) in 16 runs: 0x8028FA20-0x802910C0, 0x80291114-0x8029163C,
 *   0x80291664-0x802919FC, 0x80291A70-0x80291B08, 0x80291B48-0x80291BBC, 0x80291CD0-0x80292468, 0x802924C0-0x80293A7C,
 *   0x80293A88-0x8029573C, 0x8029576C-0x80295924, 0x80295998-0x802961F8, 0x80296260-0x80296368, 0x80296448-0x802969A8,
 *   0x802969F8-0x8029708C, 0x802970A0-0x802977E4, 0x80297814-0x80297BE4, 0x80297C30-0x80297D9C.  Known blockers:
 *   `fn_80291A70`/`fn_80291B48` read `_PLW` bytes with no field (+0x1E1 inside `equipB2.deco_level`, +0x004 inside
 *   `unk003`), and `fn_80291A70` needs `_HIT_TENJO_DATA`'s layout (still a forward declaration); `GetGroundHit2`,
 *   `hit_ground_comon` and `findInterSection3` walk `pl_land_data` (0x88 stride) and `pl_hit_id_list` with a 0x40-byte
 *   hit record that is not `LandData` and needs naming first; the query half `fn_8028FA20`-`fn_80290D08` takes a record
 *   with `VEC3` fields at +0x18/+0x1C/+0x28 and radii at +0x0C/+0x34 (`fn_802900F8` is the cheapest way in).
 *  - `fn_802910C0`: the target saves the `in` pointer (r4) after the float parameter (f1), ours before (a scheduler
 *    placement; three source variants measure the same);
 *  - `fn_802961F8`/`fn_80296228`: retail's loop guard is `cmplwi r5,0` + `ble` where MWCC emits `cmpwi` + `beq` from
 *    every shape tried (`while`, `for`, `for (;;)`+`break`, `u32`/`s32` counts, an explicit guard);
 *  - `fn_80296368`, `fn_802963B0`, `fn_802963FC`, `fn_802969A8`, `fn_802969D0`: the operand order of one `add` in the
 *    index arithmetic and the branch sense of the range checks (`||`-chain against `&&`-chain).
 *  - flipcheck: the object emits no `.bss` (0x1060 claimed), `.ctors` (0x4), `.data` (0x40), `.sbss` (0x8) or `.sdata`
 *    (0x8); `.text` 0x9C0, `.sdata2` 0xC, extab 0x80 and extabindex 0xC0 against the claims 0x87C8, 0x68, 0x218 and
 *    0x324; every compared section differs.  The pools share a literal with `Pl/pl_hit_sphere.cpp` (a fold candidate).
 */

/* ==== 0x8028F66C-0x80295EF4: the collision queries ==== */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "pl.h"              /* the Pl module header; pulls in `ef.h`'s `VEC3_ctor` */
#include "fn_8004CAD8.h"     /* the vector helpers this unit calls (rule 2) */
#include "ef/fn_800AEE48.h"  /* `fn_800B0B90` (rule 2) */
#include "Pl/fn_80288CEC.h"
#include "Pl/fn_8028F66C.h"  /* the two callees whose owners' headers cannot declare them */

/* One 0x10-byte placed sphere: an `nw4r::math::VEC3` position plus the radius the overlap tests read
 * at +0xC.  Evidence: `fn_8029CD7C`/`fn_8029D110` (the two callers) materialise the arguments as
 * 16-byte locals; every vector helper they are handed to (`PSVECSubtract`, `fn_80050EAC`) reads the
 * first 12 bytes only, while `fn_8028F66C`/`fn_8028F758` read +0xC as a scalar and compare the
 * squared distance against `(radius_a + radius_b)`. size: 0x10 */
struct HitSphere {
    /* +0x0 */ nw4r::math::VEC3 pos;
    /* +0xC */ f32 radius;
};

/* `_HIT_TENJO_DATA` is the ceiling-hit record `hit_ground_comon` carries by pointer; like `LandData`
 * its layout is not reconstructed yet, so it stays a forward declaration for the mangling. */
struct _HIT_TENJO_DATA;

/* The three functions this unit exports with a real (mangled) name that are declared here; the
 * parameters are the ones the manglings spell, so the front-end reproduces the map's names (rule 9). */
f32 GetGroundHit2(nw4r::math::VEC3* pos, u32 ground, u8 flag, u8* hit);
s32 hit_ground_comon(nw4r::math::VEC3* pos, u8 ground, LandData* land, f32* out, u32 kind,
                     u16 layers, f32* surf, f32* extra, _HIT_TENJO_DATA* tenjo);
s32 findInterSection3(nw4r::math::VEC3* a, nw4r::math::VEC3* b, nw4r::math::VEC3* c, u8 d, u32 e,
                      u8 f, u16 g, u8* h, LandData* land, u8 tenjo);

/* Every bare `fn_XXXXXXXX` is the map's own (unmangled) name, so the definitions carry C linkage. */
extern "C" {
s32 fn_8028F66C(HitSphere* a, HitSphere* b, VEC3* out);
s32 fn_8028F758(HitSphere* a, HitSphere* b, VEC3* out);
f32 fn_8028F84C(f32 value, f32 low, f32 high);
f32 fn_8028F86C(PlBox* box, const VEC3* point, f32* param);
f32 fn_8028F938(VEC3* start, VEC3* end, VEC3* point, VEC3* out);
void fn_802910C0(VEC3* out, const VEC3* in, f32 scale);
s32 fn_802919FC(_PLW* self, VEC3* pos, LandData* land, f32* out, u32 kind);
s32 fn_80291B08(_PLW* self, VEC3* pos, LandData* land, f32* out, u32 kind);
s32 fn_80291BBC(VEC3* pos, u8 flag, u16* hit_layer, f32* out, u32 kind);
void fn_80291C50(VEC3* pos, u8 flag, LandData* land, u32 kind);
VEC3* fn_80292468(VEC3* pair);
void fn_80293A7C(VEC3* v, f32 divisor);
VEC3* fn_8029573C(VEC3* v);
}

/* ------------------------------------------------------------------------------------------------ *
 * Bodies, in address order.
 * ------------------------------------------------------------------------------------------------ */

extern "C" {

/* 0x8028F66C - place `out` on sphere `b`'s surface toward sphere `a` when the two overlap. */
s32 fn_8028F66C(HitSphere* a, HitSphere* b, VEC3* out) {
    VEC3 sep;
    f32 dist;
    f32 reach;

    VEC3_ctor(&sep);
    dist = fn_80050EAC(a, b);
    reach = a->radius + b->radius;
    if (dist <= reach * reach) {
        if (dist > 0.001f) {
            PSVECSubtract(&sep.x, &a->pos.x, &b->pos.x);
            fn_80051424(&sep.x, &sep.x, b->radius / sqrt_f32(dist));
            fn_800513CC(out, &sep, &b->pos);
        } else {
            fn_80050028(out, &b->pos);
        }
        return 1;
    }
    return 0;
}

/* 0x8028F758 - the same overlap test, but `out` gets the penetration vector: the direction `a - b`
 * scaled by `(radius_a + radius_b - |a - b|) / |a - b|`, or zero when they touch. */
s32 fn_8028F758(HitSphere* a, HitSphere* b, VEC3* out) {
    VEC3 sep;
    f32 dist;
    f32 reach;

    VEC3_ctor(&sep);
    dist = fn_80050EAC(a, b);
    reach = a->radius + b->radius;
    if (dist <= reach * reach) {
        if (dist > 0.001f) {
            f32 mag;
            f32 ratio;

            PSVECSubtract(&sep.x, &a->pos.x, &b->pos.x);
            mag = sqrt_f32(dist);
            ratio = (reach - mag) / mag;
            fn_80051424(&out->x, &sep.x, ratio);
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
f32 fn_8028F84C(f32 value, f32 low, f32 high) {
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
f32 fn_8028F86C(PlBox* box, const VEC3* point, f32* param) {
    VEC3 sep;
    f32 axis_len2;
    f32 t;

    subVec3(&sep, point, &box->vec_0x00);
    axis_len2 = fn_80050EDC(&box->vec_0x18.x);
    t = 0.0f;
    if (axis_len2 >= 0.001f) {
        VEC3 scaled;

        t = fn_8028F84C(fn_80052214(&sep.x, &box->vec_0x18.x) / axis_len2, 0.0f, 1.0f);
        fn_80051EE0(&scaled, &box->vec_0x18, t);
        /* `fn_800B0B90` is declared with the `ef` module's `Vec`, a distinct 0xC record with the same
         * layout (`ef.h`); the conversion is a view, not arithmetic. */
        fn_800B0B90((Vec*)&sep, (Vec*)&scaled);
    }
    *param = t;
    return fn_80050EDC(&sep.x);
}

/* 0x8028F938 - build the box `start` -> `end`, then hand `out` the point on it closest to `point`;
 * the return is that point's squared distance from `point`. */
f32 fn_8028F938(VEC3* start, VEC3* end, VEC3* point, VEC3* out) {
    PlBox box;
    VEC3 diff;
    VEC3 pos;
    VEC3 scaled;
    f32 param;
    f32 dist;

    fn_8012A8F8(&box);
    param = 0.0f;
    copyVec3(&box.vec_0x00, start);
    copyVec3(&box.vec_0x0C, end);
    subVec3(&diff, end, start);
    copyVec3(&box.vec_0x18, &diff);
    dist = fn_8028F86C(&box, point, &param);
    fn_80051EE0(&scaled, &box.vec_0x18, param);
    addVec3(&pos, &scaled, start);
    copyVec3(out, &pos);
    return dist;
}

/* 0x802910C0 - `out = in * scale`, with `out` run through the record writer first (as the target
 * does). */
void fn_802910C0(VEC3* out, const VEC3* in, f32 scale) {
    VEC3_ctor(out);
    fn_80051424(&out->x, &in->x, scale);
}

/* 0x802919FC - the 2-layer ground query: resets the caller's `LandData` and asks for the region
 * byte's layers. */
s32 fn_802919FC(_PLW* self, VEC3* pos, LandData* land, f32* out, u32 kind) {
    fn_802977E4(land);
    return hit_ground_comon(pos, self->area_0x16, land, out, kind, 2, 0, 0, 0);
}

} /* extern "C" */

/* 0x8029163C - the ground query without the hit flag: forwards to `GetGroundHit2`. */
f32 GetGroundHit(nw4r::math::VEC3* pos, u32 ground, u8 flag) {
    u8 hit;

    return GetGroundHit2(pos, ground, flag, &hit);
}

extern "C" {

/* 0x80291B08 - the fixed-layer ground query: forwards the caller's `LandData`/out/kind and the
 * `_PLW`'s effect key, asking for all 32 layers and no surface data. */
s32 fn_80291B08(_PLW* self, VEC3* pos, LandData* land, f32* out, u32 kind) {
    return hit_ground_comon(pos, self->effect_key_0x1A4, land, out, kind, 32, 0, 0, 0);
}

/* 0x80291BBC - the single-layer probe: builds a scratch `LandData` for the query and reports both
 * the query's verdict and the layer it hit. */
s32 fn_80291BBC(VEC3* pos, u8 flag, u16* hit_layer, f32* out, u32 kind) {
    LandData land;
    s32 found;

    fn_8012A624(&land);
    fn_802977E4(&land);
    *hit_layer = 0;
    found = hit_ground_comon(pos, flag, &land, out, kind, 1, 0, 0, 0);
    *hit_layer = land.field_0x00;
    return found == 1;
}

/* 0x80291C50 - the kind-only query: no layer mask, no surface data. */
void fn_80291C50(VEC3* pos, u8 flag, LandData* land, u32 kind) {
    f32 out;

    fn_802977E4(land);
    hit_ground_comon(pos, flag, land, &out, kind, 0, 0, 0, 0);
}

/* 0x80292468 - construct both vectors of a two-vector pair and return it. */
VEC3* fn_80292468(VEC3* pair) {
    VEC3* p;

    p = pair;
    do {
        VEC3_ctor(p);
        p++;
    } while (p < pair + 2);
    return pair;
}

/* 0x80293A7C - scale `v` by the reciprocal of `divisor` through the in-place scale `fn_800513F0`;
 * the tail call is the whole body. */
void fn_80293A7C(VEC3* v, f32 divisor) {
    return fn_800513F0(v, 1.0f / divisor);
}

/* 0x8029573C - construct one vector in place and return it. */
VEC3* fn_8029573C(VEC3* v) {
    VEC3_ctor(v);
    return v;
}

} /* extern "C" */

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

/* ==== 0x80295EF4-0x80297E34: the list scans and the land table ==== */

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "ef/fn_800CDB2C.h"   /* PlayMode_ck (rule 2: the owner is `ef/fn_800CDB2C.cpp`) */
#include "menu/hit_attack_list_push.h"
#include "Pl/pl_coll.h"   /* the owner of the `.bss` arrays `pl_land_data` / `pl_hit_id_list` (rule 2) */

/* ------------------------------------------------------------------------------------------------ *
 * List scans: the registry's own id lists and the two helpers that walk a caller's array.
 * ------------------------------------------------------------------------------------------------ */

/* Whether `key` occurs in the first `count` words of `list`. */
extern "C" u32 fn_802961F8(u32 key, u32* list, s32 count)
{
    while (count > 0) {
        if (key == *list) {
            return 1;
        }
        list++;
        count--;
    }
    return 0;
}

/* The same scan over the global id array. */
extern "C" u32 fn_80296228(u32 key, s32 count)
{
    u32* list = pl_hit_id_list;

    while (count > 0) {
        if (key == *list) {
            return 1;
        }
        list++;
        count--;
    }
    return 0;
}

/* ------------------------------------------------------------------------------------------------ *
 * The land table (`.bss` `pl_land_data`, 15 x 0x88).
 * ------------------------------------------------------------------------------------------------ */

/* Zeroes the four vectors of one land record. */
extern "C" PlLandCell* fn_80297D9C(PlLandCell* self)
{
    VEC3_ctor(&self->vec_0x0C);
    VEC3_ctor(&self->vec_0x18);
    VEC3_ctor(&self->box_min_0x48);
    VEC3_ctor(&self->box_max_0x54);
    return self;
}

/* Clears the whole table. */
extern "C" void fn_8029708C(void)
{
    memset(pl_land_data, 0, 0x7F8);
}

/* ------------------------------------------------------------------------------------------------ *
 * The 0x14-byte position record.
 * ------------------------------------------------------------------------------------------------ */

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

/* Copies one position record. */
extern "C" void fn_80297BE4(LandData* dst, LandData* src)
{
    *dst = *src;
}

/* ------------------------------------------------------------------------------------------------ *
 * The land record's two 3-D grids and its AABB.
 * ------------------------------------------------------------------------------------------------ */

/* Whether `pos` lies strictly inside the record's horizontal box. */
extern "C" u32 fn_80296368(nw4r::math::VEC3* pos, PlLandCell* land)
{
    if (!(pos->x < land->box_max_0x54.x && pos->x > land->box_min_0x48.x &&
          pos->z < land->box_max_0x54.z && pos->z > land->box_min_0x48.z)) {
        return 0;
    }
    return 1;
}

/* Whether a grid-B coordinate triple is inside the record's second grid. */
extern "C" u32 fn_802963B0(s32 x, s32 y, s32 z, PlLandCell* land)
{
    if ((u32)x >= (u32)land->dim_x_0x74 || x < 0 || (u32)y >= (u32)land->dim_y_0x70 || y < 0 ||
        (u32)z >= (u32)land->dim_z_0x6C || z < 0) {
        return 0;
    }
    return 1;
}

/* The same range check for the record's first grid. */
extern "C" u32 fn_802963FC(s32 x, s32 y, s32 z, PlLandCell* land)
{
    if ((u32)x >= (u32)land->dim_x_0x38 || x < 0 || (u32)y >= (u32)land->dim_y_0x34 || y < 0 ||
        (u32)z >= (u32)land->dim_z_0x30 || z < 0) {
        return 0;
    }
    return 1;
}

/* One cell of the record's second grid, indexed [x][y][z]. */
extern "C" u32 fn_802969A8(s32 x, s32 y, s32 z, PlLandCell* land)
{
    return land->cells_0x78[(x * land->dim_y_0x70 + y) * land->dim_z_0x6C + z];
}

/* One cell of the record's first grid, indexed [x][y][z]. */
extern "C" u32 fn_802969D0(s32 x, s32 y, s32 z, PlLandCell* land)
{
    return land->cells_0x3C[(x * land->dim_y_0x34 + y) * land->dim_z_0x30 + z];
}

/* ------------------------------------------------------------------------------------------------ *
 * The 0x3C-byte box record (`.bss` `pl_hit_box`, 10 records).
 * ------------------------------------------------------------------------------------------------ */

/* Zeroes the four vectors of one box record. */
extern "C" void* fn_80297DE8(void* self)
{
    PlHitBox* box = (PlHitBox*)self;

    VEC3_ctor(&box->vec_0x08);
    VEC3_ctor(&box->vec_0x14);
    VEC3_ctor(&box->vec_0x20);
    VEC3_ctor(&box->vec_0x2C);
    return self;
}
