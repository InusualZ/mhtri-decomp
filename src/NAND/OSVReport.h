/* NAND/OSVReport.h - the OS report and scheduler entry points `NAND/nand.c` owns (docs/plan.md 6.5 rule 2, leaf
 *   header): `OSVReport` (0x804CD6A0, an empty stub in this build), `OSDisableScheduler`, `OSYieldThread`. */
#ifndef MHTRI_NAND_OSVREPORT_H
#define MHTRI_NAND_OSVREPORT_H

#include "types.h"

struct __va_list_struct;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CD6A0 - prints `fmt` with an argument list (an empty stub in the release build). */
void OSVReport(const char* fmt, struct __va_list_struct* vlist);

/* 0x804D39B0 - stops thread switching; returns the previous suspend count. */
s32 OSDisableScheduler(void);

/* 0x804D3F30 - yields the processor to another ready thread. */
void OSYieldThread(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NAND_OSVREPORT_H */
