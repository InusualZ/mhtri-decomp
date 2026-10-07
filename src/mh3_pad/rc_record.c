/*
 * mh3_pad/rc_record.c - the game-side GC pad poller that fills the RcRecord table.
 *
 * RANGE. `.text` 0x804A4BC0-0x804A4F20; `.data` 0x806184F8-0x80618508; `.bss` 0x80741998-0x80741A40; `.sbss`
 *   0x80795048-0x80795050 (3 functions / 0x344 B).
 *   - the code is game code, not an SDK library: it calls the pad reader and `PADClamp` for four channels and fills
 *     the 0x1E-byte records at `.bss` 0x807419C8 that `mh3_pad.cpp` documents as "in the DVD library's .bss"
 *   - `.data` 0x806184F8 (four channel mask words), `.bss` 0x80741998 (the PADStatus copy) and `.sbss` 0x80795048 are
 *     read only here
 *   - the left edge (0x804A4BC0) follows `DBPrintf`; the right edge is the DSP library's first function
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * NAMES. the unit name is a GUESS (the record table it fills is `mh3_pad.cpp`'s `RcRecord`).
 * RESIDUALS. No body is written (3 functions), the largest `fn_804A4BC0` at 0x804A4BC0 (0x1A4 B); `python
 *   tools/units/sweepcomments.py --unit mh3_pad/rc_record.c` lists them.
 */
