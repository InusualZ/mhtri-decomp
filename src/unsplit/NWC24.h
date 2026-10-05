/*
 * NWC24 declarations with no registered owner (docs/plan.md 6.5 rule 2).
 *
 * The WiiConnect24 (NWC24) library's scheduler state and its device-path literals are referenced by
 * the registered `NWC24/nwc24_msg.c` and `NWC24/nwc24_io.c`, but neither address is inside a
 * `splits.txt` range: the sibling objects are `.text`-only (plus the small `.data`/`.sdata`/`.sbss`
 * fragments they do claim), and the `.bss` block, the `.data` literal run and the `.sbss` flags below
 * belong to dtk's auto data runs.  Rule 2 puts them here.
 *
 * Added with the `NWC24/nwc24_msg.c` body pass.
 */
#ifndef MHTRI_UNSPLIT_NWC24_H
#define MHTRI_UNSPLIT_NWC24_H

#include "types.h"
#include "unsplit/OS.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The library's work block (`.bss` 0x80766980, 0x180 B): the two mutexes every device request takes
 * and the request/response buffers the ioctls are handed.  Only the modelled prefix is addressed by
 * this band's bodies. size: 0x80 */
typedef struct NWC24RequestWork {
    /* +0x00 */ OSMutex mutex[2];    /* +0x00 the script-mode setter's, +0x18 the scheduler pair's */
    /* +0x30 */ u8 pad_0x30[0x10];
    /* +0x40 */ u32 inBuffer[8];     /* 32 B, memset by the initialiser, the command input */
    /* +0x60 */ u32 outBuffer[8];    /* 32 B, memset by the initialiser, the command result */
} NWC24RequestWork; /* size: 0x80 */

/* 0x80766980 - the work block. */
extern NWC24RequestWork sNwc24Work;

/* The device layer's own work block (`.bss` 0x80766B00, 0xE0 B) and its one-time-initialisation flag
 * (`.sbss` 0x807958B8) are NOT declared here: `NWC24/nwc24_io.c` is their only referencer (checked
 * with `tools/units/callers.py`), so it claims `.bss` 0x80766B00-0x80766C40 and `.sbss`
 * 0x807958B8-0x807958D8 and defines them itself (rule 12). */

/* 0x800031C0 - the RTC shadow the game keeps at the top of main memory: the cached user id that
 * `NWC24iGetUserId` falls back to and refreshes through `DCStoreRange`.  It has no reloc in the
 * original object, i.e. the source spelled the address out. */
#define NWC24_RTC_USER_ID (*(volatile u32*)0x800031C0)
#define NWC24_RTC_USER_ID_HI (*(volatile u32*)0x800031C4)

/* The device-path and error-report literals the device layer opens and reports through.  The two
 * copies of the request path (0x80631178 and 0x80631200) are what makes the NWC24 band two
 * translation units (`-str reuse` merges identical literals inside one TU).  `Nwc24RequestPath2`
 * (0x80631200) and `Nwc24RequestShutdownName` (0x80631214) are NOT declared here: `NWC24/nwc24_io.c`
 * claims `.data` 0x806311E8-0x8063122A and defines them itself (rule 12). */
extern const char Nwc24RequestPath[];      /* 0x80631178 "/dev/net/kd/request" */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_NWC24_H */
