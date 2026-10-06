/* Leaf header (docs/plan.md 6.5 rule 2): the `lobby/lb_pane_ui.cpp` row the game-mode flow calls.  C linkage. */
#ifndef MHTRI_LOBBY_LOBBY_SCENE_SETUP_STEP_H
#define MHTRI_LOBBY_LOBBY_SCENE_SETUP_STEP_H

#include "types.h"

struct TaskSlot;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801FB80C - one step of the lobby scene's set-up for the game-mode task; non-zero once it is up (GUESS name). */
u8 lobby_scene_setup_step(struct TaskSlot* task);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LOBBY_SCENE_SETUP_STEP_H */
