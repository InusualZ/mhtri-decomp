/* Leaf header (docs/plan.md 6.5 rule 2): the `font/flfnt` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_FONT_FONT_PRINT_PLAIN_H
#define MHTRI_FONT_FONT_PRINT_PLAIN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8005C8F0 - font_print_ex's twin: locates the font at x,y, sets the colour and prints the formatted text, skipped while system_w's mode 3 suppresses it; the item menu prints the hunter name through it */
void font_print_plain(s16 x, s16 y, s16 color, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FONT_FONT_PRINT_PLAIN_H */
