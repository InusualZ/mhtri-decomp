/*
 * include/Network/NetworkSocketWii.h - the free functions of `src/Network/NetworkSocketWii.cpp` (`.text` 0x803F7538..0x803F84B8): the Wii socket constructor and the socket handle accessors.
 * Moved here from `Network/NetworkCommunityPat.h` when the network pilot round 3 recut gave the range its own unit
 * (docs/plan.md 6.5 rule 2: the owner declares).
 */
#ifndef MHTRI_NETWORK_NETWORKSOCKETWII_H
#define MHTRI_NETWORK_NETWORKSOCKETWII_H

#include "types.h"

class NetworkSocketHandle;

#ifdef __cplusplus
extern "C" {
#endif

/* The socket reader `network_socket_streams.cpp` polls (moved here from `Network/network_transport_types.h`). */
/* untyped: opaque handle passed through - the socket object belongs to another band */
s32 getBytesAvailableToRead(void* handle);

/* 0x803F7540 - the last error code the socket handle recorded (its +0x08 word). */
s32 networkSocketHandle_getLastError(NetworkSocketHandle* handle);

/* 0x803F7548 - construct the 0x24-byte Wii socket object `sNetworkLibraryWii::createSocket` allocates:
 * the base constructor, its own table (0x805FC984), descriptor -1 at +0x0C and the rest cleared.  GUESS
 * on the name (the retail symbol is the class's constructor; the class is not reconstructed yet). */
NetworkSocketHandle* constructNetworkSocket(NetworkSocketHandle* socket);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORKSOCKETWII_H */
