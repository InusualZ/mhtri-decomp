/* Leaf header (docs/plan.md 6.5 rule 2): `menu/fn_8031A6C0.cpp`'s `eft043_set`, which `Pl/fn_802840DC.cpp` calls. */
#ifndef MHTRI_MENU_EFT043_SET_H
#define MHTRI_MENU_EFT043_SET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8031D294 - spawns effect 43 on the actor (registry name). */
void eft043_set(struct _PLW* self, u8 type);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_EFT043_SET_H */
