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

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_80138074_H */
