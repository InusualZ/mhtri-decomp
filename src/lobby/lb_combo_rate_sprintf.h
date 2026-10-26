/* lobby/lb_combo_rate_sprintf.h - leaf header: `lb_combo_rate_sprintf` (0x8033B67C), owned by `lobby/lb_companion_ui.cpp`. */
#ifndef MHTRI_LOBBY_LB_COMBO_RATE_SPRINTF_H
#define MHTRI_LOBBY_LB_COMBO_RATE_SPRINTF_H

#include "types.h"

struct LbComboRecipe;

#ifdef __cplusplus
extern "C" {
#endif
/* Prints the yield text of `recipe` into `out`. */
void lb_combo_rate_sprintf(char* out, struct LbComboRecipe* recipe);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_COMBO_RATE_SPRINTF_H */
