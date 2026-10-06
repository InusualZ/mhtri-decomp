/* ef/fn_8010D1A8.h - the declarations of `ef/fn_8010D1A8.c`'s effect spawn helpers the enemy/effect units call
 * (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_EF_FN_8010D1A8_H
#define MHTRI_EF_FN_8010D1A8_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8010D2B0 - the joint-effect spawner: r3 the work record, r4/r5 two selectors the callers zero-extend (`clrlwi`,
 * so `u8` here; the owner's C body spells them `s8`), r6 a scalar and f1 the scale. */
void eft_spawn_pos_in_area(void* self, u8 a, u8 b, s32 c, f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_FN_8010D1A8_H */
