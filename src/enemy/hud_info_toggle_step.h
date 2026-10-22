/* `hud_info_toggle_step`, defined by `enemy/em_prog_support.cpp` (leaf header, rule 2). */
#ifndef MHTRI_ENEMY_HUD_INFO_TOGGLE_STEP_H
#define MHTRI_ENEMY_HUD_INFO_TOGGLE_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Steps the info popup's timer and toggles its view mode (4/12) on the 0x100 button in the field. */
void hud_info_toggle_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_HUD_INFO_TOGGLE_STEP_H */
