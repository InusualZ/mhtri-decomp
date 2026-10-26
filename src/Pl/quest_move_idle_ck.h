/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `Pl/pl_motion.cpp` defines, for a consumer that does not include
 * the owner's header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_PL_QUEST_MOVE_IDLE_CK_H
#define MHTRI_PL_QUEST_MOVE_IDLE_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8028D574 - 1 when the quest move work is missing or its +0x114 word is clear. */
s32 quest_move_idle_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_QUEST_MOVE_IDLE_CK_H */
