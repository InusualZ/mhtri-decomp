/* enemy/lobby_state_block.h - the leaf header for the lobby state block (`.bss` 0x806BF530, 0x2EB8 B), which
 * `enemy/em020_prog.cpp`'s `.bss` (0x806BF530..0x806C23E8) owns.  The name is a GUESS: the network pat control counts
 * its timer rows down every frame and its work-record init clears all 0x2EB8 bytes; the lobby/enemy units read its
 * +0x03 quest-active byte.  Only the timer table is viewed; a leaf header because the owner and `lobby/lb_npc.cpp`
 * declare the block as a byte array, which must not meet this type in one translation unit.
 */
#ifndef MHTRI_ENEMY_LOBBY_STATE_BLOCK_H
#define MHTRI_ENEMY_LOBBY_STATE_BLOCK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One 0x24-byte timer row: the row's state byte (`requestPeerProfileById` answers 1 / -1 from it) and the count-down
 * word the network pat control decrements every frame. */
struct LobbyTimerRow {
    /* +0x00 */ u8 pad_0x00[0x1E];
    /* +0x1E */ u8 state_0x1E;
    /* +0x1F */ u8 pad_0x1F;
    /* +0x20 */ s32 timer_0x20;
}; /* size: 0x24 */

/* One 0x120-byte copy of a connected peer the lobby keeps (the network pat control's `updatePeerCardBlock` finds the
 * row by the peer's id and refreshes its card): the id text, the name and the 0x100-byte card. */
struct LobbyPeerCopy {
    /* +0x000 */ char id_0x00[0xA];
    /* +0x00A */ char name_0x0A[0x14];
    /* +0x01E */ u8 pad_0x1E[0x2];
    /* +0x020 */ u8 card_0x20[0x100];
}; /* size: 0x120 */

/* The block, viewed as its peer copies (+0x0008) and its timer table: the count at +0x16D0 and the rows from
 * +0x16D4.  size: 0x2EB8 (the map's object size, and the length the work-record init clears) */
struct LobbyTimerBlock {
    /* +0x0000 */ u8 pad_0x0000[0x8];
    /* +0x0008 */ struct LobbyPeerCopy peers_0x0008[4];
    /* +0x0488 */ u8 pad_0x0488[0x1248];
    /* +0x16D0 */ s32 timer_count_0x16D0;
    /* +0x16D4 */ struct LobbyTimerRow timers_0x16D4[169];
    /* +0x2E98 */ u8 pad_0x2E98[0x20];
}; /* size: 0x2EB8 */

extern struct LobbyTimerBlock lobby_state_block;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_LOBBY_STATE_BLOCK_H */
