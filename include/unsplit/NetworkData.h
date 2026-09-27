/*
 * Network constants with no registered owner (docs/plan.md 6.5 rule 2) - the DATA half of the
 * Network band header.
 *
 * `include/unsplit/Network.h` is the band's code half, and it is C++-only (it carries
 * `class NetworkLogger` and the `<module>/<unit>.h` includes its callers need).  Two things make a
 * second, data-only header necessary rather than convenient: the DWCi `.c` units cannot include a
 * C++ header at all, and the session unit's own header declares `dtor_803CA338(void* self)` while
 * `Network.h` declares the same address `dtor_803CA338(void* self, s32 flags)` - each matching that
 * file's own call site - so a unit that includes both headers fails with `(10197) illegal function
 * overloading`.  The constants below are what the session units load, so they live here where every
 * one of them can reach them.
 *
 * Every address is outside a registered range (the `.sdata`/`.sdata2`/`.rodata`/`.sbss` bands around
 * 0x803D/0x8057/0x805F/0x8079 hold no registered unit), so no owner header exists; declared, never
 * defined (playbook 29).
 *
 * Added with the networking conformance pass.
 */
#ifndef MHTRI_UNSPLIT_NETWORKDATA_H
#define MHTRI_UNSPLIT_NETWORKDATA_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80793930 / 0x80793934 - the two `.sdata` floats `initNetworkSessionStable` copies into a new
 * session through the vtable's +0x54/+0x58 slots; 0x8079C764 the `.sdata2` float it publishes
 * through +0x60. */
extern f32 networkSessionTimeoutSeconds;
extern f32 networkSessionIntervalSeconds;
extern f32 networkSessionPeriodSeconds;

/* 0x80572428 - the four-word all-zero `.rodata` descriptor the session initialises a request's
 * `desc_98/9C/A0` from (the bytes are 16 x 0x00). */
extern u32 NetworkRequest_defaultDescriptor[4];

/* 0x80794CA0 - the request-id source: `requestId_70 = counter; counter = requestId_70 + 1`. */
extern u32 NetworkRequest_idCounter;

/* 0x80794CC0 - the `.sbss` mediator singleton slot the `constructNetworkWiiMediator` constructor
 * publishes into. */
extern void* sNetworkWiiMediatorInstance;

/* The `.data` messages the session band logs, read off the DOL - they name their own emitters, which
 * is how the two functions that load them were identified:
 *   0x805FA8C8 = "NetworkSessionManager::deleteRequest: request is moving.
"
 *   0x805FAAD0 = "NetworkRequest::getArgument: arg no over %d <= %d
" */
extern const char NetworkSessionManager_deleteRequestMessage[0x3A];
extern const char NetworkRequest_getArgumentMessage[0x33];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_NETWORKDATA_H */
