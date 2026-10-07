/*
 * VF/vf.h - declarations of the symbols owned by `VF/vf.cpp` that other units call: the prfile2 file-system
 *   bring-up.  The names are GUESSES (the `VF/vf.cpp` header), from the scheme their own callees use
 *   (`VFSysInit`, `VFipf2_init_prfile2`).
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

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_VF_VF_H */
