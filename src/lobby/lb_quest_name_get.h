/* Leaf header (docs/plan.md 6.5 rule 2) for `lb_quest_name_get` (0x8033AAA4) and `lb_quest_msg_get` (0x8033AAB0),
 * the quest text getters `lobby/lb_companion_ui.cpp` owns.  C linkage (both map rows are unmangled); a leaf because
 * `lobby/lb_companion_ui.h` redefines `_mh_ivec2_` and cannot meet `hud/layout_types.h`. */
#ifndef MHTRI_LOBBY_LB_QUEST_NAME_GET_H
#define MHTRI_LOBBY_LB_QUEST_NAME_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8033AAA4 - the current quest's name text. */
s32 lb_quest_name_get(void);
/* 0x8033AAB0 - the current quest's message text `index`. */
s32 lb_quest_msg_get(u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_QUEST_NAME_GET_H */
