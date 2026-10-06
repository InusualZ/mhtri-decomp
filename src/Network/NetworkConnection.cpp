/*
 * Network/NetworkConnection.cpp - the connection base `NetworkConnectionStable` is built on (constructor 0x803CA1D4,
 *   table 0x805F9190: it creates the slot's peer by kind and owns it at +0x0C), the member mutex class (table
 *   0x805F91E0) and the peer constructors the base calls (the 0x663C-byte peer, `NetworkPeerUdp`).
 * RANGE. .text 0x803CA1D4-0x803CA484 (6 functions), extab 0x80019584-0x80019618, extabindex 0x80039EAC-0x80039EF4
 *   (6 entries), .data 0x805F9190-0x805F91F0 (the two tables).  Left edge: the `.data` zigzag at 0x805F9190 (after
 *   `__vt__17NetworkStreamSink`).  Right edge: the V->S seam at 0x805F91F0 (the `NetworkConnectionStable[%d]` strings
 *   start a new TU); the `.text` edge is a GUESS at 0x803CA484: the three float setters 0x803CA484..0x803CA49C between
 *   the base destructor and `NetworkConnectionStable`'s constructor are called only from that class (0x803CBF48), so
 *   they go with it; they carry no extab record, so the extab/extabindex edges are exact either way.
 * FLAGS. the `NetworkStreamSink` row's flags, unmeasured: there is no body yet.
 * NAMES. The file name is a GUESS: the base class of `NetworkConnectionStable`.
 * RESIDUALS. Every function unwritten: the base class is not declared (`NetworkConnectionStable` is declared without a
 *   base in `Network/NetworkSessionStable.h`), the mutex record is a class (its constructor stores 0x805F91E0) that the
 *   band calls as the C pair `networkInstance_initMutex`/`_destroyMutex`, and neither peer class declares its
 *   constructor.
 */

#include "Network/NetworkConnection.h"
