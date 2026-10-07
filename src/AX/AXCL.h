/*
 * AX/AXCL.h - declarations of the symbols owned by `AX/AXCL.c` that other units call.
 */
#ifndef AX_AXCL_H
#define AX_AXCL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8046FD80 - the AX output mode word: 0 stereo, 1 surround, 2 Dolby Pro Logic II. */
u32 AXGetMode(void);

#ifdef __cplusplus
}
#endif

#endif
