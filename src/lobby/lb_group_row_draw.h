/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_80212810.cpp`'s hunter-group row helpers, which
 * `lobby/lb_companion_ui.cpp` calls.  C linkage (the map rows are plain names). */
#ifndef MHTRI_LOBBY_LB_GROUP_ROW_DRAW_H
#define MHTRI_LOBBY_LB_GROUP_ROW_DRAW_H

#include "types.h"

struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80215E6C (0x18): hunter group `id`'s colour. */
u32 lb_group_color_get(u8 id);
/* 0x80215E84 (0x108): draws the hunter-group row `id` with its colour badge and name at `pos`. */
void lb_group_row_draw(s32 id, bool enabled, bool sel, bool active, bool marked, u16 anim, struct _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_GROUP_ROW_DRAW_H */
