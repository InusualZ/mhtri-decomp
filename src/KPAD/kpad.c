/*
 * KPAD/kpad.c - the SDK KPAD library (Wii Remote sample processing, calibration, read).
 *
 * RANGE. `.text` 0x804BD780-0x804C1760; `.data` 0x8061ADA0-0x8061AE00; `.bss` 0x80747540-0x80748B90; `.sdata`
 *   0x80793EB0-0x80793F00; `.sbss` 0x80795260-0x80795298; `.sdata2` 0x8079D200-0x8079D270 (26 functions / 0x3F30
 *   B).
 *   - version string `<< RVL_SDK - KPAD ... >>` at `.data` 0x8061ADA0 is registered by the init function through
 *     `.sdata` 0x80793EB0; `.bss` 0x80747540 (channel work) and `.sdata2` 0x8079D200..0x8079D270 are read only here
 *   - the run ends at the next registered unit `MEM/mem_heap.c` (0x804C1760)
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. the functions read tunables in `mh3_pad.cpp`'s `.sdata` run (0x80790EA0..0x80790ED4) and `.sbss`
 *   0x80794800 and write them from `KPADInit`: an extern pair the registered claims place in the
 *   neighbouring unit. No body is written (26 functions), the largest `fn_804BF890` at 0x804BF890 (0x8D0 B);
 *   `python tools/units/sweepcomments.py --unit KPAD/kpad.c` lists them.
 */
