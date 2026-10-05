/*
 * Pl/pl_coll.cpp - the player actor's ground/hit collision band: the ground and hit queries, the land table and the hit-id list.
 *
 * `.text` 0x8028F66C..0x80297E34 (about 80 functions), `.bss` 0x1060 B (the `Pl/pl_coll.h` arrays), `.ctors` 4 B, `.data` 0x40 B,
 * `.sbss` 8 B, `.sdata` 8 B, `.sdata2` 0x6C B, extab 0x218 B and extabindex 0x324 B.  Phase 4 fold/recut:
 * `Pl/fn_8028F66C` (whole), the head of `Pl/fn_80295EF4` (0x80295EF4..0x80297E34) and the data-only unit `Pl/bss_pool` (the
 * `.bss` arrays the band's static initialiser constructs) are one TU of the candidate; the tail of `Pl/fn_80295EF4` goes to
 * `menu/menu_item`.
 *
 * Flags: `cflags_pl` as the former units.
 */

/* ==== absorbed from Pl/fn_8028F66C.cpp (0x8028F66C..0x80295998) ==== */
/*
 * Pl/fn_8028F66C.cpp - the ground/hit collision TU of the `Pl` band.  `.text`
 * 0x8028F66C-0x80295EF4 (53 functions, 0x6888 B), extab 0x8001323C-0x800133CC (50 records) and
 * extabindex 0x800309D8-0x80030C30 (50 records); all three ranges are registered in splits.txt.
 * Nothing is claimed out of `.data`/`.sdata`/`.sdata2` (invariant 8.4): the unit's `.sdata2` pool is
 * still emitted by the `auto_*` objects, and the `.bss` collision-work tables the query half walks
 * (`pl_move_work`, `pl_hit_box`, `pl_land_data`, `pl_hit_id_list`, ...) are owned by this unit
 * (its header `Pl/pl_coll.h` declares them).
 *
 * Extent.  The LEFT edge is a real TU boundary, pinned by the `.sdata2` run: the preceding unit
 * (`Pl/fn_80288CEC.cpp`) owns 0x8079A270-0x8079A314 and this unit's pool starts exactly at
 * 0x8079A314 (its first function loads `lbl_8079A314` = 0.001f as its first constant).  The RIGHT
 * edge (0x80295EF4) is the proposal's `--max-bytes` cap, not a TU boundary - the discovery brief
 * says so and the evidence agrees: the pool keeps growing past it (0x8079A368, 0x8079A370, ... are
 * first used by the functions after 0x80295EF4) and no `__FILE__` string exists anywhere in the
 * band (scanning the DOL for `[ -~]{3,}\.cpp` yields 99 source names, not one `pl_*` or `hit_*`).
 * The range is worked as one unit and this header says the seam is unproven.
 *
 * Module (`Pl`) - class 3 of the brief's evidence order.  Class 1 fails (no `__FILE__` string,
 * above) and class 2 fails (`python tools/symbols/dumpmap.py lookup` answers `zz_029xxxx_` for 47
 * of the 53 addresses; the six it does name are the functions the symbol map already spells out).
 * Class 3: both bracketing registered units of the band are `Pl` (`Pl/fn_80288CEC.cpp` below it and
 * the `Pl` band's `.sdata2` above it), the unit reads the Pl-band global `lbl_80794B58`, and its
 * exported entry points are the ones the Pl/ef/enemy units call (`GetGroundHit2` from
 * `Pl/pl_act.cpp`, `GetGroundHit` from `ef/eft001.cpp`, `findInterSection*` from `enemy/*`).
 *
 * File name - class 4.  Nothing supports a file name: no `__FILE__` string, no runtime-dump name,
 * and no dominant prefix in the siblings' scheme.  The stem therefore stays the map's
 * `fn_8028F66C`, matching the sibling class-4 registrations `Pl/fn_80229ECC.cpp`,
 * `Pl/fn_80241558.cpp` and `Pl/fn_80288CEC.cpp`.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for 47 of this range's 53 functions (checked
 * with `tools/symbols/dumpmap.py lookup <addr>` for every address of the inventory - 47 answer
 * `zz_<addr>_`, and the six real names the dump knows (`GetGroundHit`, `GetGroundHit2`,
 * `hit_ground_comon`, `findInterSection`, `findInterSection2`, `findInterSection3`) are already the
 * map's spellings).  The bare `fn_XX` definitions are the map's own placeholder names, so they are
 * defined inside an `extern "C"` block; the six mangled ones stay at C++ scope with the signature
 * their mangling encodes (rule 9).
 *
 * Flags: `cflags_pl` (this lib).  The unit's `.text` packs functions 4 B apart (0x8028F758,
 * 0x8028F84C ...) and every framed function carries an 8-byte extab record, i.e. the same
 * `-O3 -inline noauto -opt nopeephole -Cpp_exceptions on` line the lib settled on for
 * `Pl/fn_80288CEC.cpp`, whose `.text` ends where this one starts; 15 of the 16 written functions are
 * byte-identical under it.
 *
 * RESIDUAL (this pass).  16 of the 53 functions are written - 15 byte-identical and `fn_802910C0`
 * at 90.48 % - and the other 37 are left at 0 % rather than guessed.  The blockers, in the order
 * they were hit:
 *   * `fn_802910C0`'s residual is a scheduler placement, not a source shape: the target saves the
 *     `in` pointer (r4) *after* the float parameter (f1) and ours saves it before, so one `mr` pair
 *     lands two slots early.  Three source variants (the ratio inline, a named `ratio`, a named
 *     scale local) all keep 90.48 %; 84 B / 22 instructions either way.
 *   * `fn_80291A70` and `fn_80291B48` read `_PLW` bytes that have no field: +0x1E1 (inside
 *     `equipB2.deco_level`) and +0x004 (inside `pl.h`'s `unk003[2]` run).  `fn_80291A70`
 *     also zeroes a `_HIT_TENJO_DATA`'s +0x00/+0x01 bytes and its +0x04 float, which needs that
 *     record's layout.  They wait for those fields rather than re-cut `pl.h` from here.
 *   * The three big hits - `GetGroundHit2` (0x80291664, 920 B), `hit_ground_comon` (0x80291CD0,
 *     956 B) and `findInterSection3` (0x80295998, 1372 B), 3.2 KB together and the largest single
 *     win left - walk the `pl_land_data` (0x88-stride) land table and the `pl_hit_id_list` id list
 *     with a 0x40-byte hit record reading +0x00/+0x02/+0x07/+0x08/+0x10/+0x14/+0x1C/+0x20/+0x28/
 *     +0x2C/+0x30/+0x34/+0x38/+0x70.  Those bodies also carry the unit's only nested-loop dispatch,
 *     and `LandData` (reconstructed here from `fn_802977E4`, below) is a *different* record, so the
 *     hit record has to be named first.  `_HIT_TENJO_DATA` is still only a forward declaration.
 *   * The 21-function query half `fn_8028FA20`-`fn_80290D08` (0x8028FA20-0x80290F?) takes a record
 *     whose VEC3 fields are at +0x18/+0x1C/+0x28 with radii at +0x0C and +0x34 - `fn_802900F8`'s
 *     `hit_point_sphr(arg0 + 0xA0, arg1, arg0->0x34 + arg1->0xC)` is the cheapest way in, and +0xC
 *     is the `HitSphere` this file already defines.
 *   * `fn_80292940` (0x80292940, a 0x3C-byte record copy), `fn_80295544` (a record with a `VEC3` at
 *     +0x08), `fn_8029576C`/`fn_80295578`/`fn_80295290` (0x80295290-0x8029576C) and
 *     `fn_802924C0`/`fn_802929DC`/`fn_80293504`/`fn_80293A88`/`pl_coll_sweep_ck`/`fn_80294538`/
 *     `fn_80294B64`/`fn_80294EFC`/`fn_802950D8` need their record types settled the same way.
 *   * `fn_8028F938`'s two callees whose owners' headers cannot declare them live in
 *     `Pl/fn_8028F66C.h` (see that file for the measured `(10197)` reason).
 */

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

