/*
 * EXI/ProbeBarnacle.h - the barnacle probe's entry points (the symbols `EXI/ProbeBarnacle.c` owns).
 */
#ifndef EXI_PROBEBARNACLE_H
#define EXI_PROBEBARNACLE_H

#include "types.h"
#include "EXI/EXIBios.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL ProbeBarnacle(EXIChannel chan, u32 dev, u32* id);
void __OSEnableBarnacle(EXIChannel chan, u32 dev);
/* untyped: the register payload, 1, 2 or 4 bytes */
BOOL EXIWriteReg(EXIChannel chan, u32 dev, u32 cmd, const void* buf, s32 len);

#ifdef __cplusplus
}
#endif

#endif
