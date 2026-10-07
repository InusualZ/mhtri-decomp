/*
 * Runtime.PPCEABI.H/TRK_interrupt_vectors.h - the declaration of `gTRKInterruptVectorTable`, owned by
 *    `Runtime.PPCEABI.H/TRK_interrupt_vectors.c`.
 */
#ifndef RUNTIME_TRK_INTERRUPT_VECTORS_H
#define RUNTIME_TRK_INTERRUPT_VECTORS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80004380 (0x194) - the exception vector code the debugger copies over the low-memory vectors. */
extern u32 gTRKInterruptVectorTable[101];

#ifdef __cplusplus
}
#endif

#endif
