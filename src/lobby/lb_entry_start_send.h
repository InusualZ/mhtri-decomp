/* Leaf header (docs/plan.md 6.5 rule 2): the `lobby/lb_companion_ui.cpp` symbol `quest/quest_entry.cpp` calls,
 * for a consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_ENTRY_START_SEND_H
#define MHTRI_LOBBY_LB_ENTRY_START_SEND_H

#include "types.h"

struct LbCompanionWork;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80339AC8 - starts the lobby entry with result `value`, the quest's time `arg` and `flag` (1: the
 * reward is gone). */
void lb_entry_start_send(struct LbCompanionWork* companion, s8 value, s32 arg, u8 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_ENTRY_START_SEND_H */
