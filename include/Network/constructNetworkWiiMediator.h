/* Declarations owned by `src/Network/constructNetworkWiiMediator.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_NETWORK_CONSTRUCTNETWORKWIIMEDIATOR_H
#define MHTRI_NETWORK_CONSTRUCTNETWORKWIIMEDIATOR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80418988 - creates the Wii network library and publishes it as `sNetworkLibrary::mpInstance`. */
void constructNetworkWiiMediator(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_CONSTRUCTNETWORKWIIMEDIATOR_H */
