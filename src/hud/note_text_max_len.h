/* Leaf header (docs/plan.md 6.5 rule 2): `hud/layout.cpp`'s note line-width helper, for a consumer that cannot include
 * `hud/layout.h` (its `pl.h` and `_mh_ivec2_` meet `menu/menu_item_page.h`'s views).  C linkage (the map row is the
 * plain name). */
#ifndef MHTRI_HUD_NOTE_TEXT_MAX_LEN_H
#define MHTRI_HUD_NOTE_TEXT_MAX_LEN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E4270 - the widest line of `str` laid out at font size `size`. */
s16 note_text_max_len(u32 size, char* str);
/* 0x802E41B4 / 0x802E4340 - draw `text` at (`x`, `y`) in size `size`, as a plain line and as a note. */
void note_line_draw(s16 x, s16 y, s16 size, char* text);
void note_text_draw(s16 x, s16 y, s16 size, char* text);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_NOTE_TEXT_MAX_LEN_H */
