/* NAND/OSRestoreInterrupts.h - the declaration of `OSRestoreInterrupts`, which `NAND/nand.c` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_NAND_OSRESTOREINTERRUPTS_H
#define MHTRI_NAND_OSRESTOREINTERRUPTS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void OSRestoreInterrupts(u32 level);
#ifdef __cplusplus
}
#endif

#endif
