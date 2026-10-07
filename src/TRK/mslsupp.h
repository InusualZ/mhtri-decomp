/*
 * TRK/mslsupp.h - the MSL console and file hooks over TRK, owned by `TRK/mslsupp.c`.
 */
#ifndef TRK_MSLSUPP_H
#define TRK_MSLSUPP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8046BA60 (0x68): reads console input through the host; returns 1 when serial I/O is off. */
s32 __read_console(s32 handle, u8* buffer, u32* count, void (*idle_proc)(void));

/* 0x8046BAC8 (0x68): writes console output through the host; returns 1 when serial I/O is off. */
s32 __TRK_write_console(s32 handle, u8* buffer, u32* count, void (*idle_proc)(void));

/* 0x8046BB30 (0x8): reads from a host file. */
s32 __read_file(s32 handle, u8* buffer, u32* count, void (*idle_proc)(void));

/* 0x8046BB38 (0x8): writes to a host file. */
s32 __write_file(s32 handle, u8* buffer, u32* count, void (*idle_proc)(void));

/* 0x8046BB40 (0xAC): runs one host file access with `command` (0xD0 write, 0xD1 read); returns 0, 2 (end of file) or 1. */
s32 __access_file(s32 handle, u8* buffer, u32* count, void (*idle_proc)(void), s32 command);

#ifdef __cplusplus
}
#endif

#endif
