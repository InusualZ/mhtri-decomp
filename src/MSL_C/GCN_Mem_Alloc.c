/*
 * MSL_C/GCN_Mem_Alloc.c - the system allocation hook of the GameCube/Wii MSL port (`__sys_free`: heap-arena
 *    initialisation and `OSFreeToHeap`).
 *
 * RANGE. .text 0x80458BDC..0x80458C94 (1 functions in the map, 0xB8 B); .rodata 0x805724B8..0x80572528.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name is the string's own (`GCN_Mem_Alloc.c`).
 * EVIDENCE. its only `.rodata` string reads `GCN_Mem_Alloc.c : InitDefaultHeap. No Heap Available`, the original
 *    `__FILE__`-style name, and only `__sys_free` reads it.
 * RESIDUALS. the default-heap setup is inlined into `__sys_free`; the allocator proper starts at the next unit.
 * SHAPES. the first call builds the heap over the free arena and aligns both ends to 32 bytes.
 */
#include "MSL_C/GCN_Mem_Alloc.h"
#include "OS/OSAlloc.h"
#include "OS/OSArena.h"
#include "OS/OSError.h"
#include "OS/s_currentHeap.h"

/* untyped: caller-owned heap block */
void __sys_free(void* block)
{
    void* arena_lo;
    void* arena_hi;

    if (s_currentHeap == -1) {
        OSReport("GCN_Mem_Alloc.c : InitDefaultHeap. No Heap Available\n");
        OSReport("Metrowerks CW runtime library initializing default heap\n");
        arena_lo = OSGetArenaLo();
        arena_hi = OSGetArenaHi();
        arena_lo = OSInitAlloc(arena_lo, arena_hi, 1);
        OSSetArenaLo(arena_lo);
        arena_lo = (void*)(((u32)arena_lo + 0x1F) & ~0x1F);
        arena_hi = (void*)((u32)arena_hi & ~0x1F);
        OSSetCurrentHeap(OSCreateHeap(arena_lo, arena_hi));
        OSSetArenaLo(arena_hi);
    }
    OSFreeToHeap(s_currentHeap, block);
}
