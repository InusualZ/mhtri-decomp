/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/em018_prog.cpp`'s em018_aim_flag2_ck */
#ifndef MHTRI_ENEMY_EM018_AIM_FLAG2_CK_H
#define MHTRI_ENEMY_EM018_AIM_FLAG2_CK_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801923E0 - whether the enemy's aim record has bit 0x02 set. */
u32 em018_aim_flag2_ck(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM018_AIM_FLAG2_CK_H */
