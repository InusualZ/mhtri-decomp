/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/fn_80229ECC.cpp`'s `pl_se_frame_callback`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_SE_FRAME_CALLBACK_H
#define MHTRI_PL_PL_SE_FRAME_CALLBACK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80229ECC - the player's SE frame callback (se_req_frame_set by motion) */
void pl_se_frame_callback(struct _PLW* plw, s32 event);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_SE_FRAME_CALLBACK_H */
