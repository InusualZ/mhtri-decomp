/* hud/draw_font_align.h - leaf header for `draw_font_align`, which `src/hud/layout.cpp` defines with C linkage
 * (docs/plan.md 6.5 rule 2).  A leaf because the owner's header does not declare it yet. */
#ifndef MHTRI_HUD_DRAW_FONT_ALIGN_H
#define MHTRI_HUD_DRAW_FONT_ALIGN_H

#include "types.h"
#include "hud/layout_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E2358 - draws `str` in the box of `spr` at `pos` with the record's size and colour, centred by `flags`. */
void draw_font_align(const _SPR_DATA_* spr, s8* str, u32 flags, const _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_DRAW_FONT_ALIGN_H */
