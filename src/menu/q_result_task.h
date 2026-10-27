/* Leaf header (docs/plan.md 6.5 rule 2): `menu/menu_result.cpp`'s `q_result_task`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_MENU_Q_RESULT_TASK_H
#define MHTRI_MENU_Q_RESULT_TASK_H

#include "types.h"

struct Q_MoveWork;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8039756C (0xB00): steps the quest result screen; non-zero once it is done. */
s32 q_result_task(struct Q_MoveWork* work);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_Q_RESULT_TASK_H */
