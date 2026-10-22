/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_gesture_req`, the one declaration
 * `menu/menu_sysmsg.cpp` needs.  GUESS name, from the body (listed on the NAMES line of menu/menu_sysmsg.cpp). */
#ifndef MHTRI_PL_PL_GESTURE_REQ_H
#define MHTRI_PL_PL_GESTURE_REQ_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80274584 (0x58): stores the gesture request into the player's own move work record */
void pl_gesture_req(s8 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_GESTURE_REQ_H */
