/*
 * The quest result work (docs/plan.md 6.5 rule 1): the 0x438-byte `qResult` buffer `get_qResult_work` hands out
 * (`.bss` 0x8066A1E8, owned by `src/fn_8004CAD8.cpp`).  Three units used to carry their own prefix of it -
 * `quest/quest_entry.cpp` (`Q_ResultWork`), `menu/menu_result.cpp` (`QResultWork`) and
 * `lobby/fn_801F9CD4.cpp` (`LbChgQResult`); this is the one merged view, every named field at its offset.
 */
#ifndef MHTRI_QUEST_QUEST_RESULT_WORK_H
#define MHTRI_QUEST_QUEST_RESULT_WORK_H

#include "types.h"

/* The 16-byte quest stat the save block keeps and `quest_init` copies into the work block. */
struct Q_QuestStat {
    /* +0x00 */ u8 valid_0x00;     /* 1 when the stat is live */
    /* +0x01 */ u8 byte_0x01;
    /* +0x02 */ s16 half_0x02;
    /* +0x04 */ u32 word_0x04;
    /* +0x08 */ u32 flags_0x08;    /* bits 1/2/4/8 name the four slots the reward roll has already paid out */
    /* +0x0C */ u32 word_0x0C;
};  /* size: 0x10 */

/* The two 41-entry count runs the finished quest credits to the save block, the blocks
 * `quest_result_work_flush` clears, and the few bytes the result screen (`menu/menu_result.cpp`) and the
 * lobby colour-choice step (`lobby/fn_801F9CD4.cpp`) read.  Only those offsets are named. */
struct Q_ResultWork {
    /* +0x000 */ u16 count_a[0x29];
    /* +0x052 */ u16 count_b[0x29];
    /* +0x0A4 */ union {
        u8 block_0x0A4[0xA4];           /* cleared as one block by `quest_result_work_flush` */
        struct {
            /* +0x0A4 */ u8 pad_0x0A4[0x5C];
            /* +0x100 */ s32 gate_0x100;   /* GUESS name: the menu band's `> 0` gate */
            /* +0x104 */ u8 pad_0x104[0x44];
        };
    };
    /* +0x148 */ u8 pad_0x148[0x1E0 - 0x148];
    /* +0x1E0 */ u16 progress_0x1E0;   /* GUESS name: compared against 10000 by the result screen's phase 6 */
    /* +0x1E2 */ u8 phase_0x1E2;       /* the load phase: 2 while loading, 3 when done */
    /* +0x1E3 */ u8 kind_0x1E3;        /* the result kind (1 and 3 are checked by the lobby colour step) */
    /* +0x1E4 */ s32 field_0x1E4;      /* cleared by the flush; no reader names it */
    /* +0x1E8 */ s32 field_0x1E8;      /* cleared by the flush; no reader names it */
    /* +0x1EC */ u8 pad_0x1EC[0x304 - 0x1EC];
    /* +0x304 */ u8 block_0x304[0xA0];
    /* +0x3A4 */ u8 present_0x3A4;     /* a "records present" flag */
    /* +0x3A5 */ u8 pad_0x3A5[0x3E0 - 0x3A5];
    /* +0x3E0 */ Q_QuestStat stat_0x3E0;
    /* +0x3F0 */ s32 credit_0x3F0;     /* GUESS name: the amount the lobby colour step credits to the score */
    /* +0x3F4 */ u8 pad_0x3F4[0x438 - 0x3F4];
};  /* size: 0x438 */

#endif /* MHTRI_QUEST_QUEST_RESULT_WORK_H */
