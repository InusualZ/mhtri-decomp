/* `lb_smith_deco_info_draw`, defined by `lobby/lb_screen_step.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_SMITH_DECO_INFO_DRAW_H
#define MHTRI_LOBBY_LB_SMITH_DECO_INFO_DRAW_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws the decoration info window of item `item`: its slot size, its two skills and its text. */
void lb_smith_deco_info_draw(struct _PLW* plw, u16 item);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SMITH_DECO_INFO_DRAW_H */
