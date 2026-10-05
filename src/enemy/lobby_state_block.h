/*
 * include/enemy/lobby_state_block.h - the leaf header for the lobby state block (`.bss` 0x806BF530, 0x2EB8 B),
 * which `enemy/em020_prog.cpp`'s `.bss` range (0x806BF530..0x806C23E8) owns (docs/plan.md 6.5 rule 2).
 *
 * Name: GUESS `lobby_state_block` - the network pat control counts its timer rows down every frame and its
 * work-record init clears all 0x2EB8 bytes; the lobby/enemy units read its +0x03 quest-active byte.  Only the
 * timer table is viewed here.  A leaf header because the owner and `lobby/lb_npc.cpp` still declare the block
 * as a byte array (their `[3]` / `+ 0x13E6` reads), which must not meet this type in one translation unit.
 */
#ifndef MHTRI_ENEMY_LOBBY_STATE_BLOCK_H
#define MHTRI_ENEMY_LOBBY_STATE_BLOCK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One 0x24-byte timer row: the count-down word, then the row's other words. */
struct LobbyTimerRow {
    /* +0x00 */ s32 timer_0x00;
    /* +0x04 */ u8 pad_0x04[0x20];
}; /* size: 0x24 */

/* The block, viewed as its timer table: the count at +0x16D0 and the rows from +0x16F4.  size: 0x2EB8 (the
 * map's object size, and the length the work-record init clears) */
struct LobbyTimerBlock {
    /* +0x0000 */ u8 pad_0x0000[0x16D0];
    /* +0x16D0 */ s32 timer_count_0x16D0;
    /* +0x16D4 */ u8 pad_0x16D4[0x20];
    /* +0x16F4 */ struct LobbyTimerRow timers_0x16F4[169];
}; /* size: 0x2EB8 */

extern struct LobbyTimerBlock lobby_state_block;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_LOBBY_STATE_BLOCK_H */
