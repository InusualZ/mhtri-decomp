/*
 * AX/AXFXChorus.h - the chorus effects' line-free entry points (`AX/AXFXChorus.cpp`, 0x80476DF0..0x80476F10).
 */
#ifndef AX_AXFXCHORUS_H
#define AX_AXFXCHORUS_H

#include "types.h"

/* The leading fields of the two chorus bodies: the delay-line pointers and the flag word; the full bodies are
 * larger (sizes are approximations of the part read here). */
typedef struct AXFXChorus3 {
    /* +0x00 */ void* line[3];
    /* +0x0C */ u8 pad_0x0C[0x30];
    /* +0x3C */ u32 flags;
} AXFXChorus3; /* size: 0x40 (approximation) */

typedef struct AXFXChorus4 {
    /* +0x00 */ void* line[4];
    /* +0x10 */ u8 pad_0x10[0x40];
    /* +0x50 */ u32 flags;
} AXFXChorus4; /* size: 0x54 (approximation) */

#ifdef __cplusplus
extern "C" {
#endif

void AXFXChorus3Shutdown(AXFXChorus3* effect);
void AXFXChorus4Shutdown(AXFXChorus4* effect);

#ifdef __cplusplus
}
#endif

#endif
