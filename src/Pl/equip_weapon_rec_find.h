/* `equip_weapon_rec_find`, defined by `Pl/pl_act.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_EQUIP_WEAPON_REC_FIND_H
#define MHTRI_PL_EQUIP_WEAPON_REC_FIND_H

#include "types.h"

struct PlWeaponRec;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027ECBC - the bowgun part row (`PlWeaponRec`) of kind `kind` and id `id`. */
struct PlWeaponRec* equip_weapon_rec_find(u8 kind, u16 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_EQUIP_WEAPON_REC_FIND_H */
