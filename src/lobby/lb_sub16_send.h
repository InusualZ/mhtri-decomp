/* Leaf header (docs/plan.md 6.5 rule 2): the `lobby/lb_companion_ui.cpp` symbol `quest/quest_entry.cpp` calls,
 * for a consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_SUB16_SEND_H
#define MHTRI_LOBBY_LB_SUB16_SEND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80339790 - sends the lobby's sub-0x16 command: `value` and the element index `flag`. */
void lb_sub16_send(s32 value, u8 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SUB16_SEND_H */
