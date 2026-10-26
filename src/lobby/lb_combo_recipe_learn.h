/* lobby/lb_combo_recipe_learn.h - leaf header: `lb_combo_recipe_learn` (0x8033B298), owned by `lobby/lb_companion_ui.cpp`. */
#ifndef MHTRI_LOBBY_LB_COMBO_RECIPE_LEARN_H
#define MHTRI_LOBBY_LB_COMBO_RECIPE_LEARN_H

#include "types.h"

struct LbComboRecipe;

#ifdef __cplusplus
extern "C" {
#endif
/* Marks the recipe `recipe` seen in the user data's entry bits. */
void lb_combo_recipe_learn(struct LbComboRecipe* recipe);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_COMBO_RECIPE_LEARN_H */
