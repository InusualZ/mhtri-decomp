/*
 * include/DWCi/DWCi_NatNeg.h - the DWCi NATNEG / transport-tail unit (`src/DWCi/DWCi_NatNeg.c`,
 * `.text` 0x80512490..0x805145B8).
 *
 * Rule 2: this unit owns these symbols, so the GameSpy interface (`src/Network/fn_8041A87C.cpp`)
 * reaches them through `include/unsplit/Network.h`, which includes this header.
 *
 * The `fn_` names are the map's placeholders for functions the unit owns but has not reconstructed
 * yet - rule 7's reference half (the unit header records the deferral).  They are renamed when their
 * bodies land; only `DWCi_GetStringLength` (0x80512490) has a body today.
 */
#ifndef MHTRI_DWCI_DWCI_NATNEG_H
#define MHTRI_DWCI_DWCI_NATNEG_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* the unprototyped callback shape the GameSpy interface installs and casts back */
typedef void (*DWCiCallback)();

void fn_80512C50(void);
s32  fn_805132C0(u32 session, s32 flag, DWCiCallback empty, DWCiCallback callback, void* result);
void fn_805135E0(u32 session);
void fn_80513BB0(void);
void fn_80514400(void* data, u32 size, void* header);

/* 0x80512490: lazy string/length accessor (the seam function; body in DWCi_NatNeg.c). */
void DWCi_GetStringLength(void** buf, int* len);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_DWCI_NATNEG_H */
