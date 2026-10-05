/*
 * include/Network/NetworkSessionBase.h - the symbols `Network/NetworkSessionBase.cpp` owns that the rest of the Network band calls
 * (the mutex wrappers, the session accessor and the notify setter).
 *
 * The declarations moved out of `Network/network_transport.h` when `Network/network_transport.cpp` was split
 * into one unit per translation unit (docs/network-transport-split.md): a declaration belongs with the TU that
 * defines the symbol (rule 2).  The types they use are `Network/network_transport_types.h`'s.
 */

#ifndef NETWORK_NETWORK_SESSION_BASE_H
#define NETWORK_NETWORK_SESSION_BASE_H

#include "Network/network_transport_types.h"

extern "C" {

/* untyped: opaque handle passed through - only the peer band owns the mutex layout */
void LockMutex(void* mutex);
/* untyped: opaque handle passed through - only the peer band owns the mutex layout */
void UnlockMutex(void* mutex);
/* untyped: opaque handle passed through - the peer the caller already holds */
u32 getNetworkBinaryState(void* self);
void NetworkSessionStable_setNotifyValue(u32 value);
}

#endif /* NETWORK_NETWORK_SESSION_BASE_H */
