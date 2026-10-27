/* Leaf header (docs/plan.md 6.5 rule 2): `hud/cockpit.cpp`'s `ainpc_page_update`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_HUD_AINPC_PAGE_UPDATE_H
#define MHTRI_HUD_AINPC_PAGE_UPDATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802DB2F0 (0x618): updates the AI NPC page. */
void ainpc_page_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_AINPC_PAGE_UPDATE_H */
