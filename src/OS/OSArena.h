/*
 * OS/OSArena.h - declarations of the symbols owned by `OS/OSArena.c` that other units call or read.
 */
#ifndef OS_OSARENA_H
#define OS_OSARENA_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CC030..0x804CC080 - the upper and lower bounds of the MEM1, MEM2 and default arena. */
/* untyped: raw arena address */
void* OSGetMEM1ArenaHi(void);
/* untyped: raw arena address */
void* OSGetMEM2ArenaHi(void);
/* untyped: raw arena address */
void* OSGetArenaHi(void);
/* untyped: raw arena address */
void* OSGetMEM1ArenaLo(void);
/* untyped: raw arena address */
void* OSGetMEM2ArenaLo(void);
/* untyped: raw arena address */
void* OSGetArenaLo(void);

/* 0x804CC090..0x804CC0E0 - sets the same bounds. */
/* untyped: raw arena address */
void OSSetMEM1ArenaHi(void* newBound);
/* untyped: raw arena address */
void OSSetMEM2ArenaHi(void* newBound);
/* untyped: raw arena address */
void OSSetArenaHi(void* newBound);
/* untyped: raw arena address */
void OSSetMEM1ArenaLo(void* newBound);
/* untyped: raw arena address */
void OSSetMEM2ArenaLo(void* newBound);
/* untyped: raw arena address */
void OSSetArenaLo(void* newBound);

/* 0x804CC0F0 - takes `size` bytes aligned to `align` from the low end of the MEM1 arena. */
/* untyped: raw arena address */
void* OSAllocFromMEM1ArenaLo(u32 size, u32 align);

#ifdef __cplusplus
}
#endif

#endif
