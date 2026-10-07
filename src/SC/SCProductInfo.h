/*
 * SC/SCProductInfo.h - declarations of the symbols owned by `SC/SCProductInfo.c` that other units call or read.
 */
#ifndef SC_SCPRODUCTINFO_H
#define SC_SCPRODUCTINFO_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804DD180 - the console's product area (the dump's name). */
s8 SCGetProductArea(void);

/* 0x804DD2C0 - the GAME tag's region code (0 JP, 1 US, 2 EU, 4 KR, 5 CN; -1 when unset).  NAME: a GUESS from the tag. */
s8 SCGetProductGameRegion(void);

/* 0x804DD210 / 0x804DD250 - the console's product code string (NULL when unset) and its serial number (non-zero on
 * success); the DWC login sends them as "%s%09d".  NAMES: SCGetProductCode and SCGetProductSN are GUESSes from that
 * use, not names recovered from the SDK. */
const char* SCGetProductCode(void);
BOOL SCGetProductSN(u32* serial);

#ifdef __cplusplus
}
#endif

#endif
