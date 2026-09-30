/*
 * include/Network/NetworkSessionStable.h - the symbols `Network/NetworkSessionStable.cpp` owns that the rest of the Network band calls
 * (the session state machine's helpers, nonce and small-object destructors).
 *
 * The declarations moved out of `Network/network_transport.h` when `Network/network_transport.cpp` was split
 * into one unit per translation unit (docs/network-transport-split.md): a declaration belongs with the TU that
 * defines the symbol (rule 2).  The types they use are `Network/network_transport_types.h`'s.
 */

#ifndef NETWORK_NETWORK_SESSION_STABLE_H
#define NETWORK_NETWORK_SESSION_STABLE_H

#include "Network/network_transport_types.h"

extern "C" {

/* untyped: opaque handle passed through - the object's layout belongs to the writer band */
void* networkSmallObject_destroy(void* self, s32 flags);
/* untyped: opaque handle passed through - the object's layout belongs to the writer band */
void* networkSmallObject_init(void* self);
void networkPeer_resetSlots(NetworkSessionBase* self);
NetworkPeerOwner* dtor_803CF8F4(NetworkPeerOwner* self, s32 flags);
NetworkPeerBase* dtor_803D14C0(NetworkPeerBase* self, s32 flags);
s32 NetworkSessionStable_getOwnIndex(NetworkSessionStable* self);
void NetworkSessionStable_sendStream(NetworkSessionStable* self, NetworkStreamWriter* stream,
                                     u32 a, u32 b, const void* term, u32 c); /* untyped: byte range (the terminator) */
u32 networkSessionNonce_generate(void);
s32 networkSessionNonce_isValid(u32 nonce);
}

#endif /* NETWORK_NETWORK_SESSION_STABLE_H */
