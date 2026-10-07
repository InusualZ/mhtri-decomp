/*
 * DB/db.c - the SDK debugger interface (exception destination hook, DBPrintf).
 *
 * RANGE. `.text` 0x804A4AC0-0x804A4BC0; `.data` 0x806184E0-0x806184F8; `.sbss` 0x80795040-0x80795048 (5 functions /
 *   0xE8 B).
 *   - the five functions are the whole debugger-interface run: the exception destination thunk, its aux handler
 *     (OSReport + OSDumpContext + PPCHalt) and the `__DBInterface` accessors
 *   - `.data` 0x806184E0 is the aux handler's format string; `.sbss` `__DBInterface` / `DBVerbose` are read by no
 *     other unit
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. No body is written (5 functions), the largest `DBPrintf` at 0x804A4B70 (0x50 B); `python
 *   tools/units/sweepcomments.py --unit DB/db.c` lists them.
 */
