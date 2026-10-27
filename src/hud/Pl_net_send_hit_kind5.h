/* Leaf header (docs/plan.md 6.5 rule 2): `Pl_net_send_hit_kind5` (0x8033536C), defined by `src/hud/pl_frame_sync.cpp`.
 * Separate so a consumer does not take the whole net message family (`hud/net_char_sync.h`).
 */
#ifndef MHTRI_HUD_PL_NET_SEND_HIT_KIND5_H
#define MHTRI_HUD_PL_NET_SEND_HIT_KIND5_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

void Pl_net_send_hit_kind5(u8 attack_kind, const VEC3* pos, u16 param);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_PL_NET_SEND_HIT_KIND5_H */
