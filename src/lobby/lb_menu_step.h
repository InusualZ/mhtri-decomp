/* `lb_menu_step`, defined by `lobby/fn_80219260.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_MENU_STEP_H
#define MHTRI_LOBBY_LB_MENU_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Steps the lobby talk menu; returns 1 or 2 once the page is decided or cancelled. */
s32 lb_menu_step(u8 a, s32 b, s32 c, s32 d);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_MENU_STEP_H */
