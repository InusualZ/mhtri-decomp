/*
 * NCD/ncdsystem.h - declarations of the symbols owned by `NCD/ncdsystem.c` that other units call: the NCD
 *   configuration readers and the NET startup-error helper.
 */
#ifndef MHTRI_NCD_NCDSYSTEM_H
#define MHTRI_NCD_NCDSYSTEM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8051C554 - fill the caller's interface-configuration block; non-zero on failure (the DWCi
 * runtime initialiser prints its own " NCDGetCurrentIfConfig failed.[%d]\n" with the answer and
 * hands it the `+0x4000` region of its runtime block).  The name is read off that call site's own
 * message and the range's own "NCDGetCurrentIfConfig" string. */
s32 NCDGetCurrentIfConfig(u8* config);

/* 0x8051D248 - copies the wireless MAC address (6 bytes) into `address`. GUESS on the name. */
void NCDGetWirelessMacAddress(u8* address);

/* 0x8051C64C - fill the caller's NCD IP configuration block; negative on failure (`NHTTPi_Startup`
 * passes its own system-info block and panics on a negative answer). */
/* untyped: caller-owned payload */
s32 NCDGetCurrentIpConfig(void* config);

/* 0x8051D048 - the user-facing network error code for a failed `SOStartup` result (the SDK's NET
 * helper of that name; GUESS from its NCD-band neighbours and its one caller, which logs the negated
 * result as "Network Error Code is %d"). */
s32 NETGetStartupErrorCode(s32 result);

/* 0x8051C7A4 - the network link state (-8 / 1 while it is still coming up, 2 when the cable is out, negative on
 * failure). GUESS on the name: the SDK's NCD reader of that name, from SOStartupEx's wait on it. */
s32 NCDGetLinkStatus(void);

/* 0x8051D24C / 0x8051D60C - the NET library's memcpy / memset. */
/* untyped: byte range */
void* NETMemCpy(void* dst, const void* src, u32 size);
/* untyped: byte range */
void* NETMemSet(void* dst, s32 value, u32 size);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NCD_NCDSYSTEM_H */
