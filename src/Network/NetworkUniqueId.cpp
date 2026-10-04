/*
 * Network/NetworkUniqueId.cpp - STUB (no bodies yet).
 *
 * SECTIONS. extab 0x8001B918..0x8001B96C; extabindex 0x8003BAC0..0x8003BB20; .text 0x803F84B8..0x803F89CC; .data 0x805FC9D0..0x805FCC98.
 *
 * WHAT IT IS. the network unique id (address object) class: its log strings name the class `NetworkUniqueId`, its table is
 *   0x805FCC58 and `networkSmallObject_construct` (0x803F87B8) is its constructor; the setters, the validity test,
 *   `exportTo` and the equality test follow.  The rename of the constructor to the class's mangling is a call-site
 *   type change (the callers build a `NetworkSmallObject`), so it waits for the class lane (round 3 report).
 *
 * WHY IT SITS HERE. the network pilot round 3 recut of the phase 4 stub `Network/NetworkCommunityPat.cpp` (0x803E4888..0x803FCC34),
 *   which phase1-e.md had flagged as several TUs.  Each cut is a function start where the `.data` run, the `.sdata2`
 *   pool and the extab/extabindex tables all change owner together (`splitcheck --readers` shows every pool and
 *   `.sdata` word of the range read only from its own unit).
 *
 * EDGE.  Both ends are the scout's: the `.data` run 0x805FC9D0..0x805FCC98 is this class's strings and table alone.
 *
 * UNKNOWN. every body (the inventory is `ledger.py unit Network/NetworkUniqueId.cpp`, the map and `splits.txt`).
 *
 * FLAGS. the `Network` lib's `cflags_network`, unmeasured: there is no body to measure them on.
 */

#include "Network/NetworkUniqueId.h"
