/*
 * SC/SCApi.h - declarations of the symbols owned by `SC/SCApi.c` that other units call or read.
 */
#ifndef SC_SCAPI_H
#define SC_SCAPI_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804DCC60 - the SC's counter bias (the U32 item 0, or 0x0B49D800 when the configuration has none);
 * the value `NWC24iSynchronizeRtcCounter` rebases the device's RTC counter against.  NAME (a GUESS,
 * see `src/NWC24/nwc24_io.c`'s header): the dump answers `zz_` for it and it has no other caller. */
u32 SCGetCounterBias(void);

/* 0x804DCAD0 - the console language (the U8 item IPL.LNG). */
u8 SCGetLanguage(void);

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

#endif
