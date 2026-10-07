/*
 * DVD/dvdfs.c - the SDK DVD file-system layer (path to entry number, open, read-async).
 *
 * RANGE. `.text` 0x804A5AB0-0x804A6410; `.data` 0x806186C8-0x80618800; `.sdata` 0x80793DD0-0x80793DE0; `.sbss`
 *   0x80795070-0x807950A0 (8 functions / 0x928 B).
 *   - the DOL string `dvdfs.c` (.sdata 0x80793DD8) is the `__FILE__` of the asserts in `DVDConvertPathToEntrynum` and
 *     `DVDReadAsyncPrio`; the right edge is the first function that reads the `dvd.c` string (`StampCommand`)
 *   - `.data` 0x806186C8..0x80618800 holds this file's three message strings; `.sbss` 0x80795070..0x807950A0 holds
 *     the FST pointers and the thread queue `__DVDThreadQueue` (defined here: the ordering of the neighbouring
 *     statics demands it)
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. `__DVDThreadQueue` is read by `DVDInit` and two `dvd.c` functions as an extern. No body is written (8
 *   functions), the largest `DVDConvertPathToEntrynum` at 0x804A5AE0 (0x308 B); `python
 *   tools/units/sweepcomments.py --unit DVD/dvdfs.c` lists them.
 */
