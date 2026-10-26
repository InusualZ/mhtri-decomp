/* `equip_slot_rec_find`, defined by `Pl/pl_act.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_EQUIP_SLOT_REC_FIND_H
#define MHTRI_PL_EQUIP_SLOT_REC_FIND_H

#include "types.h"

struct PlSlotRec;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027E2A8 - the armour row (`PlSlotRec`) of equipment kind `kind` and id `id`, or null for a weapon kind. */
struct PlSlotRec* equip_slot_rec_find(u8 kind, u16 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_EQUIP_SLOT_REC_FIND_H */
