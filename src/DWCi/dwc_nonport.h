/*
 * DWCi/dwc_nonport.h - the declarations `src/DWCi/dwc_nonport.c` owns that other units use (rule 2).
 */
#ifndef MHTRI_DWCI_DWC_NONPORT_H
#define MHTRI_DWCI_DWC_NONPORT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x807957D0 - the one-shot flag and value of the console friend code. */
extern s32 DWCi_friendCodeReady;
extern u64 DWCi_consoleFriendCode;

/* 0x8062FFD0 / 0x8062FFF4 - the two lines the friend-code reporter prints. */
extern char DWCi_reportFriendCodeFormat[];
extern char DWCi_reportFriendCodeFailedFormat[];

/* 0x80507C40 - copies `len` bytes by the widest unit both pointers allow. */
void DWCi_Np_CPUCopyFast(u8* dst, const u8* src, u32 len);
/* 0x80508630 - the console friend code, fetched once. */
u64 DWCi_GetConsoleFriendCode(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_DWC_NONPORT_H */
