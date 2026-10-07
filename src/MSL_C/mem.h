/*
 * MSL_C/mem.h - the MSL memory search routine the stdio layer calls, owned by `MSL_C/mem.c`.
 */
#ifndef MSL_C_MEM_H
#define MSL_C_MEM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8045B690 (0x2C): finds the last occurrence of `ch` in the first `n` bytes of `buf`; returns NULL when absent. */
/* untyped: byte range */
void* __memrchr(const void* buf, int ch, u32 n);

#ifdef __cplusplus
}
#endif

#endif
