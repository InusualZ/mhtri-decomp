/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `lobby/lb_companion_ui.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_COMBO_RECIPE_KNOWN_CK_H
#define MHTRI_LOBBY_LB_COMBO_RECIPE_KNOWN_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8033B2C8 - 1 when the player knows recipe `recipe`. */
u32 lb_combo_recipe_known_ck(struct LbComboRecipe* recipe, s32 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_COMBO_RECIPE_KNOWN_CK_H */
