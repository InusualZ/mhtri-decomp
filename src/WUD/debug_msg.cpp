/* WUD/debug_msg.cpp - the WUD library's print helper: the empty varargs body every WUD log call goes through.
 * RANGE. .text 0x80500720-0x80500770 (1 function); no data.  Seam with WUD/WUDHidHost.cpp at 0x80500720: the SDK file
 *   list (WUD.c, WUDHidHost.c, debug_msg.c), codegen-neutral; both earlier units call the empty body out of line.
 * FLAGS. `cflags_base` (-O4,p, 16-byte function alignment).
 * NAMES. GUESS: the unit name (SDK spelling) and `WUDiDebugPrint`.
 * RESIDUALS. none.
 */

#include "WUD/debug_msg.h"

/* Takes a log line and discards it; every WUD log call goes through here. */
void WUDiDebugPrint(const char* format, ...)
{
}
