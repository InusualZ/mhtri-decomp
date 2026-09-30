/* Leaf header (docs/plan.md 6.5 rule 2): the `lobby/lb_companion_ui.cpp` symbol `quest/quest_entry.cpp` calls,
 * for a consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_ENTRY_NOTIFY_SEND_H
#define MHTRI_LOBBY_LB_ENTRY_NOTIFY_SEND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80338990 - sends the entry-notify message for entry `entry` on channel `index`. */
void lb_entry_notify_send(s32 index, u8 entry);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_ENTRY_NOTIFY_SEND_H */
