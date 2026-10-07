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

/* size: 0x09 - the idle-mode record `SCGetIdleMode` fills (`SCFindByteArrayItem(info, 2, 9)`); only
 * `subIdle` is read by the NWC24 band.  The field names past `idle` are derived from the record's own
 * order (a GUESS - the image carries no spelling for them). */
typedef struct SCIdleModeInfo {
    /* +0x00 */ u8 idle;
    /* +0x01 */ u8 subIdle;
    /* +0x02 */ u8 subIdle2;
    /* +0x03 */ u8 disc;
    /* +0x04 */ u8 pad_0x04[0x05];
} SCIdleModeInfo; /* size: 0x09 */

/* 0x804DC9E0 - the IPL.AR item (1 = widescreen, else 0; 0 when absent).  NAME: a GUESS from the item name. */
u8 SCGetAspectRatio(void);

/* 0x804DCA40 - the IPL.DH display offset, clamped to -32..32 and rounded to even. */
s8 SCGetDisplayOffsetH(void);

/* 0x804DCAC0 - copy the idle-mode record out of the SC configuration. */
BOOL SCGetIdleMode(SCIdleModeInfo* info);

/* 0x804DCB40 - the IPL.PGS item (1 = progressive, else 0).  NAME: a GUESS from the item name. */
u8 SCGetProgressiveMode(void);

/* 0x804DCC00 - the IPL.SND sound mode (0..2, default 1); `SCGetScreenSaverMode` is in its leaf header. */
u8 SCGetSoundMode(void);

/* 0x804DCCA0 / 0x804DCCC0 / 0x804DCCD0 - the BT.DINF (0x1C B) and BT.CDIF (0x1D B) device tables.
 * NAMES: a GUESS from the item names; the tables are caller-owned byte ranges. */
BOOL SCGetBtDeviceInfoArray(void* array); /* untyped: byte range */
BOOL SCGetBtCmpDeviceInfoArray(void* array); /* untyped: byte range */
BOOL SCSetBtCmpDeviceInfoArray(const void* array); /* untyped: byte range */

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
