/* Declarations owned by `src/SC/sc.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_SC_SC_H
#define MHTRI_SC_SC_H

#include "types.h"

/* Declarations moved here from `unsplit/SC.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

/* 0x804DB050 - starts the SC configuration reader. */
void SCInit(void);

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

/* 0x804DCAD0 - the console language (the U8 item IPL.LNG). */
u8 SCGetLanguage(void);

/* 0x804DD180 - the console's product area (the dump's name). */
s8 SCGetProductArea(void);

/* 0x804DD210 / 0x804DD250 - the console's product code string (NULL when unset) and its serial number (non-zero on
 * success); the DWC login sends them as "%s%09d".  NAMES: SCGetProductCode and SCGetProductSN are GUESSes from that
 * use, not names recovered from the SDK. */
const char* SCGetProductCode(void);
BOOL SCGetProductSN(u32* serial);

/* 0x804DCE90 - the country byte of the IPL.SADR simple-address record (item 16, 0x1008 bytes): TRUE and
 * `*country` set when the record is valid (its first word is not 0xFFFF and neither of its two top bytes is
 * 0 or 0xFF), else FALSE.  NAME (a GUESS): no log string covers it; it reads IPL.SADR's first byte, the
 * country code the mediator's `getCountryCode` hands on. */
BOOL SCGetCountryCode(u8* country);

/* 0x804DCF80 - bit 1 of the NET.CTPC parental-control word (item 20): the console restricts messaging with
 * other users.  The name is the one the function's own log line spells (`<< RVL_SDK -
 * SCCheckPCMessageRestriction >>`, .data 0x80629E88). */
BOOL SCCheckPCMessageRestriction(void);

/* 0x804DCFD0 - bit 2 of the same NET.CTPC word.  NAME (a GUESS in its sibling's scheme): no log line; its one
 * caller is the EC (shop) client's purchase gate, `NetworkPool::isPurchaseRestricted`. */
BOOL SCCheckPCShopRestriction(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SC_SC_H */
