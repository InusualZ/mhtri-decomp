/* ef/sys_quest_move.h - the quest move work record type `ef/system_core.cpp` owns (`get_move_work_adrs(0)`), shared with
 *   `menu/menu_item.cpp` (rule 1: one definition). */
#ifndef MHTRI_EF_SYS_QUEST_MOVE_H
#define MHTRI_EF_SYS_QUEST_MOVE_H

#include "types.h"
#include "nw4r/math.h"

/* The quest move work (`get_move_work_adrs(0)`), as far as the system core and the item menu read it.
 * size: 0x114 (lower bound, an approximation) */
typedef struct SysQuestMove {
    /* +0x000 */ u8 unused_0x000[0xDC];
    /* +0x0DC */ struct QuestWork* quest_0x0DC;  /* the running quest's work (`ai/ai_npc.cpp` asks its element picks) */
    /* +0x0E0 */ u8 unused_0x0E0[0xEB - 0xE0];
    /* +0x0EB */ u8 area_0x0EB;      /* the stage area of `pos_0x128` (the cockpit ring's projection) */
    /* +0x0EC */ u8 unused_0x0EC[0xEF - 0xEC];
    /* +0x0EF */ u8 npc_plate_0x0EF;  /* non-zero while the AI NPC's name plate shows in the cockpit (GUESS) */
    /* +0x0F0 */ u8 unused_0x0F0[0xFA - 0xF0];
    /* +0x0FA */ u8 result_0x0FA;    /* 7 ends the quest with its result */
    /* +0x0FB */ u8 unused_0x0FB[0x112 - 0x0FB];
    /* +0x112 */ u8 menu_lock_0x112; /* 1 keeps the item menu from opening (`menu/menu_item.cpp`'s frame) */
    /* +0x113 */ u8 state_0x113;     /* 1 while the quest's sub-state machine runs */
    /* +0x114 */ u8 unused_0x114[0x128 - 0x114];
    /* +0x128 */ nw4r::math::VEC3 pos_0x128; /* the world position the cockpit's quest ring centres on (`hud/cockpit_quest.cpp`) */
    /* +0x134 */ u8 unused_0x134[0x22DD - 0x134];
    /* +0x22DD */ u8 slot_state_0x22DD[4]; /* the four server slots' states; 4 is the idle one (`camera/camera_main.cpp`) */
} SysQuestMove;

#endif /* MHTRI_EF_SYS_QUEST_MOVE_H */
