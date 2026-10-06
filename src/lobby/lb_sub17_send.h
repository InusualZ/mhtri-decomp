/* Leaf header (docs/plan.md 6.5 rule 2): the `lobby/lb_companion_ui.cpp` symbol `enemy/em_pop.cpp` calls,
 * for a consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_SUB17_SEND_H
#define MHTRI_LOBBY_LB_SUB17_SEND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80339934 - sends the lobby's sub-0x17 command: the supply item id and three bytes (the supply drop sends
 * the drops delivered, the kind left and the kind delivered). */
void lb_sub17_send(u16 value, s8 first, s8 second, s8 third);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SUB17_SEND_H */
