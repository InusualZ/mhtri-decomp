/*
 * Declarations for the symbols `src/pad_connect.cpp` owns that other units use (docs/plan.md 6.5, rule 2).
 */
#ifndef MHTRI_PAD_CONNECT_H
#define MHTRI_PAD_CONNECT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* .bss 0x806694D0 - the game's OSMutex (0x18 B), initialised by the pad-connect init; the network heap's
 * allocators lock it around every MEM call. */
extern u8 game_mutex[0x18];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PAD_CONNECT_H */
