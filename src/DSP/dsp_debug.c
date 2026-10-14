/*
 * DSP/dsp_debug.c - the SDK DSP library's debug printf.
 *
 * RANGE. .text 0x804A51C0-0x804A5210 (1 function); no data.
 * FLAGS. cflags_base in the `OS` lib block of configure.py (Wii/1.3); no pragma.
 * NAMES. the unit is a GUESS from the SDK's file layout (dsp.c, dsp_debug.c, dsp_task.c): no data or alignment seam
 *   separates this function from DSP/dsp.c, so it may belong to that unit.
 * RESIDUALS. none.
 * SHAPES. the body is empty: the varargs prologue (the cr1 test, the f1-f8 and r3-r10 saves) is what MWCC emits for
 *   an empty `...` function; a va_start adds the va_list setup the target does not have.
 */
#include "types.h"
#include "DSP/dsp_debug.h"

/* 0x804A51C0 (0x50): the DSP library's trace printf; prints nothing in a release build. */
void __DSP_debug_printf(const char* fmt, ...)
{
}
