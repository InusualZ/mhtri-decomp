/* Leaf header (docs/plan.md 6.5 rule 2): `hud/cockpit.cpp`'s `ainpc_page_hold_set`, which
 * `lobby/lb_companion_ui.cpp` calls (the owner's `hud/cockpit.h` cannot be included beside it). */
#ifndef MHTRI_HUD_AINPC_PAGE_HOLD_SET_H
#define MHTRI_HUD_AINPC_PAGE_HOLD_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Holds the AI NPC page for this frame. */
void ainpc_page_hold_set(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_AINPC_PAGE_HOLD_SET_H */
