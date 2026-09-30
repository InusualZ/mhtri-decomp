/* Leaf header (docs/plan.md 6.5 rule 2): `em_net_send` (0x8033737C), defined by `src/hud/net_char_sync.cpp`.  Separate so a consumer
 * does not take the whole net message family (`hud/net_char_sync.h`, which includes `enemy/ENEMY_WORK.h`).
 */
#ifndef MHTRI_HUD_EM_NET_SEND_H
#define MHTRI_HUD_EM_NET_SEND_H

#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif

void em_net_send(struct _ENEMY_WORK* work, u8 kind, u16 param);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_EM_NET_SEND_H */
