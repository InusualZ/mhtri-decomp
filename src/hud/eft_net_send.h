/* Leaf header (docs/plan.md 6.5 rule 2): `eft_net_send` (0x803386C4), defined by `src/hud/net_char_sync.cpp`.  Separate so a consumer
 * does not take the whole net message family (`hud/net_char_sync.h`, which includes `enemy/ENEMY_WORK.h`).
 */
#ifndef MHTRI_HUD_EFT_NET_SEND_H
#define MHTRI_HUD_EFT_NET_SEND_H

#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif

void eft_net_send(struct EftSlot* slot, u32 mode, u32 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_EFT_NET_SEND_H */
