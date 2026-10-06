/* OS/PPCHalt.h - `PPCHalt`, which `OS/PPCArch.c` owns (docs/plan.md 6.5 rule 2, leaf header). */
#ifndef MHTRI_OS_PPCHALT_H
#define MHTRI_OS_PPCHALT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80477160 - stops the processor in a sync loop. */
void PPCHalt(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_OS_PPCHALT_H */
