/* Leaf header (docs/plan.md 6.5 rule 2): `chara_icon_pos_get`, defined by `hud/cockpit.cpp`. */
#ifndef MHTRI_HUD_CHARA_ICON_POS_GET_H
#define MHTRI_HUD_CHARA_ICON_POS_GET_H

#include "types.h"
#include "pl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E03D4: the world position of the player's icon of `kind` (over the head, the item tag, ...). */
void chara_icon_pos_get(_PLW* plw, u8 kind, VEC3* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_CHARA_ICON_POS_GET_H */
