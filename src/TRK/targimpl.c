/*
 * TRK/targimpl.c - the MetroTRK target implementation: MSR access, memory/register access, the exception and
 *    interrupt handlers, step control, the PPC special-register access, the input-pending setter and the
 *    address/thread stubs.
 *
 * RANGE. .text 0x8046BBEC..0x8046D420 (36 functions in the map, 0x1834 B); .rodata 0x805734A8..0x80573530; .data
 *    0x8060F810..0x8060F820; .bss 0x806F6F38..0x806F74E0; .sbss 0x80794E78..0x80794E88.
 * FLAGS. the `OS` lib's `cflags_os` (the group of the unit it was cut from; unmeasured until bodies exist).
 * NAMES. file name GUESS (MetroTRK `targimpl`).
 * EVIDENCE. `.data` 0x8060F810 (`gTRKExceptionStatus`), `.bss` 0x806F6F38.. (restore flags, step state, save
 *    state, `gTRKState`, `gTRKCPUState`), `.sbss` 0x80794E78 / 0x80794E80 and `.rodata` 0x805734A8..0x80573530
 *    are read by functions on both sides of every internal candidate boundary, so the run is one TU unless a
 *    boundary moves the globals.
 * RESIDUALS. COARSE: this is the biggest TRK unit; the PPC-access group (0x8046CF6C..) and the glue (0x8046D368..)
 *    may be separate files but the shared `gTRKState` forbids cutting before the `.bss` order is understood.
 * SHAPES. none yet: no body is written, so nothing is measured; the map rows of the range are the work list
 *    (`ledger.py unit TRK/targimpl.c`).
 */
