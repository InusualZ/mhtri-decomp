/* menu/menu_text_center_x_by_gaps.h - leaf header for `menu_text_center_x_by_gaps`, which `src/menu/menu_message.cpp`
 * defines with C linkage (docs/plan.md 6.5 rule 2).  A leaf because the owner's header does not declare it. */
#ifndef MHTRI_MENU_MENU_TEXT_CENTER_X_BY_GAPS_H
#define MHTRI_MENU_MENU_TEXT_CENTER_X_BY_GAPS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802A9630 - the x that centres `text` on `center`, counting the `size`-pixel gaps between its characters. */
s16 menu_text_center_x_by_gaps(char* text, s32 center, s16 size);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_TEXT_CENTER_X_BY_GAPS_H */
