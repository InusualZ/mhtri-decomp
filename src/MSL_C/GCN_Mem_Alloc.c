/*
 * MSL_C/GCN_Mem_Alloc.c - the system allocation hook of the GameCube/Wii MSL port (`__sys_free`: heap-arena
 *    initialisation and `OSFreeToHeap`).
 *
 * RANGE. .text 0x80458BDC..0x80458C94 (1 functions in the map, 0xB8 B); .rodata 0x805724B8..0x80572528.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (`-str reuse,pool,readonly` pools both strings into one
 *    `@stringBase0`, the second one unaligned at +0x36).
 * NAMES. file name is the string's own (`GCN_Mem_Alloc.c`); `InitDefaultHeap` is the name the first string gives
 *    the helper; `@stringBase0` (0x805724B8, 0x70 B) is the compiler's own name for the pooled string block.
 * EVIDENCE. its only `.rodata` string reads `GCN_Mem_Alloc.c : InitDefaultHeap. No Heap Available`, the original
 *    `__FILE__`-style name, and only `__sys_free` reads it.
 * RESIDUALS. none: the row and both sections match the target (`.rodata` ends 1 B short of the claim, alignment fill).
 * SHAPES. the default-heap setup is a `static inline` helper that `-inline auto` folds into `__sys_free`; written
 *    inline in the caller its two locals colour r31/r30 whichever way they are declared, as the helper's locals they
 *    colour r30/r31 as retail. The first call builds the heap over the free arena and aligns both ends to 32 bytes.
 */
#include "MSL_C/GCN_Mem_Alloc.h"
#include "OS/OSAlloc.h"
#include "OS/OSArena.h"
#include "OS/OSError.h"
#include "OS/s_currentHeap.h"

/* Builds the default heap over the whole free arena, 32-byte aligned at both ends, and makes it current. */
static inline void InitDefaultHeap(void)
{
    void* arena_lo;
    void* arena_hi;

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

/* untyped: caller-owned heap block */
void __sys_free(void* block)
{
    if (s_currentHeap == -1) {
        InitDefaultHeap();
    }
    OSFreeToHeap(s_currentHeap, block);
}
