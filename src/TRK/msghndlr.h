/*
 * TRK/msghndlr.h - the MetroTRK debugger command handlers, owned by `TRK/msghndlr.c`.
 */
#ifndef TRK_MSGHNDLR_H
#define TRK_MSGHNDLR_H

#include "types.h"
#include "TRK/msgbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8046AADC (0x8): returns whether the debugger has connected. */
s32 GetTRKConnected(void);

/* Every handler reads the command in `message`, replies to the debugger and returns 0 or an error code. */

/* 0x8046AAE4 (0x70) */
s32 TRK_DoConnect(TRKBuffer* message);

/* 0x8046AB54 (0x88) */
s32 TRKDoDisconnect(TRKBuffer* message);

/* 0x8046ABDC (0x6C) */
s32 TRKDoReset(TRKBuffer* message);

/* 0x8046AC48 (0x6C) */
s32 TRKDoOverride(TRKBuffer* message);

/* 0x8046ACB4 (0x234) */
s32 TRKDoReadMemory(TRKBuffer* message);

/* 0x8046AEE8 (0x210) */
s32 TRKDoWriteMemory(TRKBuffer* message);

/* 0x8046B0F8 (0x1FC) */
s32 TRKDoReadRegisters(TRKBuffer* message);

/* 0x8046B2F4 (0x2A0) */
s32 TRKDoWriteRegisters(TRKBuffer* message);

/* 0x8046B594 (0xC4) */
s32 TRKDoContinue(TRKBuffer* message);

/* 0x8046B658 (0x284) */
s32 TRKDoStep(TRKBuffer* message);

/* 0x8046B8DC (0xB8) */
s32 TRKDoStop(TRKBuffer* message);

/* 0x8046B994 (0xCC) */
s32 TRKDoSetOption(TRKBuffer* message);

#ifdef __cplusplus
}
#endif

#endif
