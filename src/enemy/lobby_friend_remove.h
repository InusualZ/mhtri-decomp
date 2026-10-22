/* enemy/lobby_friend_remove.h - leaf header (docs/plan.md 6.5 rule 2): `enemy/em020_prog.cpp`'s friend removal, for `menu/movie.cpp`.  C linkage (the map rows are plain names). */
#ifndef MHTRI_ENEMY_LOBBY_FRIEND_REMOVE_H
#define MHTRI_ENEMY_LOBBY_FRIEND_REMOVE_H

#include "types.h"

struct LobbyRecentRow;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80378DF8 - asks the server to drop `who` from the friend list: 0 not there, 1 sent, 2 failed. */
s32 lobby_friend_remove(const struct LobbyRecentRow* who);
/* 0x80378EA8 - drops `who` from the local friend list. */
void lobby_friend_drop(const struct LobbyRecentRow* who);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_LOBBY_FRIEND_REMOVE_H */
