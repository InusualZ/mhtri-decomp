/* nw4r/db_console.cpp - nw4r::db console and ut helpers that sat at the end of the WPAD stack range: an empty varargs printer, the console ring-buffer line count and a text-colour setter.
 * RANGE. .text 0x80500770-0x80500868 (3 functions); no data.
 *   Edges: the left edge 0x80500770 follows the WUD print helper (`WUD/wud.cpp`); the right edge is the
 *   first function of `nw4r/db_assert.cpp`, whose Panic/Warning call the printer and the line count; the colour
 *   setter is also called by `nw4r/fn_80502828.cpp` and the homebutton text panel.
 * FLAGS. the `nw4r` lib block (`cflags_nw4r`), as its neighbour `nw4r/db_assert.cpp`; unmeasured until bodies exist.
 * NAMES. this file name is a GUESS (the console record `nw4r::db::detail::ConsoleHead` of `nw4r/db_console.h` is its
 *   context); the three functions are still generated names.
 * RESIDUALS. every body unwritten (stub); the grouping of the three into one TU is unproven (the colour setter is
 *   ut code by content).
 */
