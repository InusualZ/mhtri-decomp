/*
 * DBRead.h - the declaration of `DBRead`, owned by `VF/vf.cpp`.
 */
#ifndef VF_DBREAD_H
#define VF_DBREAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x805224D0 - reads `len` bytes from the debugger channel; nonzero on error. */
s32 DBRead(u8* dst, s32 len);

#ifdef __cplusplus
}
#endif

#endif
