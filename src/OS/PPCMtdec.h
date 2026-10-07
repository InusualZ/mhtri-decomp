/* OS/PPCMtdec.h - `PPCMtdec`, which `OS/PPCArch.c` owns (docs/plan.md 6.5 rule 2, leaf header). */
#ifndef MHTRI_OS_PPCMTDEC_H
#define MHTRI_OS_PPCMTDEC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80477140 - writes the decrementer register. */
void PPCMtdec(u32 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_OS_PPCMTDEC_H */
