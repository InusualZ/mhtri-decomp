/* Leaf header (docs/plan.md 6.5 rule 2): `em_static_data_latch`, which `enemy/em_motion_update` owns (the name is a registry proposal of lane
 * enemy-control; the map row is renamed when it lands).  C linkage. */
#ifndef MHTRI_ENEMY_EM_STATIC_DATA_LATCH_H
#define MHTRI_ENEMY_EM_STATIC_DATA_LATCH_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80137C20 - latches the enemy data's static values into the work. */
void em_static_data_latch(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif
