/*
 * Network/NetworkUnitPacket.cpp - STUB (no bodies yet).
 *
 * SECTIONS. extab 0x8001B96C..0x8001BBCC; extabindex 0x8003BB20..0x8003BDF0; .text 0x803F89CC..0x803FAE9C; .data 0x805FCC98..0x805FCE50; .sdata 0x80793958..0x80793960; .sdata2 0x8079C7C0..0x8079C7D0.
 *
 * WHAT IT IS. the unit packet band: the packet reader/writer (`networkPacket_*`), the bit-stream writers (`writeByte` ..,
 *   `networkStreamWriter_*`), the stream reader (`networkStreamReader_*`) and the stream queue (`networkStreamQueue_*`).
 *
 * WHY IT SITS HERE. the network pilot round 3 recut of the phase 4 stub `Network/NetworkCommunityPat.cpp` (0x803E4888..0x803FCC34),
 *   which phase1-e.md had flagged as several TUs.  Each cut is a function start where the `.data` run, the `.sdata2`
 *   pool and the extab/extabindex tables all change owner together (`splitcheck --readers` shows every pool and
 *   `.sdata` word of the range read only from its own unit).
 *
 * EDGE.  Both ends are the scout's; `splitcheck` reads no seam inside the `.data` run (the 0x805FCE10 zigzag does not FAIL).
 *
 * UNKNOWN. every body (the inventory is `ledger.py unit Network/NetworkUnitPacket.cpp`, the map and `splits.txt`).
 *
 * FLAGS. the `Network` lib's `cflags_network`, unmeasured: there is no body to measure them on.
 */

#include "Network/NetworkUnitPacket.h"
