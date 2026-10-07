/*
 * DBWrite.h - the declaration of `DBWrite`, owned by `TRK/exi2_comm.c`.
 */
#ifndef TRK_DBWRITE_H
#define TRK_DBWRITE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80522550 - writes up to `len` bytes to the debugger channel and returns the count written. */
s32 DBWrite(const u8* src, s32 len);

#ifdef __cplusplus
}
#endif

#endif
