/*
 * Declarations owned by `sound/se_req.cpp` (docs/plan.md 6.5 rule 2): the SE request helpers of 0x800DD40C..0x800E0504 that the SE cluster
 * (`sound/fn_800D7F54.cpp`) calls.  The records are only forward-declared: that unit and `sound/se.h` each carry a view of them.
 */
#ifndef MHTRI_SOUND_SE_REQ_H
#define MHTRI_SOUND_SE_REQ_H

#include "types.h"
#include "nw4r/math.h"

struct _se_w;
struct SePool;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800DFDCC - runs the area's sound points for one frame (`SePool` is the pool `se_pool` points at). */
void se_point_frame(struct SePool* self, s32 arg);
/* 0x800E0428 - the level the point zone around `pos` gives the current area (`work` is the SE pool). */
u8 se_point_zone_level_get(struct _se_w* work, nw4r::math::VEC3* pos);
/* 0x800DD40C - starts the pool's point voice `kind` (0..2 pick voices 9..11) at `pos`.  GUESS name. */
void se_point_req(u8 kind, nw4r::math::VEC3* pos);

/* The player SE requests the eft029 effects play (r3 the player work, whose +0xAFC SE work they take; `ef/eft029_fx.cpp`
 * calls them). */
struct _PLW;
void pl_se_req_charge_end(struct _PLW* plw);
void pl_se_req_charge_start(struct _PLW* plw, u8 kind);
void pl_se_req_charge_level(struct _PLW* plw, u8 kind);
void pl_se_req_charge_7(struct _PLW* plw);
void pl_se_req_charge_end_b(struct _PLW* plw);
void pl_se_req_charge_2(struct _PLW* plw);
void pl_se_req_charge_burst(struct _PLW* plw, nw4r::math::VEC3* pos);
void pl_se_req_charge_tip(struct _PLW* plw, nw4r::math::VEC3* pos);
/* 0x800DD62C - requests SE 8 (18 when the position test holds) for the actor at `pos` (`Pl/pl_act_class3.cpp`). */
void pl_se_req_charge_hit(struct _PLW* plw, nw4r::math::VEC3* pos);
/* 0x800DD514 - requests bank 46's SE 13 at `pos`.  GUESS name. */
void se_bank46_id13_req(nw4r::math::VEC3* pos);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x800DDCB8 - requests the map SE `id` at `pos` (`map_se_req__FUcPQ34nw4r4math4VEC3`, C++ linkage: the map name
 * is the mangling, so it is declared outside the `extern "C"` block). */
void map_se_req(u8 id, nw4r::math::VEC3* pos);
/* 0x800DE014 - the capsule-seated shell-SE request (`shell_se_req2__FP5_se_wPQ34nw4r4math7CAPSULEUcUl`, C++
 * linkage: the map name is the mangling, rule 9). */
void shell_se_req2(struct _se_w* work, nw4r::math::CAPSULE* capsule, u8 id, u32 arg);
/* 0x800DE2FC - the position-seated shell-SE request (`shell_se_req__FP5_se_wPQ34nw4r4math4VEC3UcUl`). */
void shell_se_req(struct _se_w* work, nw4r::math::VEC3* pos, u8 id, u32 arg);
#endif

#endif /* MHTRI_SOUND_SE_REQ_H */
