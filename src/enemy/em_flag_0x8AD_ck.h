/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/em_common.cpp`'s `em_flag_0x8AD_ck`, the one declaration `ai/ai_npc.cpp` needs.
 * The band header `unsplit/enemy.h` declares it returning `void`; this is the answer the body gives. */
#ifndef MHTRI_ENEMY_EM_FLAG_0X8AD_CK_H
#define MHTRI_ENEMY_EM_FLAG_0X8AD_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801321B0 (0x14): whether the enemy's +0x8AD flag byte is set */
u32 em_flag_0x8AD_ck(struct _ENEMY_WORK* enemy);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_FLAG_0X8AD_CK_H */
