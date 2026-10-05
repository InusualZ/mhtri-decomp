/*
 * Network/NetworkResolverWii.h - the lookup thread's entry and the lookup it runs (`Network/NetworkResolverWii.cpp`);
 *   the types are `Network/network_transport_types.h`'s.
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
