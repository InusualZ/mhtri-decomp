/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_weapon_alt_form_ck`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_PL_PL_WEAPON_ALT_FORM_CK_H
#define MHTRI_PL_PL_WEAPON_ALT_FORM_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802740EC - whether the actor's weapon (`_PLW::equipB`) is in its alternate form (GUESS name: the rig hides the weapon model's node 2 while it is not). */
u8 pl_weapon_alt_form_ck(struct _PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_WEAPON_ALT_FORM_CK_H */
