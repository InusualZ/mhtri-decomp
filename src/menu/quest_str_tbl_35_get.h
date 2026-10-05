/*
 * Leaf header (docs/plan.md 6.5 rule 2): `quest_str_tbl_35_get` (0x803B33B0), defined by
 * `src/quest/quest_entry.cpp`.  `quest/quest_entry.h` includes this one, so there is one declaration; it is
 * separate so `lobby/lb_companion_ui.cpp` can call the string getter without taking `unsplit/menu.h`,
 * whose `Screen_w` view clashes with the lobby band's.
 */
#ifndef MHTRI_MENU_QUEST_STR_TBL_35_GET_H
#define MHTRI_MENU_QUEST_STR_TBL_35_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The quest message string table 35: the string at `index`. */
char* quest_str_tbl_35_get(u32 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_QUEST_STR_TBL_35_GET_H */
