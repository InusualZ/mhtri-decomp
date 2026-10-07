/*
 * AX/AXFXHooks.h - the allocator hooks every AXFX effect takes its delay lines from (`AX/AXFXHooks.cpp`,
 * 0x80477090..0x804770E0).
 */
#ifndef AX_AXFXHOOKS_H
#define AX_AXFXHOOKS_H

#include "types.h"

/* untyped: raw heap memory */
typedef void* (*AXFXAllocFunc)(u32 size);
/* untyped: raw heap memory */
typedef void (*AXFXFreeFunc)(void* block);

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80793D20 / 0x80793D24 - the hooks installed by AXFXSetHooks. */
extern AXFXAllocFunc AXFXAlloc;
extern AXFXFreeFunc AXFXFree;

/* untyped: raw heap memory */
void* AXFXDefaultAlloc(u32 size);
/* untyped: raw heap memory */
void AXFXDefaultFree(void* block);
void AXFXSetHooks(AXFXAllocFunc alloc, AXFXFreeFunc free);
void AXFXGetHooks(AXFXAllocFunc* alloc, AXFXFreeFunc* free);

#ifdef __cplusplus
}
#endif

#endif
