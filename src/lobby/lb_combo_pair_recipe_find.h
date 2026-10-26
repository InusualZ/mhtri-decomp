/* lobby/lb_combo_pair_recipe_find.h - leaf header: `lb_combo_pair_recipe_find` (0x8033ADAC), owned by `lobby/lb_companion_ui.cpp`. */
#ifndef MHTRI_LOBBY_LB_COMBO_PAIR_RECIPE_FIND_H
#define MHTRI_LOBBY_LB_COMBO_PAIR_RECIPE_FIND_H

#include "types.h"

struct LbComboRecipe;

#ifdef __cplusplus
extern "C" {
#endif
/* Looks the page table row `index` up as a combination recipe; null when it has none. */
struct LbComboRecipe* lb_combo_pair_recipe_find(u16 index, s32 value);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_COMBO_PAIR_RECIPE_FIND_H */
