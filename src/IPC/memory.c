/*
 * IPC/memory.c - the SDK IOS heap helpers (`iosCreateHeap`, `iosAllocAligned`, `iosFree`).
 *
 * RANGE. `.text` 0x804BD070-0x804BD5A0; `.bss` 0x807473C0-0x80747440 (4 functions / 0x518 B).
 *   - `.bss` 0x807473C0 (the heap table) is read only by these functions; the neighbours read disjoint data
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * NAMES. the unit name is a GUESS (the library's file name in the SDK scheme).
 * RESIDUALS. No body is written (4 functions), the largest `fn_804BD1A0` at 0x804BD1A0 (0x1FC B); `python
 *   tools/units/sweepcomments.py --unit IPC/memory.c` lists them.
 */
