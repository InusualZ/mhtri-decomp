/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/em024_ai`'s `em024_action11_state5_ck`, which `camera/camera_main.cpp` calls. */
#ifndef MHTRI_ENEMY_EM024_ACTION11_STATE5_CK_H
#define MHTRI_ENEMY_EM024_ACTION11_STATE5_CK_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8034F138 (enemy/em024_ai): whether the monster runs action 11 in state 5. */
s8 em024_action11_state5_ck(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM024_ACTION11_STATE5_CK_H */
