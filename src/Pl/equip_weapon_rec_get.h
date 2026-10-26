/* `equip_weapon_rec_get`, defined by `Pl/pl_act.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_EQUIP_WEAPON_REC_GET_H
#define MHTRI_PL_EQUIP_WEAPON_REC_GET_H

#include "types.h"

struct _EQUIP;
struct PlWeaponRec;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027ED18 - the bowgun part row (`PlWeaponRec`) of the equipment record `equip`. */
struct PlWeaponRec* equip_weapon_rec_get(struct _EQUIP* equip);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_EQUIP_WEAPON_REC_GET_H */
