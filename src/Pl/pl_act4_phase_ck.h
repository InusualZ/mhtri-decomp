/* Pl/pl_act4_phase_ck.h - leaf header: `pl_act4_phase_ck` (0x80287244), owned by `Pl/fn_802840DC.cpp`. */
#ifndef MHTRI_PL_PL_ACT4_PHASE_CK_H
#define MHTRI_PL_PL_ACT4_PHASE_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif
/* 1 when the player is in act group 4 at one of the acts of `phase` (0 or 1). */
u32 pl_act4_phase_ck(struct _PLW* plw, s32 phase);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT4_PHASE_CK_H */
