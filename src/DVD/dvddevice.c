/*
 * DVD/dvddevice.c - the SDK DVD device check (`__DVDCheckDevice` and its low-level callback).
 *
 * RANGE. `.text` 0x804ABF40-0x804AC1D0; `.rodata` 0x805739E0-0x80573A00; `.bss` 0x80746980-0x807469A0; `.sdata`
 *   0x80793E00-0x80793E08; `.sbss` 0x80795150-0x80795158; `.sdata2` 0x8079D0E0-0x8079D0E8 (2 functions / 0x28C
 *   B).
 *   - reads `lowDone` (.sdata 0x80793E00), `lowIntType` (.sbss 0x80795150), `CheckBuffer` (.bss 0x80746980),
 *     `__DVDDeviceErrorMessage` (.rodata 0x805739E0) and `.sdata2` 0x8079D0E0: a run of data between `dvdFatal` and
 *     the low-level driver that no other unit reads
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * NAMES. the unit name is a GUESS (named for `__DVDCheckDevice`).
 * RESIDUALS. No body is written (2 functions), the largest `__DVDCheckDevice` at 0x804ABF50 (0x27C B); `python
 *   tools/units/sweepcomments.py --unit DVD/dvddevice.c` lists them.
 */
