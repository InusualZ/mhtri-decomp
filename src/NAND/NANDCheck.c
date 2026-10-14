/*
 * NAND/NANDCheck.c - the Revolution SDK NAND library's usage and quota checks: the banner initializer, the usage
 *    query and the asynchronous home-directory quota check with its user-area completion.
 * RANGE. .text 0x804C9480-0x804C9910 (4 functions); .data 0x8061BAF0-0x8061BBD8 (the user-area directory strings, the
 *    pointer table that lists them, and the completion's strings); .sdata 0x80793F30-0x80793F48 (the banner's wide
 *    literals).  Cut from the NAND library 0x804C70E0-0x804C9F00.  Seam with NAND/NANDCore.c at 0x804C9480:
 *    `NANDGetUsage` (0x804C9584) and `NANDCheckAsync` (0x804C9648) call `nandIsInitialized` out of line while
 *    NAND/NANDCore.c inlines it.  Seam with NAND/NANDLogging.c: data order only (no shared statics).  The banner
 *    initializer 0x804C9480 has no proven side and is kept at this unit's head.
 * FLAGS. cflags_base in the `OS` lib block of configure.py (Wii/1.3); no pragma; no float code.
 * NAMES. `NANDGetUsage` and `NANDCheckAsync` are the map's (dump).  GUESS: the unit name (SDK spelling).
 * RESIDUALS. no body written (4 functions): the banner initializer 0x804C9480-0x804C9560, `NANDGetUsage`
 *    0x804C9560-0x804C9618, `NANDCheckAsync` 0x804C9620-0x804C96CC and the user-area completion 0x804C96D0-0x804C9904;
 *    no data is emitted.  Evidence: `python tools/units/ledger.py unit NAND/NANDCheck.c`, the map, splits.txt.
 * SHAPES. none yet.
 */

#include "types.h"
#include "NAND/NANDCheck.h"
