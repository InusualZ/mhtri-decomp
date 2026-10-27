/* Leaf header (docs/plan.md 6.5 rule 2): `pl_kind_item_equip_find` (0x8027DF38), defined by `src/Pl/pl_act.cpp`. */
#ifndef MHTRI_PL_PL_KIND_ITEM_EQUIP_FIND_H
#define MHTRI_PL_PL_KIND_ITEM_EQUIP_FIND_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

u16 pl_kind_item_equip_find(struct _PLW* self, s32 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_KIND_ITEM_EQUIP_FIND_H */
