/* `equip_deco_count`, defined by `Pl/pl_act.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_EQUIP_DECO_COUNT_H
#define MHTRI_PL_EQUIP_DECO_COUNT_H

#include "types.h"

struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* The decorations set in the equipment record (`equip_deco_count`) and its free decoration slots (`equip_deco_slots_free`). */
s32 equip_deco_count(struct _EQUIP* equip);
s32 equip_deco_slots_free(struct _EQUIP* equip);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_EQUIP_DECO_COUNT_H */
