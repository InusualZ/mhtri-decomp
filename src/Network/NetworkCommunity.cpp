/*
 * Network/NetworkCommunity.cpp - STUB (no bodies yet).
 *
 * SECTIONS. extab 0x8001B0A0..0x8001B178; extabindex 0x8003B448..0x8003B544; .text 0x803EF668..0x803F0294; .data 0x805FC390..0x805FC4D0; .sdata2 0x8079C7A0..0x8079C7A8.
 *
 * WHAT IT IS. the community layer base class `NetworkCommunity` - the counterpart of `NetworkLayer` for `NetworkCommunityPat`:
 *   its `.data` opens with the request descriptors (`{0, slot, 0}` member-function pointers 0x68..0x8C), then its own
 *   "NetworkCommunity::deleteRequest: request is moving." string and its table 0x805FC440, which the constructor
 *   0x803EF668 stores.  The class name is that string's.
 *
 * WHY IT SITS HERE. the network pilot round 3 recut of the phase 4 stub `Network/NetworkCommunityPat.cpp` (0x803E4888..0x803FCC34),
 *   which phase1-e.md had flagged as several TUs.  Each cut is a function start where the `.data` run, the `.sdata2`
 *   pool and the extab/extabindex tables all change owner together (`splitcheck --readers` shows every pool and
 *   `.sdata` word of the range read only from its own unit).
 *
 * EDGE.  Left 0x803EF668, not 0x803EF300 (the scout's uncertain alternative): 0x803EF300..0x803EF668 are the
 *   `setCollectionLog*` siblings, which take a `NetworkLayerPat*` and close `NetworkLayerPat`'s band the way
 *   `setSessionLog*` close `NetworkSessionManagerPat`'s before the `NetworkLayer` constructor; `splitcheck` reads both
 *   cuts the same, so the parallel decides.
 *
 * UNKNOWN. every body (the inventory is `ledger.py unit Network/NetworkCommunity.cpp`, the map and `splits.txt`).
 *
 * FLAGS. the `Network` lib's `cflags_network`, unmeasured: there is no body to measure them on.
 */

#include "Network/NetworkCommunity.h"
