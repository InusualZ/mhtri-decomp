/* Leaf header (docs/plan.md 6.5 rule 2): `hud/cockpit.cpp`'s `ainpc_page_reset`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_HUD_AINPC_PAGE_RESET_H
#define MHTRI_HUD_AINPC_PAGE_RESET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802DB2B8 (0x34): resets the AI NPC page. */
void ainpc_page_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_AINPC_PAGE_RESET_H */
