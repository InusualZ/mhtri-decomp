/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/cockpit` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_HUD_AINPC_PAGE_STATE_INIT_H
#define MHTRI_HUD_AINPC_PAGE_STATE_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802DB26C - memsets ainpc_page_state (0x4C) and sets its +0x24 to 1 and +0x30 to 0xFF */
void ainpc_page_state_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_AINPC_PAGE_STATE_INIT_H */
