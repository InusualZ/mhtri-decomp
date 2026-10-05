/*
 * Network/NetworkPool.cpp - the network pool singleton (`getNetworkPool`, its constructor 0x80412528 and
 * `NetworkPool::start`), the NHTTP wrappers around it and the library's random generator `NetworkRandom`.
 *
 * `.text` 0x804123F8..0x80413450.  Sections: extab 0x8001C8F4..0x8001C9B4; extabindex 0x8003D170..0x8003D278;
 * .data 0x80602428..0x806024B8; .sdata 0x80793988..0x80793990; .sbss 0x80794CB8..0x80794CC0;
 * .sdata2 0x8079C868..0x8079C874 (config/RMHE08/splits.txt).
 *
 * WHY IT SITS HERE.  The tail of the former `Network/network_layer_io.cpp`, its own TU since the round 4 fold gave
 *   the head to `Network/PatInterface.cpp`: the `.data` V->S seam at 0x80602428 (PatInterface's table 0x80602198,
 *   then this range's first string "NHTTPStartup") ends that TU.  The left edge 0x804123F8 is `onNHTTPDestroyed`, the
 *   `NHTTPDestroy` callback whose address only this range's `stepCleanup__11NetworkPoolFv` takes and which calls only this range;
 *   `getNetworkPool` (0x8041241C) reads this unit's `.sbss` word 0x80794CB8, `fn_80412C98` its `.sdata` 0x80793988
 *   and `clearState__11NetworkPoolFv` its `.sdata2` 0x8079C868.  Right edge: `mediatorEventCallback` 0x80413450
 *   (`Network/NetworkWiiMediator.cpp`'s band).
 *
 * UNKNOWN.  `splitcheck --unit` reads a zigzag `.data` seam at 0x806024A0 inside this range: `NetworkPool`'s table
 *   0x80602490 sits before `NetworkRandom`'s 0x806024A0, the reverse of MWCC's vtable order for one TU, so
 *   `NetworkRandom` (constructor 0x80413384) is probably a TU of its own starting in 0x804130EC..0x80413384 - not
 *   cut here (the round 4 fold keeps the tail whole).  No bodies yet; the classes are declared in
 *   `include/Network/NetworkPool.h`, the inventory is `ledger.py unit Network/NetworkPool.cpp` and the map.
 */

#include "Network/NetworkPool.h"
