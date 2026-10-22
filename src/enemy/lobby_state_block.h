/* enemy/lobby_state_block.h - the leaf header for the lobby state block (`.bss` 0x806BF530, 0x2EB8 B), which
 * `enemy/em020_prog.cpp`'s `.bss` (0x806BF530..0x806C23E8) owns.  The name is a GUESS: the lobby's network update steps
 * through its +0x00 state byte, the network pat control counts the friend rows' timers down every frame and its
 * work-record init clears all 0x2EB8 bytes; the lobby/enemy units read its +0x03 quest-active byte.  A leaf header
 * because `lobby/lb_npc.cpp` declares the block as a byte array, which must not meet this type in one translation unit.
 */
#ifndef MHTRI_ENEMY_LOBBY_STATE_BLOCK_H
#define MHTRI_ENEMY_LOBBY_STATE_BLOCK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One 0x24-byte friend row: the name, the id text, the row's state byte (`requestPeerProfileById` answers 1 / -1 from
 * it), a flag and the count-down word the network pat control decrements every frame. */
struct LobbyTimerRow {
    /* +0x00 */ char name_0x00[0x14];
    /* +0x14 */ char id_0x14[0xA];
    /* +0x1E */ u8 state_0x1E;
    /* +0x1F */ u8 flag_0x1F;
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

/* The text fields of the lobby's lists, each its own type so a row copy goes field by field (as the retail copies do). */
struct LobbyNameText {
    /* +0x00 */ char text_0x00[0x14];
}; /* size: 0x14 */
struct LobbyIdText {
    /* +0x00 */ char text_0x00[0xA];
}; /* size: 0xA */
struct LobbyMailIdText {
    /* +0x00 */ char text_0x00[0xB];
}; /* size: 0xB */
struct LobbyMailBody {
    /* +0x00 */ char text_0x00[0xC1];
}; /* size: 0xC1 */

/* One 0x1E-byte recent-player row (the friend requests the lobby has seen): the name and the id text. */
struct LobbyRecentRow {
    /* +0x00 */ struct LobbyNameText name_0x00;
    /* +0x14 */ struct LobbyIdText id_0x14;
#ifdef __cplusplus
    void assign(const LobbyRecentRow* src);
#endif
}; /* size: 0x1E */

/* One 0xE0-byte lobby mail row: the sender's name, the sender's id text and the 0xC1-byte body. */
struct LobbyMailRow {
    /* +0x00 */ struct LobbyNameText name_0x00;
    /* +0x14 */ struct LobbyMailIdText id_0x14;
    /* +0x1F */ struct LobbyMailBody body_0x1F;
#ifdef __cplusplus
    void assign(const LobbyMailRow* src);
#endif
}; /* size: 0xE0 */

/* The friend list: the count and up to 50 rows (the network pat control sorts and copies it). */
struct LobbyFriendList {
    /* +0x000 */ s32 count_0x000;
    /* +0x004 */ struct LobbyTimerRow rows_0x004[50];
}; /* size: 0x70C */

/* The recent-player list: the count and up to 16 rows. */
struct LobbyRecentList {
    /* +0x000 */ s32 count_0x000;
    /* +0x004 */ struct LobbyRecentRow rows_0x004[16];
}; /* size: 0x1E4 */

/* The mail list: the count, the per-row flags and the rows. */
struct LobbyMailList {
    /* +0x000 */ s32 count_0x000;
    /* +0x004 */ u8 flags_0x004[0x10];
    /* +0x014 */ struct LobbyMailRow rows_0x014[17];
}; /* size: 0xEF4 */

/* The block.  size: 0x2EB8 (the map's object size, and the length the work-record init clears) */
struct LobbyTimerBlock {
    /* +0x0000 */ u8 step_0x0000;            /* the lobby network update's state */
    /* +0x0001 */ u8 substep_0x0001;
    /* +0x0002 */ u8 field_0x0002;
    /* +0x0003 */ u8 quest_active_0x0003;    /* 1 while a quest is posted */
    /* +0x0004 */ s32 peer_count_0x0004;     /* the `NetPeerList` the network control copies out: the count... */
    /* +0x0008 */ struct LobbyPeerCopy peers_0x0008[4]; /* ...and the four peer records */
    /* +0x0488 */ u8 block_0x0488[0xD4C];
    /* +0x11D4 */ u8 block_0x11D4[0x212];
    /* +0x13E6 */ u8 rows_0x13E6[12][0x1F];  /* twelve 31-byte rows, each cleared by its first word */
    /* +0x155A */ u16 field_0x155A;
    /* +0x155C */ u8 request_0x155C;         /* set, the update raises the lobby notice */
    /* +0x155D */ u8 busy_0x155D;
    /* +0x155E */ u8 pad_0x155E[0x2];
    /* +0x1560 */ u8 block_0x1560[0x124];
    /* +0x1684 */ u8 card_sent_0x1684;       /* 1 once this player's card went out */
    /* +0x1685 */ u8 pad_0x1685[0x3];
    /* +0x1688 */ u32 field_0x1688;
    /* +0x168C */ s16 move_frames_0x168C;    /* the remote players' walk blend length */
    /* +0x168E */ s16 timeout_0x168E[3];     /* the server's big-data timeout elements 3, 4 and 1 */
    /* +0x1694 */ u8 field_0x1694;
    /* +0x1695 */ s8 result_0x1695;          /* the friend-list request's result code */
    /* +0x1696 */ s8 request_0x1696;         /* the community request's state: 0 pending, 1 done, else failed */
    /* +0x1697 */ u8 pad_0x1697;
    /* +0x1698 */ u16 field_0x1698[9];
    /* +0x16AA */ u8 pad_0x16AA[0x2];
    /* +0x16AC */ u32 field_0x16AC;
    /* +0x16B0 */ char self_id_0x16B0[0xB];  /* this player's id text */
    /* +0x16BB */ char self_name_0x16BB[0x15]; /* this player's name */
    /* +0x16D0 */ union {
        struct LobbyFriendList friends_0x16D0;
        /* the network pat control's view: the count and the rows it ticks.  size: 0x70C */
        struct {
            /* +0x16D0 */ s32 timer_count_0x16D0;
            /* +0x16D4 */ struct LobbyTimerRow timers_0x16D4[50];
        }; /* size: 0x70C */
    }; /* size: 0x70C */
    /* +0x1DDC */ struct LobbyRecentList recent_0x1DDC;
    /* +0x1FC0 */ u8 pad_0x1FC0[0x4];
    /* +0x1FC4 */ struct LobbyMailList mail_0x1FC4;
}; /* size: 0x2EB8 */

extern struct LobbyTimerBlock lobby_state_block;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_LOBBY_STATE_BLOCK_H */
