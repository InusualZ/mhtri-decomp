/*
 * Pl/pl_hit_sphere.cpp - the player's shape hit tests: the box builders, the point-in-sphere test, the sphere, capsule,
 *   segment, plane and triangle contact/push tests, and the triangle reach test the enemy joints use.
 * RANGE. .text 0x8028F44C-0x8029135C (25 functions, in address order below); .sdata2 0x8079A310-0x8079A330 (the pool:
 *   0.5, 0.001, 0, 1, 1e-5, -1, 10.01, then the align-8 pad), extab 0x80013224-0x800132DC, extabindex
 *   0x800309B4-0x80030AC8.  No `.data`/`.bss`: every data section of the band belongs to `Pl/pl_coll.cpp`.
 * SEAM. One TU with `Pl/pl_coll.cpp`'s former head: MWCC dedupes a TU's float literals (three functions using 0.5f
 *   emit one pool word), and the band's pool holds 0.5/0.001/1/1e-5/-1 twice - at 0x8079A310 (read only by
 *   0x8028F44C..0x8029135C, last `pl_coll_tri_hit_ck`) and from 0x8079A330 (read only by 0x80291664 on, first
 *   `GetGroundHit2`), 0x8079A32C and 0x8079A36C unreferenced (align-8 pads).  The two land-grid loaders between the
 *   runs read no pool; they fill `pl_land_data`, so they sit with its owner.  The `extabindex` records tile across
 *   0x8028F44C..0x8029135C in address order (`hit_point_sphr` has none).
 * NAMES. The file name is a GUESS from `hit_point_sphr`, the range's one mangled name; `pl_coll_tri_hit_ck` is the
 *   registry's.  GUESS (from the record each fills or the test each performs): `pl_box_set`, `pl_segment_box_set`,
 *   `pl_capsule_box_set`, every `pl_*_hit`/`_contact`/`_push`/`_closest_*` name, `pl_clampf`, `pl_plane_project`,
 *   `vec3_scale_to`.
 * FLAGS. The Pl block's flags, `-fp_contract off` included (the library-wide evidence): the fifteen Pl target
 *   objects carry 0 fused ops (`fmadds`/`fmsubs`/`fnmadds`/`fnmsubs`) against 233 `fmuls` and 457 `fadds`/`fsubs`.
 *   Under `-fp_contract on` `hit_point_sphr` fuses retail's three `fmuls` and two `fadds` into two `fmadds`; under
 *   `off` it emits retail's sequence.
 * RESIDUALS. none.
 * SHAPES. `pl_box_set` takes typed `PlBox*` parameters (`&b->vec_0x0C`), which MWCC emits as immediate offsets; a
 *   `void*`-plus-cast form CSEs the three reaches into a saved register.  `pl_segment_box_set` declares its locals
 *   `dir`, `center`, `sum` in that order: MWCC places them at 0x20, 0x14 and 0x8 as retail does.
 *   `pl_capsule_box_set` takes a non-const `PlCapsule*` (playbook 132): retail stores the `vec3_distance` result to
 *   `reach` (+0x34), forwards the reload as `frsp f1,f1`, loads `capsule->radius` and stores the sum.  Through a
 *   `const PlCapsule*` the first backend `common-subexpression-elimination` pass (dump 04, after unreachable-block
 *   deletion) treats the radius load as unable to alias the store and deletes the store as dead (0x21C of 0x220,
 *   and the `extabindex` size word 0xC0 against 0xC4); through the plain pointer the same pass keeps it.  No
 *   per-function pragma restores it (`opt_dead_assignments`, `opt_common_subs`, `opt_lifetimes`, `opt_propagation`,
 *   `opt_dead_code`, `global_optimizer`, `optimization_level 2` all measure 97.96), nor does giving both parameters
 *   one type or embedding `PlCapsule` in `PlCapsuleBox`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"              /* the Pl module header; pulls in `ef.h`'s `VEC3_ctor` and `ef_fabsf` */
