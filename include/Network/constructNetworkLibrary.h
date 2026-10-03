/* Declarations owned by `src/Network/constructNetworkLibrary.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_NETWORK_CONSTRUCTNETWORKLIBRARY_H
#define MHTRI_NETWORK_CONSTRUCTNETWORKLIBRARY_H

#include "types.h"
#include "Network/network_transport.h"

/* Declarations moved here from `include/unsplit/Network.h` (docs/plan.md 6.5 rule 2: the owner declares). */
struct NetworkPat;

#ifdef __cplusplus
extern "C" {
#endif

/* DWC/GameSpy session layer */
void constructNetworkLibrary(void);   /* 0x804189C8, the sNetworkLibrary constructor body the opener calls */

/* 0x80419C2C / 0x80419E1C - the Pat holder reset and the session-manager slot delete. */
void clearNetworkPat(struct NetworkPat* holder);

void deleteNetworkSessionManagerPat(struct NetworkPat* holder, s32 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_CONSTRUCTNETWORKLIBRARY_H */
