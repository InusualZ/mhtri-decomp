/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_companion_ui.cpp` symbols `quest/quest_entry.cpp and enemy/fn_802F5138.cpp` calls, for a consumer that cannot
 * include the owner's full header.  C linkage (the map rows are plain names).  GUESS names, derived from the
 * call sites and bodies. */
#ifndef MHTRI_LOBBY_LB_EVENT_REQUEST_H
#define MHTRI_LOBBY_LB_EVENT_REQUEST_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8033A920 - requests lobby event `id`: 1 when it was armed, 0 when the lobby refuses (server-select screen,
 * outside game mode 1, or busy). */
u32 lb_event_request(u32 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_EVENT_REQUEST_H */
