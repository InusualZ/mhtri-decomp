/* menu/put_frame_side_arrows.h - leaf header (docs/plan.md 6.5 rule 2): `menu/menu_message.cpp`'s 0x802A8650, for
 *   `menu/menu_placeinfo.cpp`.  C linkage (the map row is a plain name).  GUESS name (registry). */
#ifndef MHTRI_MENU_PUT_FRAME_SIDE_ARROWS_H
#define MHTRI_MENU_PUT_FRAME_SIDE_ARROWS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802A8650: the left/right arrows (dialog pieces 22 and 23) beside the box x,y,w,h while the pointer is off; `moved`
 * is the cursor's last move word. */
void put_frame_side_arrows(s16 x, s16 y, s16 width, s16 height, u16 moved, u32 a, u32 b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PUT_FRAME_SIDE_ARROWS_H */
