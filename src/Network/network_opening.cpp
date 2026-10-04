/*
 * Network/network_opening.cpp - the network mediator's connection opening and terms-of-use steps.
 *
 * `.text` 0x804155D4..0x80417BC0.  Sections: extab 0x8001CBF4..0x8001CD94; extabindex 0x8003D524..0x8003D764;
 * .rodata 0x80570E70..0x80570E98 (read by 0x80416A30); .data 0x80602824..0x80602968; .sdata2 0x8079C874..0x8079C878 (read by `initMediatorTerms`).
 *
 * WHAT IT IS.  The mediator's opening steps (`openingStart`, `setConnectionPaths`, `getNASToken`, the terms
 * check/update, the game-info copy pair and the transfer-state setters declared in
 * include/Network/network_opening.h) - not written yet; their inventory is `ledger.py unit
 * Network/network_opening.cpp`, the map and `splits.txt`.
 *
 * BOUNDARY.  The base library class `sNetworkLibrary` that followed (0x80417BC0..0x8041891C) is its own unit,
 * `Network/sNetworkLibrary.cpp`, with the whole `.data` and `.sbss` this unit used to claim (its header has the
 * seam evidence); the worker-thread entry points after it are `Network/sNetworkLibraryWii.cpp`'s.  `.data`
 * 0x80602824..0x80602968 is the five strings only the opening steps read (claimed with the split, so the data
 * closure holds; not emitted until the steps are written).  The two tables after them (0x80602968, 0x80602978)
 * close this TU's `.data` by MWCC's order but are read by `Network/network_layer_io.cpp` and are code-pointer
 * tables (rule 10), so they stay unclaimed until a class here emits them.
 *
 * FLAGS.  `-O3` in place of the lib's `-O4,p` (configure.py); the evidence was measured on the base-class rows,
 * which moved out with their unit, so it is unmeasured for the opening steps.
 */
#include "Network/network_opening.h"
