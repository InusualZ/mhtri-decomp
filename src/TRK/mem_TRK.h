/*
 * TRK/mem_TRK.h - MetroTRK's own memory routines, owned by `TRK/mem_TRK.c`.
 */
#ifndef TRK_MEM_TRK_H
#define TRK_MEM_TRK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804689C8 (0x13C): copies `n` bytes; returns `dst`. */
/* untyped: memcpy-shaped byte range */
void* TRK_memcpy(void* dst, const void* src, u32 n);

/* 0x80468B04 (0x128): fills `n` bytes with the low byte of `val`; returns `dst`. */
/* untyped: memset-shaped byte range */
void* TRK_memset(void* dst, int val, u32 n);

#ifdef __cplusplus
}
#endif

#endif
