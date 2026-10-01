/*
 * SO/soi.h - declarations of the symbols owned by `SO/soi.cpp` that other units call or read.
 */
#ifndef SO_SOI_H
#define SO_SOI_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* untyped: opaque band object, typed by the callers' views */
u32 fn_80526F00(void* list, u32 arg1, u32 arg2);

/* helper entry points in the neighbouring (unsplit) subsystem TUs. */
/* untyped: opaque band object, typed by the callers' views */
u32 fn_80529430(u32 index, u32 value, void* callback, u32 arg);

void fn_80529B50(u8 index, u32 value);

#ifdef __cplusplus
}
#endif

#endif
