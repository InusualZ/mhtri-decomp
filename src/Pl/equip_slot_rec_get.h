/* `equip_slot_rec_get`, defined by `Pl/pl_act.cpp` (leaf header, rule 2).  C linkage (unmangled map name). */
#ifndef MHTRI_PL_EQUIP_SLOT_REC_GET_H
#define MHTRI_PL_EQUIP_SLOT_REC_GET_H

#include "types.h"

struct _EQUIP;
struct PlSlotRec;

#ifdef __cplusplus
extern "C" {
#endif

/* The armour row (`Pl/fn_8027D684.h`'s `PlSlotRec`) of the equipment record, or null. */
struct PlSlotRec* equip_slot_rec_get(struct _EQUIP* equip);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_EQUIP_SLOT_REC_GET_H */
