/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_message` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_MENU_FRAME_ENTRIES_BUILD_ROW_H
#define MHTRI_MENU_MENU_FRAME_ENTRIES_BUILD_ROW_H

#include "types.h"

struct _PLW;
struct IdValue;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802A6DB4 - draws the item grid's sort preview of both pages */
void menu_frame_entries_build_row(_PLW* plw, IdValue* page_a, IdValue* page_b, s8 rows, u16 flags, u8 player);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_FRAME_ENTRIES_BUILD_ROW_H */
