/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `menu/menu_item_page.cpp` defines, for a consumer that cannot include
 * the owner's header (it defines its own `_mh_ivec2_`).  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_ITEM_PAGE_DRAW_OPTION_FRAME_H
#define MHTRI_MENU_ITEM_PAGE_DRAW_OPTION_FRAME_H

#include "types.h"

struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8034BEF4 - draws the option window frame with rows `a`..`b` lit and returns its text anchor in `pos`. */
void item_page_draw_option_frame(s8 a, s8 b, u16 c, u16 d, u8 flags, struct _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_ITEM_PAGE_DRAW_OPTION_FRAME_H */
