/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/cockpit.cpp` page-mode switch a lobby screen calls (the page marks and
 * the money panel are `hud/cockpit.h`'s). */
#ifndef MHTRI_HUD_AINPC_PAGE_MODE2_SET_H
#define MHTRI_HUD_AINPC_PAGE_MODE2_SET_H

#include "types.h"
#include "hud/layout_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802DFC7C - switches the NPC page to mode 2 (GUESS name). */
void ainpc_page_mode2_set(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_AINPC_PAGE_MODE2_SET_H */
