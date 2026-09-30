/*
 * include/Network/NetworkPeerBase.h - the symbols `Network/NetworkPeerBase.cpp` owns that the rest of the Network band calls
 * (the error-record accessors and the small object's deleting destructor).
 *
 * The declarations moved out of `Network/network_transport.h` when `Network/network_transport.cpp` was split
 * into one unit per translation unit (docs/network-transport-split.md): a declaration belongs with the TU that
 * defines the symbol (rule 2).  The types they use are `Network/network_transport_types.h`'s.
 */

#ifndef NETWORK_NetworkPeerBase_H
#define NETWORK_NetworkPeerBase_H

#include "Network/network_transport_types.h"

extern "C" {

void networkPeerError_get(NetworkPeerBase* self, NetworkPeerErrorRecord* out);
void networkPeerError_clear(NetworkPeerBase* self);
/* untyped: opaque handle passed through - the callers hand the peer they already hold */
void networkPeerError_set(void* self, const void* source, u32 argument, s32 code);
/* untyped: opaque handle passed through - the object's layout belongs to the writer band */
void* networkSmallObject_dtor(void* self, s32 flags);
}

#endif /* NETWORK_NetworkPeerBase_H */
