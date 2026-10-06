/*
 * VF/vf.h - declarations of the symbols owned by `VF/vf.cpp` that other units call: the prfile2 file-system
 *   bring-up, the KPR queue peek and two KBD channel calls.  The names are GUESSES (the `VF/vf.cpp`
 *   header): the VF three from the scheme their own callees use (`VFSysInit`, `VFipf2_init_prfile2`), the
 *   KPR/KBD three from their bodies.
 */
#ifndef MHTRI_VF_VF_H
#define MHTRI_VF_VF_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80521DE0 - 1 while the layers `VFipf2Init` brings up are up.  (The word it reads is the flag
 * `VFipf2Init` sets and `VFipf2Shutdown` clears.) */
u32 VFipf2IsInitialized(void);

/* 0x80521CA0 - under this band's own mutex, `VFSysInit(work, size)` followed by the disk manager,
 * prfile2 and dHash bring-up; the second argument is the work buffer's size (`DWCi_GetConsoleFriendCode`
 * hands it a freshly allocated 0x8000-byte block). */
void VFipf2Init(u8* work, u32 size);

/* 0x80521D70 - the matching take-down: finalize the file system and clear the flag. */
void VFipf2Shutdown(void);

/* 0x80526F00 - copy up to `max` queued u16 characters to `outAddress` under disabled interrupts and return
 * how many the queue holds (the bytes at +0x10/+0x11 summed); `(queue, 0, 0)` only counts. */
/* untyped: opaque band object, typed by the callers' views */
u32 KPRLookAhead(void* queue, u32 outAddress, u32 max);

/* 0x80529430 - allocate an LED request for the channel, store the value, the callback and its argument,
 * and hand it to the HID transfer; 7 when the request cannot be queued. */
/* untyped: opaque band object, typed by the callers' views */
u32 KBDSetLedsAsync(u32 index, u32 value, void* callback, u32 arg);

/* 0x80529B50 - store the word at +0x258 of the channel's 0x2A8-byte record; returns 0. */
void KBDSetChannelValue(u8 index, u32 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_VF_VF_H */
