/* Leaf header (docs/plan.md 6.5 rule 2): the combine tables `menu/menu_placeinfo.cpp`'s ranges hold (.data
 * 0x80603EA0-0x80604638, .sdata 0x80793A58-0x80793A67), for a consumer that does not include the owner's source.
 * The row types are `lobby/lb_companion_ui.h`'s, which includes this header after defining them. */
#ifndef MHTRI_MENU_LB_COMBO_RECIPE_TBL_H
#define MHTRI_MENU_LB_COMBO_RECIPE_TBL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80603EA0 - the recipe rows, grouped by their first ingredient. */
extern struct LbComboRecipe lb_combo_recipe_tbl[];
/* 0x806042B8 - each first ingredient's run of rows, ended by item 0xFFFF. */
extern struct LbComboIndex lb_combo_index_tbl[];
/* 0x806043E8 - the ingredient pair of each recipe the combine list shows. */
extern struct LbComboPair lb_combo_pair_tbl[];
/* 0x806045F8 - the yield odds lists: the base yield, then the roll each extra unit costs. */
extern u8* lb_combo_yield_odds[];
/* 0x80604628 - the five books whose possession raises the success rate. */
extern u16 lb_combo_bonus_items[];
/* 0x80793A58 - the base success rate of each rate class. */
extern s8 lb_combo_base_rates[];
/* 0x80793A60 - the rate bonus for the number of books held. */
extern u8 lb_combo_bonus_rates[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_LB_COMBO_RECIPE_TBL_H */
