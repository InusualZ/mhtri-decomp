/* Declarations owned by `src/DWCi/dwc_error.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_DWCI_DWC_ERROR_H
#define MHTRI_DWCI_DWC_ERROR_H

#include "types.h"
#include "DWCi/natneg.h"            /* the NATNEG half's own data (rule 2: the owner declares it) */
#include "DWCi/dwc_auth_interface.h"    /* the Np unit's own data (rule 2) */
#include "DWCi/dwc_init.h"          /* DWC_Init and the memory functions: their owner's header */
#include "DWCi/dwc_report.h"        /* DWC_Printf: its owner's header */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x807957B0..0x807957B8 - the last error and its code. */
extern s32 stDwcLastError;
extern s32 stDwcErrorCode;

s32 DWC_GetLastErrorEx(s32* code, s32* type);
void DWC_ClearError(void);
void DWCi_SetError(s32 error, s32 code);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_DWC_ERROR_H */
