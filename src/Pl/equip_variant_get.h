/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `equip_variant_get`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_PL_EQUIP_VARIANT_GET_H
#define MHTRI_PL_EQUIP_VARIANT_GET_H

#include "types.h"

struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027EE24 - the variant id (the first halfword of its blade or weapon record) of equipment piece `equip`, 0xFFFF
 * for a piece with none (GUESS name). */
s32 equip_variant_get(struct _EQUIP* equip);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_EQUIP_VARIANT_GET_H */
