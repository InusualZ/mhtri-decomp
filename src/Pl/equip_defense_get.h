/* `equip_defense_get`, defined by `Pl/pl_act.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_EQUIP_DEFENSE_GET_H
#define MHTRI_PL_EQUIP_DEFENSE_GET_H

#include "types.h"

struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027E510 - the armour piece's defence at its upgrade level. */
s32 equip_defense_get(struct _EQUIP* equip);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_EQUIP_DEFENSE_GET_H */
