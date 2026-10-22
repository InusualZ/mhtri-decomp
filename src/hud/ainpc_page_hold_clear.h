/* `ainpc_page_hold_clear`, defined by `hud/cockpit.cpp` (leaf header, rule 2). */
#ifndef MHTRI_HUD_AINPC_PAGE_HOLD_CLEAR_H
#define MHTRI_HUD_AINPC_PAGE_HOLD_CLEAR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Releases the AI-NPC page hold `ainpc_page_hold_set` takes. */
void ainpc_page_hold_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_AINPC_PAGE_HOLD_CLEAR_H */
