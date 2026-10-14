/*
 * DSP/dsp.c - the SDK DSP interface: mailbox access, initialisation and the public task calls.
 *
 * RANGE. `.text` 0x804A4F20-0x804A51C0; `.data` 0x80618508-0x80618588; `.sdata` 0x80793DC8-0x80793DD0; `.sbss`
 *   0x80795050-0x80795058 (9 functions / 0x2A0 B).
 *   - `.data` 0x80618508 holds the version string `<< RVL_SDK - DSP ... (0x4302_145) >>` (registered by `DSPInit`
 *     through `.sdata` 0x80793DC8), the `DSPInit(): Build Date: %s %s` printf format and its date and time strings
 *   - `.sbss` 0x80795050 is the init flag `DSPCheckInit` reads; the 4 bytes at 0x80795054 are read by nothing and pad
 *     the section to the 8-aligned start of the next object's `.sbss` (0x80795058): the decisive seam with
 *     `DSP/dsp_task.c`
 *   - the handler and `__DSP_exec_task`/`__DSP_boot_task` call the 4-5 instruction mailbox leaves at 0x804A4F20..
 *     0x804A4F74 out of line (REL24), which one TU under `-O4,p -inline auto` inlines: the leaves are in another TU
 *   - the `.data` string run turns from the `DSPInit` strings to the `__DSP_boot_task` strings at 0x80618588
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with the per-object `cflags_base`; unmeasured until bodies
 *   exist.
 * NAMES. the unit name is the SDK source file the version string and the public `DSP*` calls belong to.
 * RESIDUALS. No body is written (9 functions), the largest `DSPInit` at 0x804A4F80 (0xC0 B); `python
 *   tools/units/sweepcomments.py --unit DSP/dsp.c` lists them.
 */
