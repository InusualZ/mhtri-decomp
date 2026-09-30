/* Leaf header (docs/plan.md 6.5 rule 2): the effect-slot receivers `eft_net_recv_*` (0x80337E78..0x80338600), defined by `src/hud/net_char_sync.cpp`.  Separate so a consumer
 * does not take the whole net message family (`hud/net_char_sync.h`, which includes `enemy/ENEMY_WORK.h`).
 */
#ifndef MHTRI_HUD_EFT_NET_RECV_STATE_H
#define MHTRI_HUD_EFT_NET_RECV_STATE_H

#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif

void eft_net_recv_state(struct EftSlot* slot, struct NetEftStateMsg* msg);
void eft_net_recv_step(struct EftSlot* slot, struct NetEftStepMsg* msg);
void eft_net_recv_live(struct EftSlot* slot, struct NetEftLiveMsg* msg);
void eft_net_recv_pos(struct EftSlot* slot, struct NetEftPosMsg* msg);
void eft_net_recv_work(struct EftSlot* slot, struct NetEftWorkMsg* msg, u8 own);
void eft_net_recv_mark(struct EftSlot* slot, struct NetEftMarkMsg* msg);
void eft_net_recv_bind(struct EftSlot* slot, struct NetEftBindMsg* msg);
void eft_net_recv_release(struct EftSlot* slot, struct NetEftReleaseMsg* msg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_EFT_NET_RECV_STATE_H */
