/*
 * Network/NetworkPool.cpp - the network pool singleton (`getNetworkPool`, its constructor 0x80412528 and
 *   `NetworkPool::start`), the NHTTP wrappers around it and the library's random generator `NetworkRandom`.  No bodies
 *   yet.
 * RANGE. .text 0x804123F8-0x80413450 (49 functions); .data 0x80602428-0x806024B8, .sdata 0x80793988-0x80793990, .sbss
 *   0x80794CB8-0x80794CC0, .sdata2 0x8079C868-0x8079C870, extab, extabindex.  Left edge: the `.data` V->S seam at
 *   0x80602428 (PatInterface's table 0x80602198, then this range's first string "NHTTPStartup"); `onNHTTPDestroyed`
 *   (0x804123F8) is the `NHTTPDestroy` callback only `stepCleanup__11NetworkPoolFv` takes.  `getNetworkPool`
 *   (0x8041241C) reads the `.sbss` word 0x80794CB8, `fn_80412C98` the `.sdata` 0x80793988 and
 *   `clearState__11NetworkPoolFv` the `.sdata2` 0x8079C868.  Right edge: `mediatorEventCallback` 0x80413450
 *   (`Network/NetworkWiiMediator.cpp`).
 * FLAGS. `-O3 -inline noauto` (configure.py; docs/network.md), unmeasured until a body lands.
 * NAMES. `getNetworkPool` is a GUESS named by the mediator band (it returns the singleton).
 * RESIDUALS. every body (the inventory: `ledger.py unit Network/NetworkPool.cpp`; the classes are declared in
 *   `Network/NetworkPool.h`).  `splitcheck --unit` reads a zigzag `.data` seam at 0x806024A0: `NetworkPool`'s table
 *   0x80602490 sits before `NetworkRandom`'s 0x806024A0, the reverse of MWCC's order for one TU, so `NetworkRandom`
 *   (constructor 0x80413384) is probably a TU of its own starting in 0x804130EC..0x80413384 - not cut.
 */

#include "Network/NetworkPool.h"
