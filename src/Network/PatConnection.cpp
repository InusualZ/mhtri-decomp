/*
 * Network/PatConnection.cpp - STUB (no bodies yet).
 *
 * SECTIONS. extab 0x8001BBCC..0x8001BCAC; extabindex 0x8003BDF0..0x8003BF40; .text 0x803FAE9C..0x803FCC34; .ctors 0x8056F3BC..0x8056F3C0; .data 0x805FCE50..0x80600978; .sdata 0x80793960..0x80793968.
 *
 * WHAT IT IS. the Pat server connection: `setServerAddress`, `sendCommand`, the `readUInt*`/`writeUInt*` buffer codecs,
 *   `flushBuffer`/`encryptBuffer` - the request writers the session state machine drives - and the band's static
 *   initialiser (the `.ctors` word 0x8056F3BC = 0x803FC5B8).
 *
 * WHY IT SITS HERE. the network pilot round 3 recut of the phase 4 stub `Network/NetworkCommunityPat.cpp` (0x803E4888..0x803FCC34),
 *   which phase1-e.md had flagged as several TUs.  Each cut is a function start where the `.data` run, the `.sdata2`
 *   pool and the extab/extabindex tables all change owner together (`splitcheck --readers` shows every pool and
 *   `.sdata` word of the range read only from its own unit).
 *
 * EDGE.  Left 0x803FAE9C (the scout's); the right edge is the registered start of `Network/PatInterface.cpp`.
 *
 * UNKNOWN. every body (the inventory is `ledger.py unit Network/PatConnection.cpp`, the map and `splits.txt`).
 *
 * FLAGS. the `Network` lib's `cflags_network`, unmeasured: there is no body to measure them on.
 */

#include "Network/PatConnection.h"
