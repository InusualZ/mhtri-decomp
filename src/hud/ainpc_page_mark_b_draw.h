/* Leaf header (docs/plan.md 6.5 rule 2): `hud/cockpit.cpp`'s `ainpc_page_mark_b_draw`, which
 * `lobby/lb_companion_ui.cpp` calls (the owner's `hud/cockpit.h` cannot be included beside it). */
#ifndef MHTRI_HUD_AINPC_PAGE_MARK_B_DRAW_H
#define MHTRI_HUD_AINPC_PAGE_MARK_B_DRAW_H

#include "types.h"

struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802DE3F0 (0x90): draws the page's second button mark at layout `panel` (moved by `offset` when given). */
void ainpc_page_mark_b_draw(u16 panel, const struct _mh_ivec2_* offset);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_AINPC_PAGE_MARK_B_DRAW_H */
