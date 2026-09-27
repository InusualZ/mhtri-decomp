/*
 * include/Network/network_state.h - the two NetworkSessionManager entry points `Network/network_state.cpp`
 * owns, declared here so consumers include the owner's header (docs/plan.md 6.5 rule 2).
 *
 * Both were declared in `include/unsplit/Network.h` until the worker/net-capcom batch registered the
 * 0x803FE8E4..0x804006A8 range, which made them owned by `src/Network/network_state.cpp`.  The
 * registration-boundary rule-2 check (`land.py band_ownership_warnings`) flags a band header that still
 * declares a newly-owned symbol, so they moved here.
 *
 * `sendReqShut` (0x803FFDCC, opcode 0x04) and `resetNetworkState3` (0x803FE924) are the two the
 * GameSpy band (`Network/0x8041A87C.cpp`) calls; every other symbol the range owns is declared and
 * defined in `src/Network/network_state.cpp` itself.  The `NetworkInstance` layout lives in
 * `include/unsplit/Network.h` (a shared type, not an owned symbol).
 */
#ifndef NETWORK_STATE_H
#define NETWORK_STATE_H

#include "types.h"
#include "unsplit/Network.h"

extern "C" {

void sendReqShut(NetworkInstance* self, s32 mode);
s32  resetNetworkState3(NetworkInstance* self);

}

#endif /* NETWORK_STATE_H */
