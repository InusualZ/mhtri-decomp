/*
 * nw4r/db_assert.cpp - nw4r::db's assert reporters: `Panic`, `Warning` and their console/stack-trace helpers.
 * RANGE. .text 0x80500868-0x80500CF8 (6 functions); .data 0x8062F030-0x8062F0B0, .bss 0x80760C48-0x80760C78, .sdata
 *   0x807941E0-0x807941E8, .sbss 0x80795758-0x80795768.  No extab, no .sdata2.
 * RANGE. Left edge 0x80500868: its first function reads the console pointer at .sbss 0x8079575C that only
 *   this range reads (fn_80500868, fn_805009AC, Warning, fn_80500CE0).  0x80500770-0x80500868 before it
 *   (an empty varargs printer, a ring-buffer line count, a text-colour setter that tail-calls 0x805045A0) is
 *   nw4r::db console and ut code by content and stays in `WPAD/wpad.cpp` (unproven seam).
 * RANGE. Right edge 0x80500CF8: the next two functions are nw4r::math's table-driven exp/log (they read
 *   .data 0x8062F0B0/0x8062F1B8 and .sdata2 0x8079D480-0x8079D494), which `nw4r/math_arithmetic.cpp` owns.
 *   Every data piece here is read only from this range.
 * FLAGS. the `OS` lib's `cflags_os`, as the neighbouring `WPAD/wpad.cpp` and `nw4r/math_arithmetic.cpp`
 *   (unmeasured until bodies exist).
 * NAMES. `Panic` and `Warning` are the map's manglings; by the nw4r source the others are a GUESS:
 *   fn_80500868 `Assertion_Printf_`, fn_80500900 `ShowStack_`, fn_805009AC `VPanic`, fn_80500CE0
 *   `WarningAlarmFunc_` (the map keeps the stems).
 * RESIDUALS. Unwritten: every function in the range.
 */

#include "nw4r/db_assert.h"
