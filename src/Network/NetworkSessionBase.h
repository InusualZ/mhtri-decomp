/*
 * Network/NetworkSessionBase.h - the mutex wrappers, the session accessor and the notify setter of
 *   `Network/NetworkSessionBase.cpp`; the types are `Network/network_transport_types.h`'s.
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
