/*
 * Network/NetworkSessionBase.cpp - the session base class (its destructor is the key function that emits the
 *   0xA0-byte table) and the mutex/notify accessors the manager calls.
 *
 * One translation unit of the retail Network transport band, split out of `Network/network_transport.cpp`
 * (docs/network-transport-split.md holds the evidence and the confidence of each cut).  `.text`
 * 0x803CF654..0x803CF6D8, `.data` 0x805F99A0..0x805F9A40, extab 0x80019AAC..0x80019AB4, extabindex
 * 0x8003A338..0x8003A344.
 *
 * NAMES.  `NetworkSessionBase` is a GUESS (evidenced by `networkPeer_resetSlots` driving its +0x1C slot and the
 * setters' slot positions).  Every name here is the map's or a derived one; the derived ones are marked GUESS in
 * `Network/network_transport_types.h`.
 *
 * EDGES.  The left edge (0x803CF654) is unproven: `tudiscover` reports only weak signals there.  The right edge
 * (0x803CF6D8) is set by the setters' evidence: `setLimits`/`setHostTimeout`/`setSubhostTimeout`/`setRate` write
 * only globals that NetworkSessionStable functions read and are called only from Stable functions (0x803D2314
 * and 0x803D2368), so they live in `Network/NetworkSessionStable.cpp`; the destructor (the vtable's key function)
 * stays here.
 *
 * TABLE.  Its table (0x805F99A0, 0xA0 B: the deleting destructor, 33 pure slots and the four setters, which are
 * defined in the Stable unit) is emitted from `~NetworkSessionBase`, the key function (rule 10).
 *
 * FLAGS.  C++ under `cflags_network` (`-Cpp_exceptions on` gives the `extab`), per-unit `-O3`/`-pool off` (`configure.py`);
 * file-scope `#pragma peephole off` (playbook 39); each `dont_inline` region keeps a retail `bl` that `-inline auto` folds.
 *
 * RESIDUALS.  The constructor (0x803CF674, 32 B) and `getSomething5` (8 B) are not written.
 */
#include "types.h"
#include "Network/network_transport.h"
#include "Network/fn_803D3CE8.h"
#include "unsplit/NetworkData.h"
#include "unsplit/NetworkStream.h"
/* `unsplit/Network.h` is the Network band's code half (`getNetworkLogger` and the socket-pool helpers).  It
   cannot be included beside `unsplit/OS.h`: the two band headers declare `OSCreateThread`/`OSResumeThread` with
   different signatures and a TU that sees both fails with `(10197) illegal function overloading`. */
#include "unsplit/Network.h"
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

/* Publishes the value the session's put path consults. */
void NetworkSessionStable_setNotifyValue(u32 value)
{
    networkSessionNotifyValue = value;
}

#pragma dont_inline on

/* Deleting destructor of the peer-side object: frees on request, nothing else to chain. */
NetworkSessionBase::~NetworkSessionBase()
{
}

#pragma dont_inline off

}
