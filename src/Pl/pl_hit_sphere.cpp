/*
 * Pl/pl_hit_sphere.cpp - the player actor's box and sphere hit helpers: the box builders and `hit_point_sphr`.
 * RANGE. .text 0x8028F44C-0x8028F66C (4 functions); .sdata2 0x8079A310-0x8079A318, extab, extabindex.
 * NAMES. The file name is a GUESS from `hit_point_sphr`, the range's one map name.  GUESS: `pl_box_set` (0x8028F44C),
 *   `pl_segment_box_set` (0x8028F4B4) and `pl_capsule_box_set` (0x8028F558), from the records each fills
 *   (`PlBox`, `PlSegmentBox`, `PlCapsuleBox`).
 * FLAGS. The Pl block's flags, `-fp_contract off` included (the library-wide evidence): the fifteen Pl target
 *   objects carry 0 fused ops (`fmadds`/`fmsubs`/`fnmadds`/`fnmsubs`) against 233 `fmuls` and 457 `fadds`/`fsubs`.
 *   Under `-fp_contract on` `hit_point_sphr` fuses retail's three `fmuls` and two `fadds` into two `fmadds`; under
 *   `off` it emits retail's sequence.
 * RESIDUALS.
 *  - the 0.5 is our own pool literal (`@280`) where retail names `lbl_8079A310`: relocdiff 22/24 by name, values equal.
 *  - the unit is a fragment of `Pl/pl_coll.cpp`'s TU and cannot flip alone: the claimed `.sdata2` 0x8079A310-0x8079A318
 *    holds 0.5 (read only by `pl_segment_box_set`/`pl_capsule_box_set`) then 0.001 at 0x8079A314 (read only by eight
 *    `pl_coll` functions, first `pl_sphere_contact`).  MWCC gives every `.sdata2` pool alignment 8 (ours included), so
 *    `pl_coll`'s pool - which also holds the `lfd` double at 0x8079A370 - cannot start at 0x8079A314; one pool
 *    [0.5, 0.001, ...] from 0x8079A310 in first-use order is the only layout that places both words.  Ours emits 0x4 of
 *    the claimed 0x8, and a trial flip ends in `undefined: 'lbl_8079A314'` from `pl_coll.o`.  The fold is filed as a
 *    seam request; nothing here compensates for the missing word.
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
#include "mh3_pad/vec3.h"   /* copyVec3 (the owner's header, rule 2) */
#include "fn_8004CAD8.h"    /* subVec3 / PSVECSubtract / addVec3 / vec3_scale / vec3_distance (rule 2) */
#include "Pl/fn_80288CEC.h" /* `PlBox`, shared with `Pl/pl_motion.cpp` and `Pl/pl_coll.cpp` (rule 1) */
#include "Pl/pl_hit_sphere.h"

/* The box builders' map names are plain, so the definitions carry C linkage; `hit_point_sphr` is mangled. */
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
