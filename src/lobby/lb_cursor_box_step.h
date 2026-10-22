/* lobby/lb_cursor_box_step.h - leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_pane_ui.cpp`'s colour box cursor, for `menu/movie.cpp`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_CURSOR_BOX_STEP_H
#define MHTRI_LOBBY_LB_CURSOR_BOX_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801EC9F8 - moves the box cursor (`x`, `y`) inside its bounds by `buttons`; 1 when it moved. */
s32 lb_cursor_box_step(s16* x, s16* y, s16 x_min, s16 x_max, s16 y_min, s16 y_max, u16 buttons);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_CURSOR_BOX_STEP_H */
