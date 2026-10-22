/* Leaf header (docs/plan.md 6.5 rule 2): the `lobby/lb_companion_ui` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_ENTRY_SELECTED_SEND_H
#define MHTRI_LOBBY_LB_ENTRY_SELECTED_SEND_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8033893C - sends the player's quest-ready entry to the lobby */
void lb_entry_selected_send(_PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_ENTRY_SELECTED_SEND_H */
