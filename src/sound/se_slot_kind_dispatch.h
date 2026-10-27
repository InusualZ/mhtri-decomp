/* Leaf header (docs/plan.md 6.5 rule 2): `sound/fn_800D7F54.cpp`'s per-kind slot handler dispatch (registry name; GUESS). */
#ifndef MHTRI_SOUND_SE_SLOT_KIND_DISPATCH_H
#define MHTRI_SOUND_SE_SLOT_KIND_DISPATCH_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct SeSlot;
/* 0x800D8404 - runs the handler of the slot's `kind`. */
void se_slot_kind_dispatch(struct SeSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SE_SLOT_KIND_DISPATCH_H */
