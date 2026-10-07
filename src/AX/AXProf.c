/*
 * AX/AXProf.c - the AX profiling: the current profile accessor.
 *
 * RANGE. .text 0x80471960-0x804719A0 (1 functions, 0x40 B); .sbss 0x80794FF8-0x80795008.  Cut from the old ARC/AX
 *    block between `AX/AXVPB.c` (0x80471960) and `DSPADPCM/dspadpcm.c` (0x804719A0).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `__AXGetCurrentProfile` is the map's name; the file name `AXProf.c` is a GUESS; `__AXProfileActive`,
 *    `__AXProfileNext`, `__AXProfileCount` and `__AXProfileBuffer` are GUESSes named from how the accessor uses each word.
 * EVIDENCE. `.sbss` 0x80794FF8..0x80795008 (four words) is read by `__AXGetCurrentProfile` alone; it is the last
 *    `.sbss` of the old block.
 * RESIDUALS. __AXGetCurrentProfile 0x80471960: the ring index and the buffer pointer take swapped registers (r5/r6), 5 operand-only differences.
 * SHAPES. plain C: a ring of 56-byte profile records advanced modulo the record count.
 */

#include "types.h"

#include "AX/AXProf.h"

AXProfile* __AXProfileBuffer;
u32 __AXProfileCount;
u32 __AXProfileNext;
u32 __AXProfileActive;

/* 0x80471960 (0x40): returns the next profile record of the ring, or NULL while profiling is off. */
AXProfile* __AXGetCurrentProfile(void)
{
    AXProfile* profile;

    if (__AXProfileActive != 0) {
        profile = &__AXProfileBuffer[__AXProfileNext];
        __AXProfileNext = (__AXProfileNext + 1) % __AXProfileCount;
        return profile;
    }
    return NULL;
}
