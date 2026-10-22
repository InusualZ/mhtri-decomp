/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `lobby/fn_80212810.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_TEXT_CENTER_PRINT_H
#define MHTRI_LOBBY_LB_TEXT_CENTER_PRINT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021555C - prints `text` centred in layout panel `panel` at `size`, in `rgba` (palette colour `color` when 0). */
void lb_text_center_print(u16 panel, s8* text, s16 color, s16 size, struct _mh_ivec2_* pos, u32 rgba);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_TEXT_CENTER_PRINT_H */
