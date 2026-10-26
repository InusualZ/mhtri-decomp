/* `equip_armor_ex_rec_find`, defined by `Pl/pl_act.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_EQUIP_ARMOR_EX_REC_FIND_H
#define MHTRI_PL_EQUIP_ARMOR_EX_REC_FIND_H

#include "types.h"

struct PlArmorExRec;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027E354 - the armour's 0x1C-byte second row (its sort byte and price) for kind `kind` and id `id`, or null. */
struct PlArmorExRec* equip_armor_ex_rec_find(u8 kind, u16 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_EQUIP_ARMOR_EX_REC_FIND_H */
