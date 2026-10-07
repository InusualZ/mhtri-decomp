/*
 * DVD/dvd_broadway.c - the SDK DVD low-level driver for the Broadway IOS `/dev/di` (DVDLow*).
 *
 * RANGE. `.text` 0x804AC1D0-0x804AE880; `.data` 0x806195A0-0x8061A4A8; `.bss` 0x807469A0-0x80746B60; `.sdata`
 *   0x80793E08-0x80793E18; `.sbss` 0x80795158-0x80795180 (32 functions / 0x25FC B).
 *   - the string `/dev/di` (.sdata 0x80793E10) and the transaction-callback string (.data 0x806195A0) open the run;
 *     `.data` 0x80619600 is the merged pool of the driver's error messages
 *   - its `.bss` 0x807469A0..0x80746B60 and `.sbss` 0x80795158..0x80795180 are read only here; the right edge is the
 *     ENC library (version string registered at 0x804AE880)
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * NAMES. the unit name is a GUESS (the driver's file name in the SDK scheme).
 * RESIDUALS. No body is written (32 functions), the largest `fn_804ACE90` at 0x804ACE90 (0x298 B); `python
 *   tools/units/sweepcomments.py --unit DVD/dvd_broadway.c` lists them.
 */
