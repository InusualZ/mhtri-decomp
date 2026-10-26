/* Pl/pl_weapon_drawn_ck.h - leaf header: `pl_weapon_drawn_ck` (0x80274570), owned by `Pl/pl_act.cpp`. */
#ifndef MHTRI_PL_PL_WEAPON_DRAWN_CK_H
#define MHTRI_PL_PL_WEAPON_DRAWN_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif
/* Nonzero while the player has the weapon drawn. */
s32 pl_weapon_drawn_ck(struct _PLW* plw);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_WEAPON_DRAWN_CK_H */
