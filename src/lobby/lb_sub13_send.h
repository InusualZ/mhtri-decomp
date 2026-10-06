/* Leaf header (docs/plan.md 6.5 rule 2): the `lobby/lb_companion_ui.cpp` symbol `quest/quest_entry.cpp` calls,
 * for a consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_SUB13_SEND_H
#define MHTRI_LOBBY_LB_SUB13_SEND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80339560 - sends the lobby's sub-0x13 command: two halfwords and a value byte (the item use sends the
 * item id and the count left). */
void lb_sub13_send(u16 first, s16 second, u8 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SUB13_SEND_H */
