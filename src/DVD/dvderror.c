/*
 * DVD/dvderror.c - the SDK DVD error log (NAND-backed error record, `__DVDStoreErrorCode`).
 *
 * RANGE. `.text` 0x804AB2C0-0x804ABCE0; `.data` 0x80618C10-0x80618C40; `.bss` 0x80746720-0x80746980; `.sbss`
 *   0x80795138-0x80795148 (14 functions / 0x9DC B).
 *   - the functions carry the NAND callbacks (`cbForNand*`) and read `Callback` (.sbss 0x80795140) and the error
 *     record `__ErrorInfo`; `.data` 0x80618C10 holds the path strings `/shared2/test2/dvderror.dat` and
 *     `/shared2/test2`
 *   - `__ErrorInfo` (.bss 0x80746880) lies after this unit's own statics (0x80746720..0x80746880), so it is defined
 *     here and read by `dvd.c`
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * NAMES. the unit name is a GUESS (named for the error file the strings name).
 * RESIDUALS. No body is written (14 functions), the largest `fn_804AB450` at 0x804AB450 (0x154 B); `python
 *   tools/units/sweepcomments.py --unit DVD/dvderror.c` lists them.
 */
