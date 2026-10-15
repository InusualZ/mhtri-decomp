/*
 * NET/netmisc.h - declarations of the symbols owned by `NET/netmisc.c` that other units call: the RevoEX NET
 *   startup-error helper, the wireless MAC wrapper and the NET memcpy/memset.
 */
#ifndef MHTRI_NET_NETMISC_H
#define MHTRI_NET_NETMISC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8051D048 - the user-facing network error code for a failed `SOStartup` result (the SDK's NET
 * helper of that name; GUESS from its neighbours and its one caller, which logs the negated
 * result as "Network Error Code is %d"). */
s32 NETGetStartupErrorCode(s32 result);

/* 0x8051D248 - copies the wireless MAC address (6 bytes) into `address`. GUESS on the name. */
void NCDGetWirelessMacAddress(u8* address);

/* 0x8051D24C / 0x8051D60C - the NET library's memcpy / memset. */
/* untyped: byte range */
void* NETMemCpy(void* dst, const void* src, u32 size);
/* untyped: byte range */
void* NETMemSet(void* dst, s32 value, u32 size);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NET_NETMISC_H */
