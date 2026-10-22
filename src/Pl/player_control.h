/* Pl/player_control.h - the declarations of `Pl/player_control.cpp` its consumers call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_PL_PLAYER_CONTROL_H
#define MHTRI_PL_PLAYER_CONTROL_H

#include "types.h"

#ifdef __cplusplus
/* 0x8026837C `player_control_release__Fv` - releases the player works (C++ scope in the owner). */
void player_control_release(void);
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* 0x80267548 - the player work array the system core's move work 2 points at. */
u32 player_work_array_get(void);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PLAYER_CONTROL_H */
