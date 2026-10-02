/* NAND/OSDisableInterrupts.h - the declaration of `OSDisableInterrupts`, which `NAND/nand.c` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_NAND_OSDISABLEINTERRUPTS_H
#define MHTRI_NAND_OSDISABLEINTERRUPTS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
u32 OSDisableInterrupts(void);
#ifdef __cplusplus
}
#endif

#endif
