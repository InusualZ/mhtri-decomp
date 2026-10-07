/*
 * RVLGX/GXFifo.c - the SDK GX FIFO management (CP/GP link check, FIFO bases, breakpoint callback).
 *
 * RANGE. `.text` 0x804B4460-0x804B4E40; `.data` 0x8061A7A0-0x8061A7D0; `.bss` 0x807472A0-0x80747300; `.sbss`
 *   0x807951F0-0x80795210 (9 functions / 0x9BC B).
 *   - `.data` 0x8061A7A0 / 0x8061A7B8 are the `CPUFifo:` / `GPFifo:` debug strings read by `CPGPLinkCheck`; `.bss`
 *     0x807472A0 / 0x807472C4 and `.sbss` 0x807951F0..0x80795210 are read by the setters and the clean-up function
 *   - the run starts at the CP interrupt handler (0x804B4460: its statics 0x807951F4/0x807951FC/0x80795200/0x80795204
 *     continue the FIFO run) and ends at `GXSetVtxDesc`
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. `.data` (0x30), `.bss` (0x60) and `.sbss` (0x20) are claimed and not emitted (their readers are
 *   unwritten). `__GXIsGPFifoReady` written (byte-identical). The other 8 functions are unwritten, the
 *   largest `GXSetGPFifo` at 0x804B4930 (0x290 B); `python tools/units/sweepcomments.py --unit
 *   RVLGX/GXFifo.c` lists them.
 */

#pragma function_align 16

#include "types.h"

/* The GP FIFO ready flag (`.sbss` 0x807951F1): this unit's own byte, declared and never defined. */
extern u8 lbl_807951F1;

/* Returns whether the GP FIFO is ready (the byte flag the FIFO setup leaves set). */
BOOL __GXIsGPFifoReady(void)
{
    return lbl_807951F1;
}
