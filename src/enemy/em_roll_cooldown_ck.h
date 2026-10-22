/* enemy/em_roll_cooldown_ck.h - the declaration of `em_roll_cooldown_ck`, which `enemy/em_common.cpp` owns (docs/plan.md
 * 6.5 rule 2); the signature is the one `enemy/fn_80165FC8.h` gives its callers. */
#ifndef MHTRI_ENEMY_EM_ROLL_COOLDOWN_CK_H
#define MHTRI_ENEMY_EM_ROLL_COOLDOWN_CK_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif
/* 0x80129A1C - while `*cooldown` has run out: 1 when `roll % 100 < percent`, else re-arms `*cooldown` to 900. */
u32 em_roll_cooldown_ck(struct _ENEMY_WORK* self, u16 roll, u32 percent, s16* cooldown);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_ROLL_COOLDOWN_CK_H */
