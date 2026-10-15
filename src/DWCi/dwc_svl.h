/*
 * DWCi/dwc_svl.h - the declarations `src/DWCi/dwc_svl.c` owns (docs/plan.md 6.5 rule 2): the DWC service-locator
 *   token request the game's network mediator drives.  `DWCSvlResult` comes from `DWCi/dwc_auth_interface.h`.
 */
#ifndef MHTRI_DWCI_DWC_SVL_H
#define MHTRI_DWCI_DWC_SVL_H

#include "types.h"
#include "DWCi/dwc_auth_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80795800 - the result record the service-locator request fills. GUESS on the name. */
extern DWCSvlResult* DWCi_svlResult;

s32 DWC_SVLBegin(void);
void DWC_SVLEnd(void);
s32 DWC_SVLGetTokenAsync(const char* svl, DWCSvlResult* result);
s32 DWC_SVLProcess(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_DWC_SVL_H */
