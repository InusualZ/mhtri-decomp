/* enemy/em020_prog.h - declarations `enemy/em020_prog.cpp` owns that other units use (the lobby-message tail). */
#ifndef MHTRI_ENEMY_EM020_PROG_H
#define MHTRI_ENEMY_EM020_PROG_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80378184 - queues the member-join announcement for a joining member's id text (GUESS name). */
void sendMemberJoinNotice(const char* id_text);
/* 0x80378B70 - drops lobby mail `index`, moving the later ones up; 1 when dropped (GUESS name). */
u8 dropLobbyMail(s32 index);
/* 0x80378D48 - adds a friend notice row (id text, name) to the lobby's notice list (GUESS name). */
void addFriendNotice(const char* id_text, const char* name);
/* 0x80377110 - releases the parts and model of remote player slots 1..9 whose lobby slot record still points at
 * their move work (GUESS name); the network control calls it when the layer changes. */
void releaseRemotePlayerParts(void);
/* One 0x30-byte lobby message (the network pat control posts the member ones: kind 2 a member joined, 3 one left,
 * with the member's slot and peer record; the arena blocks carry the rest). */
struct NetLobbyMessage {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 valid_0x01;
    /* +0x02 */ u8 data_0x02[0x22];
    /* +0x24 */ u8 slot_0x24;
    /* +0x25 */ u8 pad_0x25[0x3];
    /* +0x28 */ struct NetPeerRec* peer_0x28;
    /* +0x2C */ u8 pad_0x2C[0x4];
}; /* size: 0x30 */
/* 0x80378464 - dispatch one 0x30-byte lobby message by kind to the member, roster and quest-page handlers
 * (GUESS name). */
void handleLobbyNetMessage(struct NetLobbyMessage* message);
/* 0x8037583C - whether the shared lobby block's +0x03 quest-active byte is 1. */
u32 em020_quest_active_ck(void);
/* .sbss 0x80794BF0 - the message id of the last failed network layer command (GUESS name). */
extern s32 net_layer_error_message;

/* 0x80375858 - clears the lobby state block's quest-active byte. */
void em020_quest_active_clear(void);
/* 0x803759BC - stores the lobby message band's flag byte. */
void em020_unknown_flag_set(u8 flag);
/* 0x80376978 - clears the hunter-card pages. */
void em020_quest_pages_clear(void);
/* 0x803759C4 - draws the lobby's network error window from `lobby_w`'s error message (GUESS name). */
void lobby_net_err_draw(void);

/* 0x803777C8 - sends the user profile's part 2 when online (GUESS name). */
void em020_profile_send(void);
/* 0x80377348 - sends the served meal's skills and bonuses to the area `area` (a 0x36-byte packet).  GUESS name. */
void meal_result_send(u8 area);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM020_PROG_H */
