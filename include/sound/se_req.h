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

/* 0x800DFDCC - the SE pool's per-slot refresh (`SePool` is the pool `lbl_80794978` points at). */
void fn_800DFDCC(struct SePool* self, s32 arg);
/* 0x800E0428 - the positional request that takes the SE work object and a world position. */
u32 fn_800E0428(struct _se_w* work, nw4r::math::VEC3* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SE_REQ_H */
