/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_pane_ui.cpp`'s `lb_progress_rewards_check`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  The map's name. */
#ifndef MHTRI_LOBBY_LB_PROGRESS_REWARDS_CHECK_H
#define MHTRI_LOBBY_LB_PROGRESS_REWARDS_CHECK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801FBB64 - applies the progress unlocks (userdata_progress_flag_ck, lb_progress_unlock, gallery_open) */
void lb_progress_rewards_check(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_PROGRESS_REWARDS_CHECK_H */
