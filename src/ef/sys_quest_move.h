/* ef/sys_quest_move.h - the quest move work record type `ef/system_core.cpp` owns (`get_move_work_adrs(0)`), shared with
 *   `menu/menu_item.cpp` (rule 1: one definition). */
#ifndef MHTRI_EF_SYS_QUEST_MOVE_H
#define MHTRI_EF_SYS_QUEST_MOVE_H

#include "types.h"

/* The quest move work (`get_move_work_adrs(0)`), as far as the system core and the item menu read it.
 * size: 0x114 (lower bound, an approximation) */
typedef struct SysQuestMove {
    /* +0x000 */ u8 unused_0x000[0xFA];
    /* +0x0FA */ u8 result_0x0FA;    /* 7 ends the quest with its result */
    /* +0x0FB */ u8 unused_0x0FB[0x112 - 0x0FB];
    /* +0x112 */ u8 menu_lock_0x112; /* 1 keeps the item menu from opening (`menu/menu_item.cpp`'s frame) */
    /* +0x113 */ u8 state_0x113;     /* 1 while the quest's sub-state machine runs */
} SysQuestMove;

#endif /* MHTRI_EF_SYS_QUEST_MOVE_H */
