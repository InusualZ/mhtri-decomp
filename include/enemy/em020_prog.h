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
/* .sbss 0x80794BF0 - the message id of the last failed network layer command (GUESS name). */
extern s32 net_layer_error_message;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM020_PROG_H */
