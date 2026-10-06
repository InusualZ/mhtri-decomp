/* Leaf header (docs/plan.md 6.5 rule 2): the `lobby/lb_companion_ui.cpp` symbol `quest/quest_entry.cpp` calls,
 * for a consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_SUB14_SEND_H
#define MHTRI_LOBBY_LB_SUB14_SEND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80339610 - sends the lobby's sub-0x14 command with `value` (the arena item the area hand-off reached). */
void lb_sub14_send(s8 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SUB14_SEND_H */
