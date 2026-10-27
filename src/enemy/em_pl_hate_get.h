/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `enemy/em_common.cpp` defines, for `enemy/fn_801B7020.cpp`. */
#ifndef MHTRI_ENEMY_EM_PL_HATE_GET_H
#define MHTRI_ENEMY_EM_PL_HATE_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8012C0EC: the work's hate word for player `index` plus the enemy data's condition bonuses, clamped at 0. */
s32 em_pl_hate_get(struct _ENEMY_WORK* enemy, u32 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_PL_HATE_GET_H */
