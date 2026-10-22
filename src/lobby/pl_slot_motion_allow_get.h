/* `pl_slot_motion_allow_get`, defined by `lobby/lb_equip_page.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_PL_SLOT_MOTION_ALLOW_GET_H
#define MHTRI_LOBBY_PL_SLOT_MOTION_ALLOW_GET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* Which of the player's two weapon motions slot `slot` allows (0 or 1). */
u8 pl_slot_motion_allow_get(struct _PLW* self, u32 slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_PL_SLOT_MOTION_ALLOW_GET_H */
