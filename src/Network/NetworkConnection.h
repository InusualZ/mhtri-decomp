/*
 * Network/NetworkConnection.h - the declarations of `Network/NetworkConnection.cpp`: the member mutex pair the network
 *   records embed.
 */
#ifndef MHTRI_NETWORK_NETWORKCONNECTION_H
#define MHTRI_NETWORK_NETWORKCONNECTION_H

#include "types.h"

extern "C" {

/* 0x803CA338 / 0x803CA37C - destroy (flags -1 at every call site) and construct the member mutex the network
 * records embed (the constructor references the 0x10-byte .data 0x805F91E0).  The callers hand a byte block
 * whose layout only this unit's class owns. */
/* untyped: opaque handle passed through - only the writer band owns the layout */
void networkInstance_destroyMutex(void* self, s32 flags);
/* untyped: opaque handle passed through - only the writer band owns the layout */
void networkInstance_initMutex(void* self);

}

#endif /* MHTRI_NETWORK_NETWORKCONNECTION_H */
