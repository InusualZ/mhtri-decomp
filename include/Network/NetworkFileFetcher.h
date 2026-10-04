/*
 * include/Network/NetworkFileFetcher.h - the free functions of `src/Network/NetworkFileFetcher.cpp` (`.text` 0x803F6458..0x803F7538): the kind-2 fetcher constructor.
 * Moved here from `Network/NetworkCommunityPat.h` when the network pilot round 3 recut gave the range its own unit
 * (docs/plan.md 6.5 rule 2: the owner declares).
 */
#ifndef MHTRI_NETWORK_NETWORKFILEFETCHER_H
#define MHTRI_NETWORK_NETWORKFILEFETCHER_H

#include "types.h"

class NetworkFileFetcher;          /* include/Network/network_pat_control.h */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803F73C0 - construct the 0x10-byte fetcher `sNetworkLibraryWii::createFetcher` builds for kind 2:
 * the base constructor, then its own table (0x805FC8A8).  GUESS on the name, from that kind. */
NetworkFileFetcher* constructNetworkFetcherKind2(NetworkFileFetcher* fetcher);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORKFILEFETCHER_H */
