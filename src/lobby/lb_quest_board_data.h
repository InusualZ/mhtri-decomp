/* The quest-board payload record `lobby/lb_quest_board.cpp`'s screen work points at, on its own so a unit that
 * reads it (`enemy/em_pop.cpp`'s extra-condition tests) need not take the board's lobby views. */
#ifndef MHTRI_LOBBY_LB_QUEST_BOARD_DATA_H
#define MHTRI_LOBBY_LB_QUEST_BOARD_DATA_H

#include "types.h"

/* The record `LbQuestBoardWork::data_0x264` points at: the screen's payload buffer (the block's
 * whole body is memset before it is filled, so only the offsets the band touches are named).
 * size: 0xB03 (lower bound) */
typedef struct LbQuestBoardData {
    /* +0x000 */ u8 pad_0x000[0x14];
    /* +0x014 */ u8 field_0x014;
    /* +0x015 */ u8 pad_0x015[0x13C - 0x015];
    /* +0x13C */ struct { /* size: 0xC */
        /* +0x0 */ u8 unused_0x0[6];
        /* +0x6 */ u16 count_0x6;   /* `quest_ex_condition_ck`'s kind 3 needs all six at 0 */
        /* +0x8 */ u8 unused_0x8[4];
    } rows_0x13C[6];
    /* +0x184 */ u8 pad_0x184[0xB01 - 0x184];
    /* +0xB01 */ u8 flags_0xB01;
    /* +0xB02 */ u8 param_0xB02;
} LbQuestBoardData;

#endif /* MHTRI_LOBBY_LB_QUEST_BOARD_DATA_H */
