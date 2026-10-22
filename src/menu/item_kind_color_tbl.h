/* Leaf header (docs/plan.md 6.5 rule 2): symbols `menu/menu_item.cpp` defines, for `hud/cockpit_quest.cpp`. */
#ifndef MHTRI_MENU_ITEM_KIND_COLOR_TBL_H
#define MHTRI_MENU_ITEM_KIND_COLOR_TBL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x805CDE78: the colour of each item kind (`ItemDataRecord::kind`). */
extern u32 item_kind_color_tbl[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_ITEM_KIND_COLOR_TBL_H */
