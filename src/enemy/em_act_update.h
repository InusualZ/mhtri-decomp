/* Leaf header (docs/plan.md 6.5 rule 2): `em_act_update`, which `enemy/em_motion_update` owns (the name is a registry proposal of lane
 * enemy-control; the map row is renamed when it lands).  C linkage. */
#ifndef MHTRI_ENEMY_EM_ACT_UPDATE_H
#define MHTRI_ENEMY_EM_ACT_UPDATE_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8013791C - the enemy's per-frame motion/action driver. */
void em_act_update(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif
