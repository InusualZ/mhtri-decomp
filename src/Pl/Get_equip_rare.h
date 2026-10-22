/* `Get_equip_rare` and `Get_pl_type`, defined by `Pl/pl_act.cpp` (leaf header, rule 2).  C++ linkage: the map spells
 * them `Get_equip_rare__FP6_EQUIP` and `Get_pl_type__FP6_EQUIPP6_EQUIP`. */
#ifndef MHTRI_PL_GET_EQUIP_RARE_H
#define MHTRI_PL_GET_EQUIP_RARE_H

#include "types.h"

struct _EQUIP;

/* The rarity of the equipment record. */
u8 Get_equip_rare(struct _EQUIP* equip);

/* The player weapon type a bowgun and its option record make. */
u8 Get_pl_type(struct _EQUIP* equipA, struct _EQUIP* equipB);

#endif /* MHTRI_PL_GET_EQUIP_RARE_H */
