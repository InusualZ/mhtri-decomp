/* lobby/lb_combo_rate_get.h - leaf header: `lb_combo_rate_get` (0x8033AED0), owned by `lobby/lb_companion_ui.cpp`. */
#ifndef MHTRI_LOBBY_LB_COMBO_RATE_GET_H
#define MHTRI_LOBBY_LB_COMBO_RATE_GET_H

#include "types.h"

struct LbComboRecipe;

#ifdef __cplusplus
extern "C" {
#endif
/* The success rate of `recipe` in `mode` for the cursor `kind`. */
s8 lb_combo_rate_get(struct LbComboRecipe* recipe, s32 mode, u8 kind);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_COMBO_RATE_GET_H */
