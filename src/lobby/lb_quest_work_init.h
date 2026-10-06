/* Leaf header (docs/plan.md 6.5 rule 2): the `lobby/lb_companion_ui.cpp` symbol `quest/quest_entry.cpp` calls,
 * for a consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_QUEST_WORK_INIT_H
#define MHTRI_LOBBY_LB_QUEST_WORK_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8033A13C - sets the lobby's quest work up for kind `kind`.  `u16` is the callers' view (the quest start
 * passes the row's byte zero-extended to 16 bits); the owner spells `u8` and narrows it before storing. */
void lb_quest_work_init(u16 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_QUEST_WORK_INIT_H */
