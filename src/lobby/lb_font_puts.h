/* lobby/lb_font_puts.h - leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_80212810.cpp`'s 0x80215414, for `menu/menu_placeinfo.cpp`.
 *   C linkage (the map row is a plain name); the signature follows the owner's body.  GUESS name (registry). */
#ifndef MHTRI_LOBBY_LB_FONT_PUTS_H
#define MHTRI_LOBBY_LB_FONT_PUTS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80215414: prints `str` at x,y in `color` through `flfntFontPuts`, with a shadow under flag 4. */
void lb_font_puts(s16 x, s16 y, s8* str, u32 color, u32 flags);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_FONT_PUTS_H */
