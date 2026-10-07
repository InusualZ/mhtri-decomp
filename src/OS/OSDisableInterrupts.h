/* OS/OSDisableInterrupts.h - the declaration of `OSDisableInterrupts`, which `OS/OSInterrupt.c` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_OS_OSDISABLEINTERRUPTS_H
#define MHTRI_OS_OSDISABLEINTERRUPTS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
u32 OSDisableInterrupts(void);
#ifdef __cplusplus
}
#endif

#endif
