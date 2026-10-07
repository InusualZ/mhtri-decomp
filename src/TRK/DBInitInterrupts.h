/*
 * DBInitInterrupts.h - the declaration of `DBInitInterrupts`, owned by `TRK/exi2_comm.c`.
 */
#ifndef TRK_DBINITINTERRUPTS_H
#define TRK_DBINITINTERRUPTS_H

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
