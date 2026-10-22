/* `lb_anim_mark_draw`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_ANIM_MARK_DRAW_H
#define MHTRI_LOBBY_LB_ANIM_MARK_DRAW_H

#include "types.h"

struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws an animated mark and, when `flag` is clear, its overlay. */
void lb_anim_mark_draw(u8 frame, u8 flag, struct _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_ANIM_MARK_DRAW_H */
