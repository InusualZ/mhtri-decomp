/*
 * Network/NetworkResolverWii.h - the symbols `Network/NetworkResolverWii.cpp` owns that the rest of the Network band calls
 * (the lookup thread's entry and the lookup it runs).
 *
 * The declarations moved out of `Network/network_transport.h` when `Network/network_transport.cpp` was split
 * into one unit per translation unit (docs/network-transport-split.md): a declaration belongs with the TU that
 * defines the symbol (rule 2).  The types they use are `Network/network_transport_types.h`'s.
 */

#ifndef NETWORK_NETWORK_RESOLVER_WII_H
#define NETWORK_NETWORK_RESOLVER_WII_H

#include "Network/network_transport_types.h"

extern "C" {

/* untyped: opaque handle passed through - the OS thread entry receives and returns a plain pointer */
void* networkResolver_threadEntry(void* self);
void networkResolver_lookup(NetworkResolverWii* self);
}

#endif /* NETWORK_NETWORK_RESOLVER_WII_H */
