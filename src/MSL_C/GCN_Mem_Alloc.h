/*
 * MSL_C/GCN_Mem_Alloc.h - the system allocation hook of the GameCube/Wii MSL port, owned by `MSL_C/GCN_Mem_Alloc.c`.
 */
#ifndef MSL_C_GCN_MEM_ALLOC_H
#define MSL_C_GCN_MEM_ALLOC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80458BDC (0xB8): creates the default heap on first use, then returns `block` to the current heap. */
/* untyped: caller-owned heap block */
void __sys_free(void* block);

#ifdef __cplusplus
}
#endif

#endif
