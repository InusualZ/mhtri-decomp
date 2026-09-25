/* The enemy animation/user-data unit `enemy/fn_80138074.c` (0x80138074..0x8013ACC4).
 *
 * Declarations moved here from the consumer units' `src/` files (docs/plan.md 6.5 rule 2:
 * an extern lives with the TU that owns the symbol).  The signature set is what the
 * consumers used; where only the parameter spelling differed the wider form is kept.
 */
#ifndef MHTRI_ENEMY_FN_80138074_H
#define MHTRI_ENEMY_FN_80138074_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

u32 fn_8013A884(struct _ENEMY_WORK* self, s32 value);
u32 fn_8013A8B4(struct _ENEMY_WORK* enemy, s32 a, s32 b);
u8 fn_8013A900(struct _ENEMY_WORK* enemy);
void fn_8013AAC4(struct _ENEMY_WORK* enemy);
u32 fn_8013AB74(struct _ENEMY_WORK *self, u32 a, u32 b);

/* Declarations moved here from `enemy/fn_80176C58.cpp` (docs/plan.md 6.5 rule 2). */
s32 fn_801391E8(struct _ENEMY_WORK* self);
void fn_801390FC(struct _ENEMY_WORK* self, void* arg1);
/* 0x8013A654 - r3 (`self`, the owner spells it `ResUserDataAc*`) and r4, stored at +0x8 of the
 * record at +0x4; moved here from `enemy/fn_801550FC.cpp` on landing (rule 2). */
void fn_8013A654(struct _ENEMY_WORK* self, u32 a);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_80138074_H */
