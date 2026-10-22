/* `lb_title_frame_draw`, defined by `lobby/fn_801E0ADC.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_TITLE_FRAME_DRAW_H
#define MHTRI_LOBBY_LB_TITLE_FRAME_DRAW_H

#include "types.h"

struct LbList;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws the title list `list` with each row's kind from `kinds`. */
void lb_title_frame_draw(struct LbList* list, u8* kinds);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_TITLE_FRAME_DRAW_H */
