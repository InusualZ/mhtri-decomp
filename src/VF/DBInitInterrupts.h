/*
 * DBInitInterrupts.h - the declaration of `DBInitInterrupts`, owned by `VF/vf.cpp`.
 */
#ifndef VF_DBINITINTERRUPTS_H
#define VF_DBINITINTERRUPTS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x805223D8 - masks and installs the debugger-channel interrupt handlers. */
void DBInitInterrupts(void);

#ifdef __cplusplus
}
#endif

#endif
