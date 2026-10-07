/*
 * lobby/lb_npc_motion_play.h - leaf header (docs/plan.md 6.5 rule 2) for `lobby/lb_npc.cpp`'s `lb_npc_motion_play`
 *   (0x801FE1CC), in the signature its callers' code shows (the two floats pass through to the model's motion call
 *   and the result is compared with 1); the owner still defines it with a narrower view.
 */
#ifndef MHTRI_LOBBY_LB_NPC_MOTION_PLAY_H
#define MHTRI_LOBBY_LB_NPC_MOTION_PLAY_H

#include "types.h"

struct _LB_NPC;

#ifdef __cplusplus
extern "C" {
#endif

/* Plays motion `motion` on the NPC's model (frame `frame`, speed `speed`); 1 when the motion has run out.  GUESS. */
u32 lb_npc_motion_play(struct _LB_NPC* self, u16 motion, f32 frame, f32 speed);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_NPC_MOTION_PLAY_H */
