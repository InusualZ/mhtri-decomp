/* Leaf header (docs/plan.md 6.5 rule 2): `Pl_net_can_send` (0x80334A4C), defined by `src/hud/net_char_sync.cpp`.  Separate so a consumer
 * does not take the whole net message family (`hud/net_char_sync.h`, which includes `enemy/ENEMY_WORK.h`).
 */
#ifndef MHTRI_HUD_PL_NET_CAN_SEND_H
#define MHTRI_HUD_PL_NET_CAN_SEND_H

#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif

u32 Pl_net_can_send(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_PL_NET_CAN_SEND_H */