#include "mh3_pad/vec3.h"   /* copyVec3 (the owner's header, rule 2) */
#include "fn_8004CAD8.h"    /* subVec3 / PSVECSubtract / addVec3 / vec3_scale / vec3_distance and the rest (rule 2) */
#include "ef/fn_800AEE48.h"  /* `vec3_sub_assign` (rule 2) */
#include "Pl/fn_80288CEC.h" /* `PlBox`, shared with `Pl/pl_motion.cpp` and `Pl/pl_coll.cpp` (rule 1) */
#include "Pl/fn_8028F66C.h"  /* `em_pl_box_init` (rule 2) */
#include "ef/effect.h"    /* `eft_vec_ctor` (rule 2) */
#include "ef/ef_vec3_dist_sq.h"
#include "Pl/pl_hit_sphere.h"
#include "Pl/pl_coll.h"   /* `pl_tri2d_cross` / `pl_tri2d_point_ck` (rule 2) */

/* The moved bodies' prototypes (C linkage: the map rows are plain names). */
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
s32 pl_segment_sphere_hit(PlSegmentBox* seg, HitSphere* sphere, VEC3* enter, VEC3* exit);
s32 pl_segment_capsule_hit(PlSegmentBox* seg, PlCapsuleBox* capsule, VEC3* enter, VEC3* exit);
u32 pl_segment_plane_hit(PlSegment* seg, PlPlane* plane, VEC3* out);
s32 pl_triangle_closest_point(VEC3* point, VEC3* a, VEC3* b, VEC3* c, VEC3* out);
}

extern "C" {

/* 0x8028F44C (0x68): builds the first 0x24 bytes of a box from two vectors and their difference. */
void pl_box_set(PlBox* a, PlBox* b) {
    f32 diff[3];

    copyVec3(&b->vec_0x00, &a->vec_0x00);
    copyVec3(&b->vec_0x0C, &a->vec_0x0C);
    subVec3(diff, &a->vec_0x0C, &a->vec_0x00);
    copyVec3(&b->vec_0x18, (const nw4r::math::VEC3*)diff);
}

/* 0x8028F4B4 (0xA4): fills `box` from the segment's endpoints. */
void pl_segment_box_set(const PlSegment* seg, PlSegmentBox* box) {
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 center;
    nw4r::math::VEC3 sum;

    copyVec3(&box->start, &seg->start);
    copyVec3(&box->end, &seg->end);
    subVec3(&dir, &seg->end, &seg->start);
    copyVec3(&box->dir, &dir);
    addVec3(&sum, (VEC3*)&seg->start, (VEC3*)&seg->end);
    vec3_scale(&center, &sum, 0.5f);
    copyVec3(&box->center, &center);
    box->half_length = vec3_distance(&box->start, &box->center);
}

/* 0x8028F558 (0xC4): fills `box` from the capsule. */
void pl_capsule_box_set(PlCapsule* capsule, PlCapsuleBox* box) {
    copyVec3(&box->start, &capsule->start);
    copyVec3(&box->end, &capsule->end);
    box->radius = capsule->radius;
    PSVECSubtract((f32*)&box->dir, (const f32*)&capsule->end, (const f32*)&capsule->start);
    box->center.x = 0.5f * (capsule->start.x + capsule->end.x);
    box->center.y = 0.5f * (capsule->start.y + capsule->end.y);
    box->center.z = 0.5f * (capsule->start.z + capsule->end.z);
    box->reach = vec3_distance((void*)&capsule->start, &box->center);
    box->reach += capsule->radius;
}

}

/* 0x8028F61C (0x50): 1 when `point` lies inside the sphere of `radius` around `center`. */
u32 hit_point_sphr(nw4r::math::VEC3* point, nw4r::math::VEC3* center, f32 radius) {
    f32 dx = center->x - point->x;
    f32 dy = center->y - point->y;
    f32 dz = center->z - point->z;
    return dz * dz + (dx * dx + dy * dy) <= radius * radius;
}

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

