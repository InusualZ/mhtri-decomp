/*
 * lobby/lb_unlock_cond_ck.h - leaf header (docs/plan.md 6.5 rule 2) for `lobby/fn_802FA9A0.cpp`'s unlock queries; the
 *   owner's full header keeps its own `lobby_w` view, which cannot sit beside `lobby/lobby_w.h`.
 */
#ifndef MHTRI_LOBBY_LB_UNLOCK_COND_CK_H
#define MHTRI_LOBBY_LB_UNLOCK_COND_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802FB4BC - whether unlock condition `id` holds: 0xFFFF never, 0 always, the others by their progress-flag lists.
 * GUESS name. */
u32 lb_unlock_cond_ck(u32 id);
/* 0x802FF234 - whether the whale event's countdown byte has gone negative.  GUESS name. */
u32 kujira_event_over_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_UNLOCK_COND_CK_H */
