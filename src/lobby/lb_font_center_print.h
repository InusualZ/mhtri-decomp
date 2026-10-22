/* lobby/lb_font_center_print.h - leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_80212810.cpp`'s 0x802154A8, for `menu/menu_placeinfo.cpp`.
 *   C linkage (the map row is a plain name); the signature follows the owner's body.  GUESS name (registry). */
#ifndef MHTRI_LOBBY_LB_FONT_CENTER_PRINT_H
#define MHTRI_LOBBY_LB_FONT_CENTER_PRINT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802154A8: prints `str` centred on x and on y at font size `size`, in `color`, with a shadow under flag 4. */
void lb_font_center_print(s16 x, s16 y, s8* str, s16 size, u32 color, u32 flags);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_FONT_CENTER_PRINT_H */
