/*
 * DSP/dsp_task.c - the SDK DSP task manager: the DSP interrupt handler and the task queue.
 *
 * RANGE. `.text` 0x804A5210-0x804A5AB0; `.data` 0x80618588-0x806186C8; `.sbss` 0x80795058-0x80795070 (5 functions /
 *   0x8A0 B).
 *   - the function at 0x804A5210 is the DSP interrupt handler `DSPInit` installs (`__OSSetInterruptHandler(7, ...)`),
 *     the SDK's `__DSPHandler`; the others are `__DSP_exec_task`, `__DSP_boot_task`, `__DSP_insert_task` and
 *     `__DSP_remove_task`
 *   - `.data` 0x80618588 holds the `DSP is booting task` and `__DSP_boot_task()` printf formats
 *   - `.sbss` 0x80795058..0x80795070 holds the rude task, its pending flag and the four task list pointers (tmp, last,
 *     first, current); it starts 8-aligned after the unread 4 bytes at 0x80795054 that end `DSP/dsp.c`
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with the per-object `cflags_base`; unmeasured until bodies
 *   exist.
 * NAMES. the unit name is the SDK source file that defines the `__DSP_*` task functions and globals.
 * RESIDUALS. `.data` 0x80618698..0x806186BC is the string `__DSP_add_task() : Added task : 0x%08X\n`, which no
 *   function reads (its `__DSP_add_task` was deadstripped) and the source must still emit. No body is written (5
 *   functions), the largest, the handler at 0x804A5210 (0x42C B); `python tools/units/sweepcomments.py --unit
 *   DSP/dsp_task.c` lists them.
 */
