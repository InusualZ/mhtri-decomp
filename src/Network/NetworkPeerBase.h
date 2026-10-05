/*
 * Network/NetworkPeerBase.h - the error-record accessors of `Network/NetworkPeerBase.cpp`; the types are
 *   `Network/network_transport_types.h`'s.
 */

#ifndef NETWORK_NetworkPeerBase_H
#define NETWORK_NetworkPeerBase_H

#include "Network/network_transport_types.h"
#include "Network/NetworkUniqueId.h"

extern "C" {

void networkPeerError_get(NetworkPeerBase* self, NetworkPeerErrorRecord* out);
void networkPeerError_clear(NetworkPeerBase* self);
/* untyped: opaque handle passed through - the callers hand the peer they already hold */
void networkPeerError_set(void* self, const void* source, u32 argument, s32 code);
}

#endif /* NETWORK_NetworkPeerBase_H */
