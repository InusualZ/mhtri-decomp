/* OS/OSVReport.h - `OSVReport`, which `OS/OSError.c` owns (docs/plan.md 6.5 rule 2, leaf header): 0x804CD6A0, an empty
 *   stub in this build. */
#ifndef MHTRI_OS_OSVREPORT_H
#define MHTRI_OS_OSVREPORT_H

#include "types.h"

struct __va_list_struct;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CD6A0 - prints `fmt` with an argument list (an empty stub in the release build). */
void OSVReport(const char* fmt, struct __va_list_struct* vlist);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_OS_OSVREPORT_H */
