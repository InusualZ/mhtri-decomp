/* Leaf header (docs/plan.md 6.5 rule 2): `Pl_net_send` (0x80335CE8), defined by `src/hud/net_char_sync.cpp`.  Separate so a consumer
 * does not take the whole net message family (`hud/net_char_sync.h`, which includes `enemy/ENEMY_WORK.h`).
 */
#ifndef MHTRI_HUD_PL_NET_SEND_H
#define MHTRI_HUD_PL_NET_SEND_H

#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif

void Pl_net_send(struct _PLW* plw, u8 kind, u16 param);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_PL_NET_SEND_H */
