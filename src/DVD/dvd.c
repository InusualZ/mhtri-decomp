/*
 * DVD/dvd.c - the SDK DVD command layer (state machine, error recovery, ReadAbs/Inquiry/Cancel/Reset).
 *
 * RANGE. `.text` 0x804A6410-0x804AB040; `.data` 0x80618800-0x80618C10; `.bss` 0x80741A40-0x807466F0; `.sdata`
 *   0x80793DE0-0x80793DF8; `.sbss` 0x807950A0-0x80795138 (53 functions / 0x4AB8 B).
 *   - the DOL string `dvd.c` (.sdata 0x80793DEC) is the `__FILE__` of its asserts; the version string `<< RVL_SDK -
 *     DVD ... >>` at `.data` 0x80618800 is registered by `DVDInit` through `__DVDVersion` (.sdata 0x80793DE0)
 *   - `.data` jump tables for the state functions, `.bss` 0x80741A40..0x807466F0 (ticket/TMD buffers, command blocks,
 *     the cover alarm), `.sbss` 0x807950A0..0x80795138 (pause/fatal/motor state)
 *   - the right edge is the queue helpers (`__DVDClearWaitingQueue`), whose only data is `WaitingQueue`
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. `__ErrorInfo` (.bss 0x80746880) is read here but sits in `DVD/dvderror.c`'s run. No body is written (53
 *   functions), the largest `fn_804A9A90` at 0x804A9A90 (0x9E0 B); `python tools/units/sweepcomments.py
 *   --unit DVD/dvd.c` lists them.
 */
