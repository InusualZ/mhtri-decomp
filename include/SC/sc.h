/* Declarations owned by `src/SC/sc.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_SC_SC_H
#define MHTRI_SC_SC_H

#include "types.h"

/* Declarations moved here from `include/unsplit/SC.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

/* 0x804DB0F0 - the SC state machine: 0 = idle, 1 = busy, 2 = the configuration file was reloaded.
 * The band's compares are unsigned (`cmplwi r3,2` / `cmplwi r3,1`), so the status is `u32`. */
u32 SCCheckStatus(void);

/* 0x804DC280 / 0x804DBEB0 - read one typed item out of the SC configuration; FALSE when it is absent. */
BOOL SCFindU32Item(u32* value, u32 item);

BOOL SCFindByteArrayItem(void* value, u32 item, u32 size); /* untyped: byte range */

/* 0x804DCC60 - the SC's counter bias (the U32 item 0, or 0x0B49D800 when the configuration has none);
 * the value `NWC24iSynchronizeRtcCounter` rebases the device's RTC counter against.  NAME (a GUESS,
 * see `src/NWC24/nwc24_io.c`'s header): the dump answers `zz_` for it and it has no other caller. */
u32 SCGetCounterBias(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SC_SC_H */
