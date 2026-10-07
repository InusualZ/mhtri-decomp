/*
 * MIX/mix.c - the MIX audio mixer: pan/volume coefficient table lookups, the channel state and the two 0x1.6 KB
 *    update bodies.
 * RANGE. .text 0x804C25E0-0x804C5C10 (17 functions); .data 0x8061AE00-0x8061B9A0; .bss 0x80748BB8-0x8074CF40; .sbss
 *    0x807952A0-0x807952B8.  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: `__MIXSetPan` is the
 *    dump's name for the first function; every function in the range reads the 0xBA0-byte coefficient table
 *    (.data 0x8061AE00), the 0x2A00/0x1988-byte channel arrays (.bss 0x80748BB8, 0x8074B5B8) or the mix state
 *    (.sbss 0x807952A0..0x807952B4), and it calls AX (0x8046E4E0, 0x80471950); nothing outside the range reads
 *    them.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; the bodies written here are measured with
 *    them.
 * NAMES. `__MIXSetPan` is the dump's name; the file name `mix.c` is a GUESS; every other function keeps its map stem.
 * RESIDUALS. `fn_804C2840` (0x16C4 B) and `fn_804C40D0` (0x1694 B), `__MIXSetPan` and the channel functions are unwritten; the
 *    .data coefficient table 0x8061AE00, the .bss channel arrays 0x80748BB8/0x8074B5B8 and the .sbss mix state 0x807952A0..0x807952B8
 *    are claimed but not emitted (declared `extern`, never defined), so flipping the unit drops 2976 + 17288 + 24 bytes.
 * SHAPES. none beyond what the bodies show.
 */

#include "types.h"

extern u32 lbl_807952A0;      /* AX mix state (.sbss) */
extern u32 lbl_807952A4;      /* AX mix state (.sbss) */
extern u32 lbl_807952B0;      /* AX mix state (.sbss) */
extern int lbl_807952AC;      /* AX mixing mode (.sbss) */
extern u8 lbl_8061AE00[];     /* AX pan/mix coefficient table (.data, 0xBA0 B) */

/* Clear the mixing state. */
void fn_804C2800(void)
{
    lbl_807952A0 = 0;
    lbl_807952B0 = 0;
    lbl_807952A4 = 0;
}

/* Set the mixing mode. */
void fn_804C2820(int mode)
{
    lbl_807952AC = mode;
}

/* Read the mixing mode. */
int fn_804C2830(void)
{
    return lbl_807952AC;
}

/* Clamp an AX pan/volume index into the coefficient table's u16 row. */
u16 fn_804C26A0(s32 x)
{
    if (x <= -904)
        return 0;
    if (x >= 60)
        return 0xFF64;
    return ((u16*)lbl_8061AE00)[x + 904];
}
