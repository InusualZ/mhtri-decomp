/* `equip_weapon_recs_find`, defined by `Pl/pl_act.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_EQUIP_WEAPON_RECS_FIND_H
#define MHTRI_PL_EQUIP_WEAPON_RECS_FIND_H

#include "types.h"

struct PlSkillRec;
struct PlWeaponRec;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027ED6C - stores the blade row (kinds 7-10/14-15) or the bowgun part row (11-13) of kind `kind` and id `id`
 * in `blade`/`gun` (the other null); returns 0, 1, or -1 for no weapon kind. */
s32 equip_weapon_recs_find(u8 kind, u16 id, struct PlSkillRec** blade, struct PlWeaponRec** gun);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_EQUIP_WEAPON_RECS_FIND_H */
