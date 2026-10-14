/* WPAD/WPADMem.h - the memory-block reset, the game-data transfers and the remote memory reads and writes `WPAD/WPADMem.cpp` owns
 *   (docs/plan.md 6.5 rule 2). */
#ifndef WPAD_WPADMEM_H
#define WPAD_WPADMEM_H

#include "types.h"
#include "WPAD/wpad.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804F8EC0..0x804F9A10, in address order. */
void WPADiClearMemBlock(s32 chan);
void wpadGameDataCallback(s32 chan, s32 result);
void wpadBulkWriteNext(s32 chan, s32 result);
void wpadBulkWriteFirst(s32 chan, s32 result);
void wpadSetDpdStatusA(s32 chan, s32 error);
void wpadSetDpdStatusB(s32 chan, s32 error);
s32 WPADWriteGameData(s32 chan, const void* src, u16 size, u32 addr, WPADCallback callback); /* untyped: byte range */
s32 WPADReadGameData(s32 chan, void* dest, u16 size, u32 addr, WPADCallback callback); /* untyped: caller-owned byte buffer */
s32 wpadReadMemory(s32 chan, void* dest, u16 size, u32 addr, WPADCallback callback); /* untyped: caller-owned byte buffer */
s32 wpadWriteMemory(s32 chan, const void* src, s32 size, u32 addr, WPADCallback callback); /* untyped: byte range */
s32 wpadGetDpdCalibration(s32 chan, u8** out);
void wpadCopyTitleString(const u16* title);
s32 WPADWriteExtReg(s32 chan, const void* src, s32 size, u32 addr, WPADCallback callback); /* untyped: byte range */

#ifdef __cplusplus
}
#endif

#endif /* WPAD_WPADMEM_H */
