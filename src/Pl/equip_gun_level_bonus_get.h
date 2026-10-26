/* `equip_gun_level_bonus_get`, defined by `Pl/pl_act.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_EQUIP_GUN_LEVEL_BONUS_GET_H
#define MHTRI_PL_EQUIP_GUN_LEVEL_BONUS_GET_H

#include "types.h"

struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027E5E4 - the bowgun piece's summed upgrade bonus up to its level. */
s16 equip_gun_level_bonus_get(struct _EQUIP* equip);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_EQUIP_GUN_LEVEL_BONUS_GET_H */
