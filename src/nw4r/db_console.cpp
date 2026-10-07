/* nw4r/db_console.cpp - nw4r::db console helpers that sat at the end of the WPAD stack range: an empty varargs printer and the console ring-buffer line count.
 * RANGE. .text 0x80500770-0x80500868 (3 functions); no data.
 *   Edges: the left edge 0x80500770 follows the WUD print helper (`WUD/wud.cpp`); the right edge is the
 *   first function of `nw4r/db_assert.cpp`, whose Panic/Warning call the printer and the line count; the colour
 *   setter is also called by `nw4r/fn_80502828.cpp` and the homebutton text panel.
 * FLAGS. the `nw4r` lib block (`cflags_nw4r`), as its neighbour `nw4r/db_assert.cpp`.
 * NAMES. this file name is a GUESS (the console record `nw4r::db::detail::ConsoleHead` of `nw4r/db_console.h` is its
 *   context); `Console_Printf` and `Console_GetTotalLines` are GUESSES from what the two bodies do and who calls them
 *   (`nw4r/db_assert.cpp`).
 * RESIDUALS. Flip blocker: .text object 0xD4 vs claimed 0xF8 (the unwritten 0x24-byte function below).  `fn_80500844` (0x80500844, 0x24 B) is not written: it is the out-of-line copy of the inline
 *   `ut::CharWriter::SetTextColor` (`nw4r/fn_80502828.h`), a weak function emitted by a caller that is not in the
 *   image, so no source in this range references it; the grouping of the three into one TU is unproven.
 */
#include "nw4r/db_console.h"
#include "OS/OSInterrupt.h"
#include "stdarg.h"

namespace nw4r {
namespace db {

/* 0x80500770 (0x68): the console printer, an empty body that only spills the argument registers. */
void Console_Printf(ConsoleHandle console, const char* format, ...) {
    va_list vlist;

    va_start(vlist, format);
    va_end(vlist);
}

/* 0x805007D8 (0x6C): counts the lines held by the ring buffer, with interrupts off. */
s32 Console_GetTotalLines(ConsoleHandle console) {
    s32 diff;
    u16 lines;
    s32 total;
    BOOL enabled = OSDisableInterrupts();

    diff = console->printTop - console->ringTop;
    if (diff < 0) {
        diff += console->height;
    }
    lines = diff;
    if (console->printXPos != 0) {
        lines++;
    }
    total = console->ringTopLineCnt + lines;
    OSRestoreInterrupts(enabled);
    return total;
}

}  // namespace db
}  // namespace nw4r
