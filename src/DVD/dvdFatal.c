/*
 * DVD/dvdFatal.c - the SDK DVD fatal-error screen (`__DVDShowFatalMessage`, auto-fatal setter).
 *
 * RANGE. `.text` 0x804ABDD0-0x804ABF40; `.rodata` 0x805739A8-0x805739E0; `.data` 0x80618C40-0x806195A0; `.sdata`
 *   0x80793DF8-0x80793E00; `.sbss` 0x80795148-0x80795150; `.sdata2` 0x8079D0D8-0x8079D0E0 (4 functions / 0x154
 *   B).
 *   - `.sdata` 0x80793DF8 holds the two message pointers (0x806192BC / 0x80619360) that `__DVDShowFatalMessage`
 *     reads; `.data` 0x80618C40..0x806195A0 is the Shift-JIS message table; `.rodata` 0x805739A8/0x805739C4 and
 *     `.sdata2` 0x8079D0D8 are its constants
 *   - `FatalFunc` (.sbss 0x80795148) is read by the setter and the two accessors that follow
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * NAMES. the unit name is a GUESS (named for what the messages do).
 * RESIDUALS. No body is written (4 functions), the largest `__DVDShowFatalMessage` at 0x804ABDD0 (0xCC B); `python
 *   tools/units/sweepcomments.py --unit DVD/dvdFatal.c` lists them.
 */
