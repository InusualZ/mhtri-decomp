/* Leaf header (docs/plan.md 6.5 rule 2): the `lobby/lb_companion_ui.cpp` symbol `enemy/em_pop.cpp` calls,
 * for a consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_ENTRY_START_DEFAULT_H
#define MHTRI_LOBBY_LB_ENTRY_START_DEFAULT_H

#include "types.h"

struct LbCompanionWork;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80339BE8 - starts the lobby entry with result `value` and the quest's time `arg` (the server-select
 * screen's form of `quest_result_enter`). */
void lb_entry_start_default(struct LbCompanionWork* companion, u8 value, s32 arg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_ENTRY_START_DEFAULT_H */
