/* Declarations owned by `ef/fn_8010D1A8.c` (docs/plan.md 6.5 rule 2): the effect spawn helpers the
 * enemy/effect units call that this unit defines.  A consumer includes this header instead of
 * declaring them itself.
 */
#ifndef MHTRI_EF_FN_8010D1A8_H
#define MHTRI_EF_FN_8010D1A8_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8010D2B0 - the joint-effect spawner: r3 the work record, r4/r5 two `s8` selectors, r6 a scalar
 * and f1 the scale (the owner's own definition is
 * `void fn_8010D2B0(void* arg0, s8 arg1, s8 arg2, u32 arg3, f32 farg0)`).  Added with
 * `enemy/fn_80387844.cpp`'s action band (rule 2). */
void fn_8010D2B0(void* self, s8 a, s8 b, u32 c, f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_FN_8010D1A8_H */
