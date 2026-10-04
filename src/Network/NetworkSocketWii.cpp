/*
 * Network/NetworkSocketWii.cpp - STUB (no bodies yet).
 *
 * SECTIONS. extab 0x8001B884..0x8001B918; extabindex 0x8003BA00..0x8003BAC0; .text 0x803F7538..0x803F84B8; .data 0x805FC920..0x805FC9D0; .sdata2 0x8079C7B8..0x8079C7C0.
 *
 * WHAT IT IS. the Wii socket class `NetworkSocketWii` - the "NetworkSocketWii::init()"/"::open()" strings and its table 0x805FC984,
 *   which `constructNetworkSocket` (0x803F7548) stores - with the socket handle accessors `getBytesAvailableToRead` and
 *   `networkSocketHandle_getLastError` ahead of it.
 *
 * WHY IT SITS HERE. the network pilot round 3 recut of the phase 4 stub `Network/NetworkCommunityPat.cpp` (0x803E4888..0x803FCC34),
 *   which phase1-e.md had flagged as several TUs.  Each cut is a function start where the `.data` run, the `.sdata2`
 *   pool and the extab/extabindex tables all change owner together (`splitcheck --readers` shows every pool and
 *   `.sdata` word of the range read only from its own unit).
 *
 * EDGE.  Left 0x803F7538, not 0x803F7548: the two 8-byte accessors read the socket handle (+0x08 the last error) and
 *   carry no extab record, so nothing pins them to the fetcher; `splitcheck` reads both cuts the same.
 *
 * UNKNOWN. every body (the inventory is `ledger.py unit Network/NetworkSocketWii.cpp`, the map and `splits.txt`).
 *
 * FLAGS. the `Network` lib's `cflags_network`, unmeasured: there is no body to measure them on.
 */

#include "Network/NetworkSocketWii.h"
