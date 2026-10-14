/*
 * DSP/dsp_debug.h - the declaration of the symbol owned by `DSP/dsp_debug.c`.
 */
#ifndef DSP_DSP_DEBUG_H
#define DSP_DSP_DEBUG_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804A51C0 (0x50): the DSP library's trace printf; prints nothing in a release build. */
void __DSP_debug_printf(const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
