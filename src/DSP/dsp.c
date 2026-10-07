/*
 * DSP/dsp.c - the SDK DSP interface: mailbox access, task manager and its debug printf.
 *
 * RANGE. `.text` 0x804A4F20-0x804A5AB0; `.data` 0x80618508-0x806186C8; `.sdata` 0x80793DC8-0x80793DD0; `.sbss`
 *   0x80795050-0x80795070 (15 functions / 0xB50 B).
 *   - version string `<< RVL_SDK - DSP ... (0x4302_145) >>` at `.data` 0x80618508 is registered by `DSPInit` through
 *     `.sdata` 0x80793DC8
 *   - `.data` 0x80618588 (DSP boot task image) is read by `__DSP_boot_task` only; `.sbss` 0x80795050..0x80795070
 *     holds the init flag and the task queue heads
 *   - no data seam exists inside the run: one unit for the library, the file boundaries (mail/reset, debug, task) are
 *     not cut
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * NAMES. the unit name is a GUESS (the library's first source file).
 * RESIDUALS. No body is written (15 functions), the largest `fn_804A5210` at 0x804A5210 (0x42C B); `python
 *   tools/units/sweepcomments.py --unit DSP/dsp.c` lists them.
 */
