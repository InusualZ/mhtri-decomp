/* Leaf header (docs/plan.md 6.5 rule 2): `fn_8004CAD8.cpp`'s `userdata_item_new_marks_clear`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_USERDATA_ITEM_NEW_MARKS_CLEAR_H
#define MHTRI_USERDATA_ITEM_NEW_MARKS_CLEAR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004D4D4 - clears bit 2 of the 187 flag bytes at +0x4B50 (lobby_world_block) */
void userdata_item_new_marks_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_ITEM_NEW_MARKS_CLEAR_H */
