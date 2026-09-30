/* The small per-enemy work record `em_get_unique_work` hands back through its third parameter (the map
 * spells the type `_ENEMY_MINI_WORK` in that symbol's mangling).  Only the bytes the enemy-control net sync
 * reads are named; its extent is not measured.
 * size: 0x1D (measured minimum: the highest byte any reader names is +0x1C; the true extent is unmeasured) */
#ifndef MHTRI_ENEMY_ENEMY_MINI_WORK_H
#define MHTRI_ENEMY_ENEMY_MINI_WORK_H

#include "types.h"

struct _ENEMY_MINI_WORK {
    /* +0x00 */ u8 unused_0x00[0x08 - 0x00];
    /* +0x08 */ u8 state_0x08;           /* the mode the net events compare against (1, 3 or 4) */
    /* +0x09 */ u8 unused_0x09[0x12 - 0x09];
    /* +0x12 */ u8 step_0x12;            /* the step counter the net events raise */
    /* +0x13 */ u8 unused_0x13[0x1C - 0x13];
    /* +0x1C */ u8 phase_0x1C;           /* the phase counter the net events compare against */
};

#endif /* MHTRI_ENEMY_ENEMY_MINI_WORK_H */
