/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_companion_ui.cpp`'s `lb_tutorial_step`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_LOBBY_LB_TUTORIAL_STEP_H
#define MHTRI_LOBBY_LB_TUTORIAL_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8033A160 (0x4B4): the tutorial prompts' per-frame step. */
void lb_tutorial_step(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_TUTORIAL_STEP_H */
