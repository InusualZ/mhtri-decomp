/* ef/eft018.h - the declarations of `ef/eft018.cpp`'s effect spawn helpers the enemy/effect units call
 * (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_EF_EFT018_H
#define MHTRI_EF_EFT018_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8010D2B0 - the joint-effect spawner: r3 the work record, r4/r5 two `u8` selectors the callers zero-extend
 * (`clrlwi`), r6 a scalar and f1 the scale. */
void eft_spawn_pos_in_area(void* self, u8 a, u8 b, s32 c, f32 scale);

void eft_pl_weapon_follow_spawn(struct Plw* self);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT018_H */
