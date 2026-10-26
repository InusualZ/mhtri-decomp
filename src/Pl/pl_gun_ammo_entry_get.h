/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_gun_ammo_entry_get`, which `Pl/fn_802840DC.cpp` calls. */
#ifndef MHTRI_PL_PL_GUN_AMMO_ENTRY_GET_H
#define MHTRI_PL_PL_GUN_AMMO_ENTRY_GET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80279414 - the actor's gun-ammo record for an ammo kind, or 0 (registry name). */
u8* pl_gun_ammo_entry_get(struct _PLW* self, u8 ammo);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_GUN_AMMO_ENTRY_GET_H */
