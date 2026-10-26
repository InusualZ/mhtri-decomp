/* `lb_cancel_prompt_draw`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_CANCEL_PROMPT_DRAW_H
#define MHTRI_LOBBY_LB_CANCEL_PROMPT_DRAW_H

#include "types.h"

struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80215754 - draws the cancel/settle prompt in state `kind` at `pos`. */
void lb_cancel_prompt_draw(u8 kind, struct _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_CANCEL_PROMPT_DRAW_H */
