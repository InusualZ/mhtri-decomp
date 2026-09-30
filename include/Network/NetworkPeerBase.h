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
#include "Network/network_writer_types.h"

/* The small transport object seen as a stream sink (GUESS on the name: retail's small object is the writer band's
   `NetworkSmallObject`, a 0x10-byte record with the sink's layout).  Its deleting destructor is the
   compiler-emitted `__dt__22NetworkSmallObjectSinkFv`: it runs the sink's destructor with in-charge flag 0, frees
   on request and stores no table (the key function `clear` is defined in the writer band, so none is emitted). */
class NetworkSmallObjectSink : public NetworkStreamSink {
public:
    virtual void clear();
    virtual ~NetworkSmallObjectSink();

    /* Runs the destructor on a small object a caller keeps on its stack or inside a record (no delete).  Inside a
       `#pragma dont_inline on` region the helper stays a `bl`, so those callers spell the qualified call out. */
    static void destroy(NetworkSmallObject* object)
    {
        ((NetworkSmallObjectSink*)object)->NetworkSmallObjectSink::~NetworkSmallObjectSink();
    }
};

extern "C" {

void networkPeerError_get(NetworkPeerBase* self, NetworkPeerErrorRecord* out);
void networkPeerError_clear(NetworkPeerBase* self);
/* untyped: opaque handle passed through - the callers hand the peer they already hold */
void networkPeerError_set(void* self, const void* source, u32 argument, s32 code);
}

#endif /* NETWORK_NetworkPeerBase_H */
