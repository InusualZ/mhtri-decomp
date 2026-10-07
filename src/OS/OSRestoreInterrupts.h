/* OS/OSRestoreInterrupts.h - the declaration of `OSRestoreInterrupts`, which `OS/OSInterrupt.c` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_OS_OSRESTOREINTERRUPTS_H
#define MHTRI_OS_OSRESTOREINTERRUPTS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void OSRestoreInterrupts(u32 level);
#ifdef __cplusplus
}
#endif

#endif
