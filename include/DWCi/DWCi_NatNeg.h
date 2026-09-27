/*
 * include/DWCi/DWCi_NatNeg.h - the DWCi NATNEG / transport-tail unit (`src/DWCi/DWCi_NatNeg.c`,
 * `.text` 0x80512490..0x805145B8).
 *
 * Rule 2: this unit owns these symbols, so the GameSpy interface unit
 * reaches them through `include/unsplit/Network.h`, which includes this header.
 *
 * The five entry points the GameSpy interface calls are named from its call sites
 * (`DWCi_NatNegStartSession` -> the negotiation starter that takes the session id and the two
 * callbacks, `DWCi_NatNegEndSession` -> the teardown it is paired with, `DWCi_NatNegCleanup` ->
 * the global cleanup called next to it, `DWCi_NatNegProcess` -> the per-iteration step, and
 * `DWCi_NatNegSendPacket` -> the header+payload send).  Each is a GUESS - the map has only
 * placeholders for the band - and a marker for a later reconstruction to confirm; the unit's own
 * header says so too.  Only `DWCi_GetStringLength` (0x80512490) has a body today.
 */
#ifndef MHTRI_DWCI_DWCI_NATNEG_H
#define MHTRI_DWCI_DWCI_NATNEG_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* the unprototyped callback shape the GameSpy interface installs and casts back */
typedef void (*DWCiCallback)();

void DWCi_NatNegCleanup(void);
s32  DWCi_NatNegStartSession(u32 session, s32 flag, DWCiCallback empty, DWCiCallback callback, void* result);
void DWCi_NatNegEndSession(u32 session);
void DWCi_NatNegProcess(void);
void DWCi_NatNegSendPacket(void* data, u32 size, void* header);

/* 0x80512490: lazy string/length accessor (the seam function; body in DWCi_NatNeg.c). */
void DWCi_GetStringLength(void** buf, int* len);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_DWCI_NATNEG_H */
