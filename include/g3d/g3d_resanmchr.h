/* The owner header of `g3d/g3d_resanmchr.cpp` (rule 2): the cross-unit declarations its symbols
 * need.  Written by `enemy/em_action.cpp`'s lane with the one declaration it needed.
 */
#ifndef MHTRI_G3D_G3D_RESANMCHR_H
#define MHTRI_G3D_G3D_RESANMCHR_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8008EE68 - the caller's matrix times the joint's world matrix (`enemy/fn_80138074.c` spells the
 * second argument `Mtx34*`; only the pointer width is load-bearing). */
void fn_8008EE68(void* arg0, MTX34* mtx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_RESANMCHR_H */
