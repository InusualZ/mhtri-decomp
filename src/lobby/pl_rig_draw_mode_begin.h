/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_equip_page.cpp`'s `pl_rig_draw_mode_begin`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_PL_RIG_DRAW_MODE_BEGIN_H
#define MHTRI_LOBBY_PL_RIG_DRAW_MODE_BEGIN_H

#include "types.h"

struct PlSeRig;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8022498C - sets the draw mode of the rig's models (mhchar_draw_mode_set) before the update */
void pl_rig_draw_mode_begin(struct PlSeRig* rig);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_PL_RIG_DRAW_MODE_BEGIN_H */
