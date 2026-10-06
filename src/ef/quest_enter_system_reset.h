/* Leaf header (docs/plan.md 6.5 rule 2): `ef/system_core.cpp` symbols `quest/quest_entry.cpp` calls, for a consumer that cannot
 * include the owner's full header.  C linkage (the map rows are plain names).  GUESS names, derived from the
 * call sites and bodies. */
#ifndef MHTRI_EF_QUEST_ENTER_SYSTEM_RESET_H
#define MHTRI_EF_QUEST_ENTER_SYSTEM_RESET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D0FCC - the system reset the quest entry runs between the result text and the quest-work reset. */
void quest_enter_system_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_QUEST_ENTER_SYSTEM_RESET_H */
