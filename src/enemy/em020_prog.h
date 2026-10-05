/*
 * Declarations for the symbols `src/enemy/em020_prog.cpp` owns that other units use (docs/plan.md 6.5, rule 2).
 */
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
/* 0x80377110 - for remote player slots 1..9: when the lobby's 0x130-byte slot record (0x806BE340) is active and
 * still points at that slot's move work, releases its parts models and its model (0x80269F04), then clears the
 * record.  NAME (a GUESS from the body); the network control calls it when the layer changes. */
void releaseRemotePlayerParts(void);
/* 0x80378464 - dispatches one 0x30-byte lobby message the network control's message pool delivers (a jump table
 * on its kind) to the lobby's member, roster and quest-page handlers.  NAME (a GUESS from the body). */
struct NetLobbyMessage;
void handleLobbyNetMessage(struct NetLobbyMessage* message);
/* .sbss 0x80794BF0 - the message id of the last failed network layer command (GUESS name). */
extern s32 net_layer_error_message;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM020_PROG_H */