/* ==== recut from Pl/fn_80295EF4.cpp (0x80295EF4..0x80297E34) ==== */
/*
 * Naming note: the symbol map spells 97 of this range's 99 functions as bare `fn_XXXXXXXX` and
 * names the other two with a mangling (`get_hit_id__Fv`) or a plain name; the map is the only
 * evidence for a name here - no `__FILE__` string covers the band (the DOL's string pool jumps from
 * `enemy_control.cpp` at 0x805A1BB8 straight to `menu_item.cpp` at 0x805CDFC8) and
 * `tools/symbols/dumpmap.py lookup` answers a `zz_XXXXXXXX_` placeholder for every in-range address
 * (checked for 0x80295EF4, 0x8029B918, 0x8029F204 and the range's other endpoints).
 *
 * Pl/fn_80295EF4.cpp - the hit/land query band of the `Pl` module.  `.text` 0x80295EF4-0x8029F3C8
 * (99 functions, 0x94D4 B).  Nothing else is claimed: the band's `.data` tables and both pools
 * (`.sdata2` 0x8079A330-0x8079A3C8, `.sdata` 0x80794B58) stay with the `auto_*` objects, and so does
 * the `.ctors` word for the static initializer `fn_80297C30` (invariant 8.4 - never claim a range the
 * object does not emit).
 *
 * The seam is unproven, and it is a `--max-bytes` cut, not a TU boundary (the brief says so and the
 * evidence agrees): the band's `.sdata2` run 0x8079A330-0x8079A3C8 is referenced from 0x80291664
 * through 0x8029F204 without a single run break, and the static initializer at 0x80297C30 (inside
 * this range) constructs arrays whose element constructors are the *previous* proposal's functions
 * (`fn_80295544` at 0x80295544) - i.e. 0x8028F66C-0x8029F3C8 and their neighbours are very likely one
 * original object.  Both halves are registered separately only because discovery cut the run; the
 * merge is requested in the outbox (`shared-file`).
 *
 * Module `Pl` - class 3 of the brief's evidence order.  Class 1 (a `__FILE__` string) and class 2 (a
 * real runtime-dump name) both fail (above).  Class 3 is conclusive: the band's callees are the Pl
 * family (`get_move_work_adrs`, `_PLW` fields, `Pl_frame_check`) and its record vocabulary is the one
 * the map already names inside the Pl band (`_HIT_W`, `PlLandCell`, `hit_ground_comon`, `get_hit_id`,
 * `body_set`), with the registered sibling `Pl/fn_80288CEC.cpp` ending at 0x8028F66C straight before
 * it.  File name - class 4: nothing names a file, so the stem stays the map's `fn_80295EF4`, the
 * sibling class-4 pattern of `Pl/fn_80229ECC.cpp` / `Pl/fn_8024158.cpp` / `Pl/fn_80288CEC.cpp`.
 *
 * Language: C++ (mangled `get_hit_id__Fv` is defined here, and the range reaches mangled callees).
 * Flags: `cflags_pl` (the lib's, settled by `Pl/pl_act.cpp`/`pl_master.cpp`/`pl_skill.cpp`).
 *
 * RESIDUAL (this pass).  24 of the range's 99 functions are written and measured; the row is at
 * ~4 % of the range's bytes, with 16 functions byte-identical and the rest of this pass's set between
 * 92 and 99 %.  Per-symbol numbers are in `build/RMHE08/report.json` and the outbox; what is
 * deliberately NOT written, and why:
 *   * `hit_data_apply` (452 B, the hit-record initializer) reads the owner's area byte for all four
 *     owner kinds; kinds 1 and 2 land on offsets `pl.h` does not name (+0x1E1 inside an `_EQUIP`
 *     sub-record, +0x08 inside a run another lane owns), and this pass did not invent names for them.
 *   * The owner work records (`_HIT_W`'s owner, written at +0x470/+0x574/+0x8B4 by the
 *     `fn_802996B4`..`fn_802997E4` family) are not pinned: `pl.h`'s `+0x470` is an `s16` while those
 *     functions store a float triple there, so the field's owner is unresolved (the same blocker the
 *     sibling band records from the other side).
 *   * The band's switches (`fn_80299EF8`, `fn_8029A358`, `fn_8029B918`, `fn_8029E35C`, ...) are the
 *     ceiling the sibling band already recorded: every jump table in the range is zero-filled in the
 *     original DOL, so a sparse switch's case-to-body mapping is not recoverable from the image.
 *   * `.text` only is claimed.  The split does hand the unit's target object the `.ctors` word
 *     (4 B), `extab` (0x240 B) and `extabindex` (0x360 B) that bracket the range, and our object does
 *     emit the extab of the functions it defines - but the ranges are not registered here because the
 *     `.text` cut is not a TU boundary (see above) and this source states the static initializer
 *     `fn_80297C30` as an explicit function rather than a file-scope object (invariant 8.4).
 *
 * Measured residuals in the written set (best variant landed for each):
 *   * `fn_802961F8` / `fn_80296228` (95.0 / 95.71): retail's loop guard is `cmplwi r5,0` + `ble`
 *     where MWCC emits `cmpwi` + `beq` from every shape tried (`while`, `for`, `for (;;)`+break,
 *     `u32` and `s32` counts, an explicit pre-loop guard - the last one costs 12 bytes).
 *   * `fn_8029F084` (92.41): retail carries an unused accumulator (`li r5,0` then `+9` per outer
 *     iteration) that MWCC's dead-code pass removes from every source spelling tried; ours is 4 B
 *     short.  The loop shape itself (two unrolled ten-entry rows) is the one that reproduces.
 *   * `fn_802963B0` / `fn_802963FC` / `fn_802969A8` / `fn_802969D0` (98.32 / 98.32 / 99.0 / 99.0):
 *     instruction-for-instruction equal but for the operand order of one `add` in the index
 *     arithmetic, and the branch form of the range checks (`||`-chain vs `&&`-chain inversion).
 *   * `fn_80296368` (98.78), `fn_8029A140` (68.61): the same class - only the branch sense of one
 *     `fcmpo` differs.  `fn_8029A140`'s tail block is laid out in the other order (4 B).
 */

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "ef/fn_800CDB2C.h"   /* PlayMode_ck (rule 2: the owner is `ef/fn_800CDB2C.cpp`) */
#include "menu/hit_attack_list_push.h"
#include "Pl/pl_coll.h"   /* the owner of the `.bss` arrays `pl_land_data` / `pl_hit_id_list` (rule 2) */

/* The pool constants this pass needs: 0.0f (`lbl_8079A330`), 1.0f (`lbl_8079A338`) and the pair
 * `lbl_8079A380` = 0.0f / `lbl_8079A3A4` = 1.0f.  They are declared, never defined (invariant 8.4). */

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
