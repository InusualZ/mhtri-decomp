/*
 * Network/NetworkConnection.cpp - the connection base `NetworkConnectionStable` is built on (constructor 0x803CA1D4,
 *   table 0x805F9190: it creates the slot's peer by kind and owns it at +0x0C), the member mutex class (table
 *   0x805F91E0) and the peer constructors the base calls (the 0x663C-byte peer, `NetworkPeerUdp`).
 * RANGE. .text 0x803CA1D4-0x803CA49C (9 functions), extab 0x80019584-0x80019618, extabindex 0x80039EAC-0x80039EF4
 *   (6 entries), .data 0x805F9190-0x805F91F0 (the two tables).  Left edge: the `.data` zigzag at 0x805F9190 (after
 *   `__vt__17NetworkStreamSink`).  Right edge: the V->S seam at 0x805F91F0 (the `NetworkConnectionStable[%d]` strings
 *   start a new TU) and `NetworkConnectionStable`'s constructor 0x803CA49C (its extabindex record opens the next run);
 *   the three float setters 0x803CA484..0x803CA49C are the base table's slots +0x3C..+0x44, so they are this class's.
 * FLAGS. the library's flags plus `#pragma peephole off`, measured on the six written rows.
 * NAMES. The file and class names are GUESSes: the base class of `NetworkConnectionStable`; the slot names are read off
 *   the derived class's overrides.  GUESS: setInterval, setTimeout, setLimit (the defaults each new connection copies).
 * NAMES. GUESS: NetworkMutex (the member mutex the network records embed).
 * RESIDUALS. `extab` (flip blocker): 0x80 of 0x94 B - retail's `NetworkPeerGameSpy` constructor carries a cleanup
 *   record against `NetworkPeerBase`'s destructor (`destroy__15NetworkPeerBaseFs`), which this project models as the
 *   plain virtual `destroy` the Matching peers are built on.
 * SHAPES. `NetworkMutex`'s destructor is complete with an empty body (it only frees on request) and matches.
 * SHAPES. `NetworkPeerUdp`'s constructor is complete with an empty body (the compiler emits the base call and the
 *   table store) and matches.
 */

#include "Network/NetworkConnection.h"
#include "Network/NetworkPeerBuffer.h"
#include "Network/NetworkPeerUdp.h"
#include "Network/NetworkPeerMcs.h"
#include "Network/GameSpyInterfaceThread.h"      /* NetworkPeerGameSpy */
#include "Network/network_shared_data.h"      /* the connection defaults */

#pragma peephole off

/* Clears the callback, owner and peer, then creates the peer of `kind`: 1 a buffer peer, 2 a Udp peer, 4 and 5 an Mcs
 * peer (armed and not), 6 a GameSpy peer. */
NetworkConnection::NetworkConnection(s32 kind)
{
    callback_04 = NULL;
    owner_08 = NULL;
    peer_0C = NULL;
    switch (kind) {
    case 1:
        peer_0C = new NetworkPeerBuffer();
        break;
    case 2:
        peer_0C = new NetworkPeerUdp();
        break;
    case 4:
        peer_0C = new NetworkPeerMcs(1);
        break;
    case 5:
        peer_0C = new NetworkPeerMcs(0);
        break;
    case 6:
        peer_0C = new NetworkPeerGameSpy();
        break;
    }
}

/* Chains the peer base and builds the receive queue's mutex; the interface is bound later (`setContext`). */
NetworkPeerGameSpy::NetworkPeerGameSpy()
{
}

/* Frees the mutex on request (the OS mutex needs no teardown). */
NetworkMutex::~NetworkMutex()
{
}

/* Initialises the OS mutex. */
NetworkMutex::NetworkMutex()
{
    OSInitMutex(&mutex_04);
}

/* Chains the peer base; the Udp peer's own fields are bound later (`setContext`). */
NetworkPeerUdp::NetworkPeerUdp()
{
}

/* Destroys the peer. */
NetworkConnection::~NetworkConnection()
{
    if (peer_0C != NULL) {
        if (peer_0C != NULL) {
            peer_0C->destroy(1);
        }
        peer_0C = NULL;
    }
}

/* Sets the keep-alive interval new connections start with. */
void NetworkConnection::setInterval(f32 seconds)
{
    networkConnectionDefaultInterval = seconds;
}

/* Sets the connect timeout new connections start with. */
void NetworkConnection::setTimeout(f32 seconds)
{
    networkConnectionDefaultTimeout = seconds;
}

/* Sets the receive timeout new connections start with. */
void NetworkConnection::setLimit(f32 seconds)
{
    networkConnectionDefaultLimit = seconds;
}
