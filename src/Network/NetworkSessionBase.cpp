/*
 * Network/NetworkSessionBase.cpp - the session base class (its destructor is the key function that emits the 0xA0-byte
 *   table) and the mutex/notify accessors the manager calls.
 * RANGE. .text 0x803CF654-0x803CF6D8 (6 functions); .data 0x805F99A0-0x805F9A40, extab, extabindex.  One TU of the
 *   transport band: docs/network.md (the left edge is unproven; the four setters `setLimits`/
 *   `setHostTimeout`/`setSubhostTimeout`/`setRate` write only globals NetworkSessionStable reads and are called only
 *   from it (0x803D2314, 0x803D2368), so they live in `Network/NetworkSessionStable.cpp`).
 * FLAGS. `-O3 -pool off` (configure.py; measured in docs/network.md); file-scope `#pragma peephole off`
 *   (playbook 39).
 * NAMES. `NetworkSessionBase` is a GUESS (`resetAllSlots__20NetworkSessionStableFv` drives its +0x1C slot; the setters'
 *   slot positions).  `getNetworkBinaryState` is a GUESS from the field it reads,
 *   `NetworkStateMachine::binaryState_6134` (the view `Network/PatInterface.h` owns).  The other derived names are
 *   marked in `Network/network_transport_types.h`.
 * RESIDUALS. none.
 * SHAPES. The table (0x805F99A0: the deleting destructor, 33 pure slots and the four setters) is emitted from
 *   `~NetworkSessionBase`, the key function (rule 10).  Each `dont_inline` region keeps a retail `bl`.
 */
#include "types.h"
#include "Network/network_transport.h"
#include "Network/NetworkSessionManager.h"
/* `unsplit/Network.h` is the Network band's code half (`getNetworkLogger` and the socket-pool helpers).  It
   cannot be included beside `unsplit/OS.h`: the two band headers declare `OSCreateThread`/`OSResumeThread` with
   different signatures and a TU that sees both fails with `(10197) illegal function overloading`. */
#include "unsplit/Network.h"
#include "Network/PatInterface.h"   /* owner Network/PatInterface.cpp (the state machine and the packet layer) */
#include "unsplit/Runtime.PPCEABI.H.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The range keeps retail's unfused forms: a separate `extsh`+`cmpwi` before the free, and the memset argument setup
   in source order (`addi` before the two `li`s) - the peephole pass folds both.  Scoped off for the file. */
#pragma peephole off

extern "C" {

/* Unlocks the mutex a peer keeps at its own +0x04. */
/* untyped: opaque handle passed through - only the peer band owns the mutex layout */
void UnlockMutex(void* mutex)
{
    NetworkPeerLock* lock = (NetworkPeerLock*)mutex;

    OSUnlockMutex(&lock->mutex_04);
}

/* Locks the mutex a peer keeps at its own +0x04. */
/* untyped: opaque handle passed through - only the peer band owns the mutex layout */
void LockMutex(void* mutex)
{
    NetworkPeerLock* lock = (NetworkPeerLock*)mutex;

    OSLockMutex(&lock->mutex_04);
}

/* Returns the state machine's binary state byte (+0x6134); 10 means the maintenance-reject path. */
/* untyped: opaque handle passed through - callers hand over the session-manager instance */
u32 getNetworkBinaryState(void* self)
{
    return ((NetworkStateMachine*)self)->binaryState_6134;
}

/* Publishes the value the session's put path consults. */
void NetworkSessionStable_setNotifyValue(u32 value)
{
    networkSessionNotifyValue = value;
}

/* Builds the base: its table, then an empty callback, user pointer and host flag. */
NetworkSessionBase::NetworkSessionBase()
{
    callback_04 = NULL;
    user_08 = NULL;
    isHost_0C = 0;
}

#pragma dont_inline on

/* Deleting destructor of the peer-side object: frees on request, nothing else to chain. */
NetworkSessionBase::~NetworkSessionBase()
{
}

#pragma dont_inline off

}
