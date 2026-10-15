/*
 * NCD/ncdsystem.h - declarations of the symbols owned by `NCD/ncdsystem.c` that other units call: the NCD
 *   configuration readers (the NET helpers are `NET/netmisc.h`).
 */
#ifndef MHTRI_NCD_NCDSYSTEM_H
#define MHTRI_NCD_NCDSYSTEM_H

#include "types.h"
#include "NET/netmisc.h" /* the NET helpers linked after NCD, which callers of this header use */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8051C554 - fill the caller's interface-configuration block; non-zero on failure (the DWCi
 * runtime initialiser prints its own " NCDGetCurrentIfConfig failed.[%d]\n" with the answer and
 * hands it the `+0x4000` region of its runtime block).  The name is read off that call site's own
 * message and the range's own "NCDGetCurrentIfConfig" string. */
s32 NCDGetCurrentIfConfig(u8* config);

/* 0x8051C64C - fill the caller's NCD IP configuration block; negative on failure (`NHTTPi_Startup`
 * passes its own system-info block and panics on a negative answer). */
/* untyped: caller-owned payload */
s32 NCDGetCurrentIpConfig(void* config);

/* 0x8051C7A4 - the network link state (-8 / 1 while it is still coming up, 2 when the cable is out, negative on
 * failure). GUESS on the name: the SDK's NCD reader of that name, from SOStartupEx's wait on it. */
s32 NCDGetLinkStatus(void);

/* 0x8051C8A8 - reads the wireless MAC address (6 bytes) through `/dev/net/ncd/manage`; negative on failure. */
s32 NCDiGetWirelessMacAddress(u8* address);

/* 0x8051C9D0 - the enabled profiles' masks by kind (wired, wireless, other wireless); negative on failure. */
s32 NCDiGetEnabledConfigList(u32* wired, u32* wireless, u32* wirelessOther);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NCD_NCDSYSTEM_H */
