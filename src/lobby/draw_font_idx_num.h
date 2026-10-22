/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `lobby/fn_80212810.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_DRAW_FONT_IDX_NUM_H
#define MHTRI_LOBBY_DRAW_FONT_IDX_NUM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802152A4 - prints the number `value` in layout text slot `id`. */
void draw_font_idx_num(u16 id, s32 value, u32 flags, const struct _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_DRAW_FONT_IDX_NUM_H */
