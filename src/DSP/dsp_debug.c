/*
 * DSP/dsp_debug.c - the SDK DSP library's debug printf.
 *
 * RANGE. `.text` 0x804A51C0-0x804A5210 (1 function / 0x50 B); no data.
 *   - `__DSP_debug_printf` sits between the last public `DSP/dsp.c` call (`DSPAssertTask`) and the interrupt handler
 *     that opens `DSP/dsp_task.c`
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with the per-object `cflags_base`; unmeasured until a body
 *   exists.
 * NAMES. the unit and its boundaries are a GUESS from the SDK's file layout (dsp.c, dsp_debug.c, dsp_task.c): no
 *   data or alignment seam separates this function from `DSP/dsp.c`, so it may belong to that unit.
 * RESIDUALS. No body is written (1 function), `__DSP_debug_printf` at 0x804A51C0 (0x50 B); `python
 *   tools/units/sweepcomments.py --unit DSP/dsp_debug.c` lists it.
 */
