/* Leaf header (docs/plan.md 6.5 rule 2): `light_em_slot_free`, which `light/light` owns (the name is a registry proposal of lane
 * enemy-control; the map row is renamed when it lands).  C linkage. */
#ifndef MHTRI_LIGHT_LIGHT_EM_SLOT_FREE_H
#define MHTRI_LIGHT_LIGHT_EM_SLOT_FREE_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802C070C - frees the enemy work's light slot byte. */
void light_em_slot_free(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif
