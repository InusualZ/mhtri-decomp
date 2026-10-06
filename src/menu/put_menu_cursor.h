/* Leaf header (docs/plan.md 6.5 rule 2): `menu/menu_item.cpp`'s menu cursor, for a consumer that cannot include
 * `menu/menu_item.h`.  C++ linkage (the map row is `put_menu_cursor__FPUsUsPC10_mh_ivec2_`). */
#ifndef MHTRI_MENU_PUT_MENU_CURSOR_H
#define MHTRI_MENU_PUT_MENU_CURSOR_H

#include "types.h"

struct _mh_ivec2_;

/* 0x802A2564 - puts the cursor sprite rows `rows` (frame `index`) at `pos`. */
void put_menu_cursor(u16* rows, u16 index, const struct _mh_ivec2_* pos);

#endif /* MHTRI_MENU_PUT_MENU_CURSOR_H */
