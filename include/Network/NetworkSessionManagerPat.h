/*
 * Declarations of `src/Network/NetworkSessionManagerPat.cpp` that are not class members: the two free functions inside the
 * unit's `.text` range (0x803D70B8..0x803E44C8).  The class itself is declared in `Network/NetworkSessionManager.h`.  Moved here
 * from `include/Network/network_pat_control.h` when the phase 4 fold gave the unit the whole band.
 */
#ifndef MHTRI_NETWORK_NETWORKSESSIONMANAGERPAT_H
#define MHTRI_NETWORK_NETWORKSESSIONMANAGERPAT_H

#include "types.h"
#include "Network/NetworkPat.h"

class NetworkSessionManagerPat;

#ifdef __cplusplus
extern "C" {
#endif

/* The holder `getPatsObject` returns is `NetworkPat` (include/Network/NetworkPat.h).  The accessor is at 0x803DA020. */
NetworkPat* getPatsObject(void);
/* `isNetworkSessionManagerPatReady` (0x803DF1A8) is the session manager's readiness probe: it is handed the object slot
 * +0x00 holds, so its parameter is that object's class. */
BOOL isNetworkSessionManagerPatReady(NetworkSessionManagerPat* session_manager);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORKSESSIONMANAGERPAT_H */
