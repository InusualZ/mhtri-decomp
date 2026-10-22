/* `lb_eqshop_set_parts`, `lb_eqshop_weapon_tables` and `lb_eqshop_armor_tables`, defined by `ef/eft052.cpp`'s `.data`
 * (leaf header, rule 2): the equipment shop's goods tables, rows of three halfwords (an id and two payload ids). */
#ifndef MHTRI_EF_LB_EQSHOP_SET_PARTS_H
#define MHTRI_EF_LB_EQSHOP_SET_PARTS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* .data 0x805ED210 - the armour sets: set id -> the 0xC and 0xD kind pieces (four rows). */
extern u16 lb_eqshop_set_parts[12];
/* .data 0x805ED304 - the weapon goods tables, one per shop category. */
extern u16* lb_eqshop_weapon_tables[5];
/* .data 0x805ED318 - the armour goods tables, one per shop category (NULL-terminated). */
extern u16* lb_eqshop_armor_tables[6];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_LB_EQSHOP_SET_PARTS_H */
