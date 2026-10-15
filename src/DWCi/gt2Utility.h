/*
 * DWCi/gt2Utility.h - the declarations `src/DWCi/gt2Utility.c` owns that other units call (docs/plan.md 6.5 rule 2):
 *   the GT2 byte-order and address helpers, and the GameSpy stats server host name `DWC_Init` rewrites.
 */
#ifndef MHTRI_DWCI_GT2UTILITY_H
#define MHTRI_DWCI_GT2UTILITY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* host-to-network byte order for a port */
u16 DWCi_htons(u16 port);
/* formats an IPv4 address and port as text into `buf` (or an internal buffer when `buf` is NULL) */
char* DWCi_formatAddress(u32 addr, u16 port, char* buf);
/* parses "host:port" into an address and a port; 0 on a bad port or an unresolvable host */
u32 DWCi_parseAddress(char* str, u32* outAddr, u16* outPort);

/* 0x806308A8 - the GameSpy stats server host name, a 64-byte buffer `DWC_Init` rewrites for the server type.
 * GUESS on the name. */
extern char DWCi_statsServerHostname[64];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_GT2UTILITY_H */
