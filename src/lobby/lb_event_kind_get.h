/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_802FA9A0.cpp`'s `lb_event_kind_get`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_EVENT_KIND_GET_H
#define MHTRI_LOBBY_LB_EVENT_KIND_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802FB5F0 - returns lobby_w byte +0x3 */
u8 lb_event_kind_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_EVENT_KIND_GET_H */
