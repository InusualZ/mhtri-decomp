/*
 * enemy/em020_ai.h - the declarations `enemy/em020_ai.cpp` owns for the symbols of its `.text` range
 * that its consumers still reach through a declaration (docs/plan.md 6.5 rule 2: an `extern` lives
 * with the TU that owns the symbol).
 *
 * `getInstance_` (0x803768F0), `fn_803768F8` (0x803768F8) and `fn_80377664` (0x80377664) all sit
 * inside the unit's claimed range, so the consumers that used to read them out of
 * `include/unsplit/Network.h` / `include/unsplit/lobby.h` include this header instead.
 */
#ifndef MHTRI_ENEMY_EM020_AI_H
#define MHTRI_ENEMY_EM020_AI_H

#include "types.h"

/* The network singleton `getInstance_` returns: the owner of the record is
 * `include/unsplit/Network.h`, which only needs the tag here. */
struct NetworkInstance;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803768F0 - the network singleton (an 8-byte accessor). */
struct NetworkInstance* getInstance_(void);
/* 0x803768F8 - the lobby's "can leave the map" predicate (`lobby_w` +0x163/+0x16F/+0x0B0). */
u32 fn_803768F8(void);
/* 0x80377664 - the network big-data request entry (the `fn_804273EC(1, 0, 0)` wrapper). */
s32 fn_80377664(void* unused);

#ifdef __cplusplus
}
#endif

#endif
