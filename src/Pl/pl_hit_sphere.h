/* The player hit-sphere unit `Pl/pl_hit_sphere.cpp`: the effect-quad hit test and the box compare its callers drive.
 * Both map names are bare `fn_` stems, so they take C linkage.
 */
#ifndef MHTRI_PL_PL_HIT_SPHERE_H
#define MHTRI_PL_PL_HIT_SPHERE_H

#include "types.h"

struct EmEffectSegment;
struct EmEffectQuad;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8028F4B4 - whether the segment crosses the quad. */
u8 fn_8028F4B4(const struct EmEffectSegment* seg, const struct EmEffectQuad* quad);

/* 0x8028F558 - compares two enemy scratch blocks (`enemy/em005_act.cpp`, `enemy/em007_act.cpp`). */
/* untyped: two caller-owned scratch blocks, opaque to the callee's callers */
s32 fn_8028F558(void* a, void* b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_HIT_SPHERE_H */
