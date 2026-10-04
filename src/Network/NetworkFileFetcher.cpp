/*
 * Network/NetworkFileFetcher.cpp - STUB (no bodies yet).
 *
 * SECTIONS. extab 0x8001B780..0x8001B884; extabindex 0x8003B8F8..0x8003BA00; .text 0x803F6458..0x803F7538; .data 0x805FC820..0x805FC920; .sdata 0x80793950..0x80793958.
 *
 * WHAT IT IS. the file fetcher: `NetworkFileFetcher` (constructor 0x803F6554, table 0x805FC880, `open`/`copyError` named in the map;
 *   the class is declared in `Network/network_pat_control.h`), its base (0x803F6458 stores the dtor-only table
 *   0x805FC820), the kind-2 fetcher `constructNetworkFetcherKind2` (0x803F73C0, table 0x805FC8A8) and a second
 *   dtor-only base table 0x805FC8D0 (stored by 0x803F74D8).
 *
 * WHY IT SITS HERE. the network pilot round 3 recut of the phase 4 stub `Network/NetworkCommunityPat.cpp` (0x803E4888..0x803FCC34),
 *   which phase1-e.md had flagged as several TUs.  Each cut is a function start where the `.data` run, the `.sdata2`
 *   pool and the extab/extabindex tables all change owner together (`splitcheck --readers` shows every pool and
 *   `.sdata` word of the range read only from its own unit).
 *
 * EDGE.  Left 0x803F6458, not 0x803F63D8: 0x803F6458 is the first function whose extabindex entry is this run's (0x8003B8F8)
 *   and it stores the run's first table 0x805FC820.  DATA ORDER: `splitcheck` reads a zigzag at 0x805FC8A8 (three tables
 *   in constructor text order, where one TU emits them in reverse).  Measured: merging the unit into either neighbour
 *   keeps the FAIL, splitting it at 0x803F73C0 moves it to 0x805FC8D0, only a three-way split (0x803F73C0, 0x803F74D8,
 *   0x118 and 0x60 bytes of code) clears it - not taken on that evidence alone (round 3 report).
 *
 * UNKNOWN. every body (the inventory is `ledger.py unit Network/NetworkFileFetcher.cpp`, the map and `splits.txt`).
 *
 * FLAGS. the `Network` lib's `cflags_network`, unmeasured: there is no body to measure them on.
 */

#include "Network/NetworkFileFetcher.h"
