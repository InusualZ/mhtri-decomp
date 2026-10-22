/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_80212810.cpp`'s `lb_button_prompt_draw`.  C linkage (the map row is
 * a plain name). */
#ifndef MHTRI_LOBBY_LB_BUTTON_PROMPT_DRAW_H
#define MHTRI_LOBBY_LB_BUTTON_PROMPT_DRAW_H

#include "types.h"

struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021591C - draws the button prompt `kind` at `pos`. */
s32 lb_button_prompt_draw(s32 kind, struct _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_BUTTON_PROMPT_DRAW_H */
