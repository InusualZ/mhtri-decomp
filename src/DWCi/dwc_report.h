/* Declarations owned by `src/DWCi/dwc_report.cpp` (docs/plan.md 6.5 rule 2): the DWC report printer. */
#ifndef MHTRI_DWCI_DWC_REPORT_H
#define MHTRI_DWCI_DWC_REPORT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x807957C8 - the report category mask `DWC_Printf` tests. */
extern u32 stDwcReportLevel;

/* 0x805078F0 - the category-filtered `printf`-style report: prints the category prefix and the message when
 * the category bit is set in `stDwcReportLevel`. */
void DWC_Printf(u32 level, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_DWC_REPORT_H */
