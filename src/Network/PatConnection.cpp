/*
 * Network/PatConnection.cpp - the Pat server connection: `setServerAddress`, `sendCommand`, the `readUInt*`/`writeUInt*`
 *   buffer codecs, `flushBuffer`/`encryptBuffer` (the request writers the session state machine drives) and the band's
 *   static initialiser (the `.ctors` word 0x8056F3BC = 0x803FC5B8).  No bodies yet.
 * RANGE. .text 0x803FAE9C-0x803FCC34 (37 functions); .ctors 0x8056F3BC-0x8056F3C0, .data 0x805FCE50-0x80600978, .sdata
 *   0x80793960-0x80793968, extab, extabindex.  Each cut is a function start where the `.data` run, the `.sdata2` pool
 *   and the extab/extabindex tables all change owner together (`splitcheck --readers`: every pool and `.sdata` word of
 *   the range is read only from its own unit); the right edge is the start of `Network/PatInterface.cpp`.
 * FLAGS. the `Network` lib's `cflags_network`, unmeasured: there is no body to measure them on.
 * RESIDUALS. every body (the inventory: `ledger.py unit Network/PatConnection.cpp`, the map and `splits.txt`).
 */

#include "Network/PatConnection.h"
