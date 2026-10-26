/* `equip_kind_item_count` (0x8027E918), defined by `Pl/pl_act.cpp` (leaf header, rule 2): the number of items an
 * equipment kind's table holds (armour kinds 1-6, weapon kinds 7-15; 0 for any other kind). */
#ifndef MHTRI_PL_EQUIP_KIND_ITEM_COUNT_H
#define MHTRI_PL_EQUIP_KIND_ITEM_COUNT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

s32 equip_kind_item_count(u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_EQUIP_KIND_ITEM_COUNT_H */
