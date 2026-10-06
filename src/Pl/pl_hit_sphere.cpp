/*
 * Pl/pl_hit_sphere.cpp - the player actor's box and sphere hit helpers: the box builders and `hit_point_sphr`.
 * RANGE. .text 0x8028F44C-0x8028F66C (4 functions); .sdata2 0x8079A310-0x8079A318, extab, extabindex.
 * NAMES. The file name is a GUESS from `hit_point_sphr`, the range's one map name.
 * RESIDUALS. `fn_8028F4B4` and `fn_8028F558` (the remaining box builders, 0x8028F4B4-0x8028F61C) are unwritten;
 *   `hit_point_sphr` is partial, with no recorded cause.
 *  - flipcheck: the object emits no `.sdata2` (0x8 claimed); `.text` 0xB0, extab 0x8 and extabindex 0xC against the
 *    claims 0x220, 0x18 and 0x24; every compared section differs.  The `.sdata2` pool shares a literal with
 *    `Pl/pl_coll.cpp`, whose `.text` follows: the two may be one TU (a fold candidate).
 * SHAPES. `fn_8028F44C` takes typed `PlBox*` parameters (`&b->vec_0x0C`), which MWCC emits as immediate offsets; a
 *   `void*`-plus-cast form CSEs the three reaches into a saved register.
 */

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"
#include "Pl/pl_act.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/fn_802693C4.h"
#include "Pl/fn_80288CEC.h" /* `PlBox`, shared with `Pl/pl_motion.cpp` and `Pl/pl_coll.cpp` (rule 1) */
#include "ef/fn_800CDB2C.h"
#include "enemy/em_pop.h" /* quest_flag_8_ck, quest_flag_80_ck (the owner's header, rule 2) */
#include "fn_80047398.h" /* `arena_userdata_apply` (rule 2) */
#include "quest/arenatask.h" /* `arena_user_data_buf` (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "Network/network_pat_control.h" /* the owner's header (rule 2) */
#include "quest/quest_entry.h" /* quest_monsters_release (the owner's header, rule 2) */

extern "C" {
f32 subVec3(void* dst, void* a, void* b);
}

/* Every bare `fn_XXXXXXXX` is the map's own (unmangled) name, so the definitions carry C linkage;
 * `hit_point_sphr` is mangled in the map and stays at C++ scope below. */
extern "C" {

/* 0x8028F44C - build the first 0x24 bytes of a box from two vectors and their cross product. */
void fn_8028F44C(PlBox* a, PlBox* b) {
    f32 cross[3];

    copyVec3(&b->vec_0x00, &a->vec_0x00);
    copyVec3(&b->vec_0x0C, &a->vec_0x0C);
    subVec3(cross, &a->vec_0x0C, &a->vec_0x00);
    copyVec3(&b->vec_0x18, (const nw4r::math::VEC3*)cross);
}

}


/* 0x8028F61C - 1 when `point` lies inside the sphere of `radius` around `center`. */
u32 hit_point_sphr(nw4r::math::VEC3* point, nw4r::math::VEC3* center, f32 radius) {
    f32 dx = center->x - point->x;
    f32 dy = center->y - point->y;
    f32 dz = center->z - point->z;
    return dx * dx + dy * dy + dz * dz <= radius * radius;
}
