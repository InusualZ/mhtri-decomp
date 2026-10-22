/* Leaf header (docs/plan.md 6.5 rule 2): `fn_8004CAD8.cpp`'s `userdata_arena_row_get`, the one declaration `menu/menu_sysmsg.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of menu/menu_sysmsg.cpp). */
#ifndef MHTRI_USERDATA_ARENA_ROW_GET_H
#define MHTRI_USERDATA_ARENA_ROW_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004ED14 (0x38): returns the player's chat phrase `index` (37-byte slots of the user data at +0x4114) */
char* userdata_arena_row_get(s32 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_ARENA_ROW_GET_H */
