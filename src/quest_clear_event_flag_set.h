/* Leaf header (docs/plan.md 6.5 rule 2): `fn_8004CAD8.cpp`'s `quest_clear_event_flag_set`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_QUEST_CLEAR_EVENT_FLAG_SET_H
#define MHTRI_QUEST_CLEAR_EVENT_FLAG_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004D698 (0x74): raises the cleared quest's event flag. */
void quest_clear_event_flag_set(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_QUEST_CLEAR_EVENT_FLAG_SET_H */
