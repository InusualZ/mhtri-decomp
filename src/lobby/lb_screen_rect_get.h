/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_80219260`'s `lb_screen_rect_get`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_LOBBY_LB_SCREEN_RECT_GET_H
#define MHTRI_LOBBY_LB_SCREEN_RECT_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021A664 (lobby/fn_80219260): the lobby screen window's rectangle. */
void lb_screen_rect_get(s16* x, s16* y, s16* w, s16* h);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SCREEN_RECT_GET_H */
