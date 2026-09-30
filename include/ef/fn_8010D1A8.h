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

/* 0x8010D2B0 - the joint-effect spawner: r3 the work record, r4/r5 two selectors the callers zero-extend
 * (`clrlwi`, so `u8`; the owner's C body spells them `s8`), r6 a scalar and f1 the scale.  Measured
 * 2026-09-30: the `u8` spelling raises `enemy/fn_80387844.cpp` (`fn_8038C124` 97.52 -> 98.01,
 * `fn_8038CCA4` 68.46 -> 68.63) and lets `enemy/em024_ai.cpp` match its call site.  Added with
 * `enemy/fn_80387844.cpp`'s action band (rule 2). */
void eft_spawn_pos_in_area(void* self, u8 a, u8 b, s32 c, f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_FN_8010D1A8_H */
