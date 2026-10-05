/* Leaf header for the two niku-table entry points the enemy action band calls (`stage/shell.cpp`, rule 2):
 * the owner's full header defines the shell work record and the pools, which the enemy units do not need.
 */
struct ShellNikuEntry;
struct _ENEMY_WORK;

#ifndef MHTRI_STAGE_NIKU_FIND_H
#define MHTRI_STAGE_NIKU_FIND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The live niku row of `player` with sequence number `seq`, or 0. */
struct ShellNikuEntry* niku_find(u8 player, u8 seq);
/* 1 when the row's shell has no serial entry, or its serial entry is finished (state 4) with the enemy's
 * word; otherwise 0 (a null row answers 0). */
u32 niku_enemy_serial_matches(struct ShellNikuEntry* entry, struct _ENEMY_WORK* enemy);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_NIKU_FIND_H */
