/* Leaf header (docs/plan.md 6.5 rule 2): `menu/menu_result.cpp`'s `multi_box_result_step`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_MENU_MULTI_BOX_RESULT_STEP_H
#define MHTRI_MENU_MULTI_BOX_RESULT_STEP_H

#include "types.h"

struct Q_MoveWork;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8039E348 (0x284): steps the versus box result screen; the choice it returns picks the arena sub-mode. */
s32 multi_box_result_step(struct Q_MoveWork* work);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MULTI_BOX_RESULT_STEP_H */
