/*
 * include/DWCi/fn_805113B0.h - the declarations `src/DWCi/fn_805113B0.c` owns that other units call
 * (docs/plan.md 6.5 rule 2): the address helpers the GameSpy interface unit formats its connect
 * address with.  A consumer includes this header instead of declaring the symbols itself.  Keep it
 * minimal - the signatures are the ones `fn_805113B0.c` defines.
 */
#ifndef MHTRI_DWCI_FN_805113B0_H
#define MHTRI_DWCI_FN_805113B0_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* host-to-network byte order for a port */
u16 DWCi_htons(u16 port);
/* formats an IPv4 address and port as text into `buf` (or an internal buffer when `buf` is NULL) */
char* DWCi_formatAddress(u32 addr, u16 port, char* buf);

#ifdef __cplusplus
}
#endif

/* Declarations moved here from `include/unsplit/DWCi.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

extern char DWCi_addressFormat[6];

extern char DWCi_addressFormatHost[3];

extern char DWCi_addressFormatPort[4];

/* 0x80795820 - the index of the two-buffer address-string ring `DWCi_formatAddress` writes into the
 * 0x807625C0 buffers (`DWCi_addressRing`, declared by the NATNEG unit's header - the word falls
 * inside its `.bss` run). */
extern u32 DWCi_addressRingIndex;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_FN_805113B0_H */
