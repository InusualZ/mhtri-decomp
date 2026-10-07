/*
 * AX/AXFXDelay.h - the delay effects' line-free entry points (`AX/AXFXDelay.cpp`, 0x80476F10..0x80477090).
 */
#ifndef AX_AXFXDELAY_H
#define AX_AXFXDELAY_H

#include "types.h"

/* The leading fields of the two delay bodies: the delay-line pointers and the flag word; the full bodies are
 * larger (sizes are approximations of the part read here). */
typedef struct AXFXDelay3 {
    /* +0x00 */ void* line[3];
    /* +0x0C */ u8 pad_0x0C[0x70];
    /* +0x7C */ u32 flags;
} AXFXDelay3; /* size: 0x80 (approximation) */

typedef struct AXFXDelay4 {
    /* +0x00 */ void* line[4];
    /* +0x10 */ u8 pad_0x10[0x80];
    /* +0x90 */ u32 flags;
} AXFXDelay4; /* size: 0x94 (approximation) */

#ifdef __cplusplus
extern "C" {
#endif

BOOL AXFXDelay3Shutdown(AXFXDelay3* effect);
BOOL AXFXDelay4Shutdown(AXFXDelay4* effect);
void AXFXDelay3Free(AXFXDelay3* effect);
void AXFXDelay4Free(AXFXDelay4* effect);

#ifdef __cplusplus
}
#endif

#endif
