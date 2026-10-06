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
/* 0x80378464 - dispatch one 0x30-byte lobby message by kind to the member, roster and quest-page handlers
 * (GUESS name). */
struct NetLobbyMessage;
void handleLobbyNetMessage(struct NetLobbyMessage* message);
/* .sbss 0x80794BF0 - the message id of the last failed network layer command (GUESS name). */
extern s32 net_layer_error_message;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM020_PROG_H */
