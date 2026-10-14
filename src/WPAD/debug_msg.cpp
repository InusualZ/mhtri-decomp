/* WPAD/debug_msg.cpp - the WPAD library's print helper: the empty varargs body every WPAD log call goes through.
 * RANGE. .text 0x804F9AE0-0x804F9B30 (1 function); no data.  Seam with WPAD/WPADMem.cpp at 0x804F9AE0: the SDK file list
 *   (WPAD.c, WPADHIDParser.c, WPADEncrypt.c, WPADClamp.c, WPADMem.c, debug_msg.c), codegen-neutral; WPAD.c, the parser and
 *   the key generators call the empty body out of line.
 * FLAGS. `cflags_base` (-O4,p, 16-byte function alignment).
 * NAMES. GUESS: the unit name (SDK spelling) and `WPADiDebugPrint`.
 * RESIDUALS. none.
 */

#include "types.h"
#include "WPAD/debug_msg.h"

/* Takes the format of a log line and discards it; every WPAD log call goes through here. */
void WPADiDebugPrint(const char* format, ...)
{
}
