/*
 * AX/AXFXHooks.cpp - the AXFX allocation hooks: the two hook slots, their default heap thunks and the
 *    installer / reader pair.
 *
 * RANGE. .text 0x80477090-0x804770E0 (4 functions, 0x44 B); .sdata 0x80793D20-0x80793D28 (AXFXAlloc, AXFXFree).
 *    Left edge: the end of `AX/AXFXDelay.cpp` (UNPROVEN, no pooled data on that side); right edge: the start of
 *    `OS/PPCArch.c`.  The two slots are read by the reverb, chorus and delay units through `AX/AXFXHooks.h`.
 * FLAGS. the `OS` lib group with the 16-byte function alignment restored below.
 * NAMES. AXFXSetHooks and AXFXGetHooks are the map's names; GUESS: AXFXDefaultAlloc/AXFXDefaultFree.
 * RESIDUALS. none measured.
 */

#pragma function_align 16

#include "types.h"
#include "AX/AXFXHooks.h"
#include "OS/OSAlloc.h"
#include "OS/s_currentHeap.h"

extern "C" {

AXFXAllocFunc AXFXAlloc = AXFXDefaultAlloc;
AXFXFreeFunc AXFXFree = AXFXDefaultFree;

/* The default allocation hook: takes the block from the current heap. */
/* untyped: raw heap memory */
void* AXFXDefaultAlloc(u32 size)
{
    return OSAllocFromHeap(s_currentHeap, size);
}

/* The default free hook: returns the block to the current heap. */
/* untyped: raw heap memory */
void AXFXDefaultFree(void* block)
{
    OSFreeToHeap(s_currentHeap, block);
}

/* Installs the allocation and free hooks. */
void AXFXSetHooks(AXFXAllocFunc alloc, AXFXFreeFunc free)
{
    AXFXAlloc = alloc;
    AXFXFree = free;
}

/* Reads back the installed hooks. */
void AXFXGetHooks(AXFXAllocFunc* alloc, AXFXFreeFunc* free)
{
    *alloc = AXFXAlloc;
    *free = AXFXFree;
}

} /* extern "C" */
