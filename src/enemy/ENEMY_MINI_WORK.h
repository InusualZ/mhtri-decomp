/* The small per-enemy work record `em_get_unique_work` hands back through its third parameter (`_ENEMY_MINI_WORK` in
 * that symbol's mangling); only the bytes the net sync and the quest kill bookkeeping read are named.
 * size: 0x24 (measured minimum: the highest byte any reader names is +0x22; the true extent is unmeasured) */
#ifndef MHTRI_ENEMY_ENEMY_MINI_WORK_H
#define MHTRI_ENEMY_ENEMY_MINI_WORK_H

#include "types.h"

struct _ENEMY_MINI_WORK {
    /* +0x00 */ u8 unused_0x00[0x02 - 0x00];
    /* +0x02 */ u8 monster_0x02;         /* the kind `em_set_kill_record` tallies */
    /* +0x03 */ u8 team_0x03;
    /* +0x04 */ u8 unused_0x04[0x08 - 0x04];
    /* +0x08 */ u8 state_0x08;           /* the mode the net events compare against (1, 3 or 4) */
    /* +0x09 */ u8 entry_0x09;           /* the enemy data entry `enemy/em022_prog.cpp`'s mini hooks look up */
    /* +0x0A */ u8 area_0x0A;            /* the area the mini work stands in (`enemy/em022_prog.cpp`) */
    /* +0x0B */ u8 area_prev_0x0B;       /* the area it stood in before the last move */
    /* +0x0C */ u8 mode_0x0C;            /* `enemy/em010_prog.cpp`'s mini step roams only in mode 3 */
    /* +0x0D */ u8 element_0x0D;         /* the quest element it belongs to (above 2: none) */
    /* +0x0E */ u8 unused_0x0E[0x12 - 0x0E];
    /* +0x12 */ u8 step_0x12;            /* the step counter the net events raise */
    /* +0x13 */ u8 unused_0x13;
    /* +0x14 */ u8 end_kind_0x14;        /* 2 when the enemy left without a kill (`em_mini_kill_kind_get`) */
    /* +0x15 */ u8 busy_0x15;            /* `enemy/em010_prog.cpp`'s state reset runs only while it is zero */
    /* +0x16 */ u16 order_0x16;          /* its spawn order (`quest_spawn_rec_find`) */
    /* +0x18 */ u8 unused_0x18[0x1C - 0x18];
    /* +0x1C */ u8 phase_0x1C;           /* the phase counter the net events compare against */
    /* +0x1D */ u8 area_slot_0x1D;       /* the area list its spawn record was filed under */
    /* +0x1E */ u8 unused_0x1E[0x20 - 0x1E];
    /* +0x20 */ u8 roam_state_0x20;      /* `enemy/em010_prog.cpp`'s mini step: 0 idle, 1 move timer armed */
    /* +0x21 */ u8 unused_0x21;
    /* +0x22 */ s16 move_timer_0x22;     /* the frames until the next area move (`enemy/em022_prog.cpp`), -1 = unarmed */
};

#endif /* MHTRI_ENEMY_ENEMY_MINI_WORK_H */
