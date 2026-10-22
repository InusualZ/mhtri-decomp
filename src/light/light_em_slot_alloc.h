/* Leaf header (docs/plan.md 6.5 rule 2): `light_em_slot_alloc`, which `light/light` owns (the name is a registry proposal of lane
 * enemy-control; the map row is renamed when it lands).  C linkage. */
#ifndef MHTRI_LIGHT_LIGHT_EM_SLOT_ALLOC_H
#define MHTRI_LIGHT_LIGHT_EM_SLOT_ALLOC_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802C05D0 - the light slot of an enemy work (7 for kind 40). */
s32 light_em_slot_alloc(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif
