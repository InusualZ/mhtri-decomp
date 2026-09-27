/*
 * OS declarations with no registered owner (docs/plan.md 6.5 rule 2).
 *
 * The Revolution SDK OS library's bootstrap helpers `OSRegisterVersion` (0x804CB380),
 * `OSDisableInterrupts` (0x804D0C70) and `OSRestoreInterrupts` (0x804D0CB0) are called by units whose
 * own ranges the OS module's registered units do not cover: `NWC24/nwc24_msg.c` registers the NWC24
 * library's version string through `OSRegisterVersion` and brackets its message-library state update
 * with the interrupt pair.  The module the band names is `OS` (its bracketing registered units are the
 * OS band's), and no registered unit defines any of the three, so this file is their home.
 *
 * Added with the `NWC24/nwc24_msg.c` registration (the NWC24 SDK band, 0x8051D710-0x8051E864).
 */
#ifndef MHTRI_UNSPLIT_OS_H
#define MHTRI_UNSPLIT_OS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CB380 - records a library's version string with the OS. */
void OSRegisterVersion(const char* version);

/* 0x804D0C70 / 0x804D0CB0 - disable interrupts, returning the previous state; restore it. */
BOOL OSDisableInterrupts(void);
void OSRestoreInterrupts(BOOL level);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_OS_H */
