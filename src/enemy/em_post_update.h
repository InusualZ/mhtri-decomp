/* Leaf header (docs/plan.md 6.5 rule 2): `em_post_update`, which `enemy/fn_80138074` owns (the name is a registry proposal of lane
 * enemy-control; the map row is renamed when it lands).  C linkage. */
#ifndef MHTRI_ENEMY_EM_POST_UPDATE_H
#define MHTRI_ENEMY_EM_POST_UPDATE_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80138B60 - the second per-frame pass over a live work. */
void em_post_update(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif
