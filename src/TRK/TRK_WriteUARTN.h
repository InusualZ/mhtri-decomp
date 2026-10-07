/*
 * TRK/TRK_WriteUARTN.h - the declaration of `TRK_WriteUARTN`, owned by `TRK/dolphin_trk.c`.
 */
#ifndef TRK_TRK_WRITEUARTN_H
#define TRK_TRK_WRITEUARTN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8046933C - writes `size` bytes from `buffer` to the debugger UART; nonzero on error. */
/* untyped: byte range */
s32 TRK_WriteUARTN(const void* buffer, u32 size);

#ifdef __cplusplus
}
#endif

#endif
