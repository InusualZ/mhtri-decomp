/* Leaf header (docs/plan.md 6.5 rule 2): `hud/cockpit.cpp`'s `ainpc_page_idle`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_HUD_AINPC_PAGE_IDLE_H
#define MHTRI_HUD_AINPC_PAGE_IDLE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802DB2EC (0x4): the AI NPC page's idle hook. */
void ainpc_page_idle(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_AINPC_PAGE_IDLE_H */
