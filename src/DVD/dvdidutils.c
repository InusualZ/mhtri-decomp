/*
 * DVD/dvdidutils.c - the SDK disk-ID comparison (`DVDCompareDiskID`).
 *
 * RANGE. `.text` 0x804ABCE0-0x804ABDD0 (1 functions / 0xF0 B).
 *   - one function with no data of its own; both neighbours read disjoint data, so the seam is clean on each side.
 *     The file attribution has no data evidence (roster only)
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * NAMES. the unit name is a GUESS (single function, no string).
 * RESIDUALS. no data evidence ties this function to a library file; it could belong to `dvderror.c` or `dvdFatal.c`.
 *   No body is written (1 functions), the largest `DVDCompareDiskID` at 0x804ABCE0 (0xF0 B); `python
 *   tools/units/sweepcomments.py --unit DVD/dvdidutils.c` lists them.
 */
