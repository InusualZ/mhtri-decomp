/*
 * unsplit/NWC24.h - the NWC24 band's remaining spellings with no owner (docs/plan.md 6.5 rule 2): the RTC shadow
 * words the source spells as fixed addresses.
 *
 * The work block `sNwc24Work` (`.bss` 0x80766980) and the request path `Nwc24RequestPath` (`.data` 0x80631178)
 * are `NWC24/nwc24_msg.c`'s and are declared in `NWC24/nwc24_msg.h`.
 */
#ifndef MHTRI_UNSPLIT_NWC24_H
#define MHTRI_UNSPLIT_NWC24_H

#include "types.h"
#include "unsplit/OS.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_NWC24_H */
