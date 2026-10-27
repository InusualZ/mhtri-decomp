/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_gesture_ready_ck`, the one declaration
 * `menu/menu_plsearch.cpp` needs.  GUESS name, from the body (listed on the NAMES line of menu/menu_plsearch.cpp). */
#ifndef MHTRI_PL_PL_GESTURE_READY_CK_H
#define MHTRI_PL_PL_GESTURE_READY_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80274904 (0x14): 1 once the player's armed/gesture latch (+0x5C9) is set */
s32 pl_gesture_ready_ck(struct _PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_GESTURE_READY_CK_H */
