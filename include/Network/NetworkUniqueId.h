/*
 * include/Network/NetworkUniqueId.h - the free functions of `src/Network/NetworkUniqueId.cpp` (`.text` 0x803F84B8..0x803F89CC): the address-object (unique id) helpers.
 * Moved here from `Network/NetworkCommunityPat.h` when the network pilot round 3 recut gave the range its own unit
 * (docs/plan.md 6.5 rule 2: the owner declares).
 */
#ifndef MHTRI_NETWORK_NETWORKUNIQUEID_H
#define MHTRI_NETWORK_NETWORKUNIQUEID_H

#include "types.h"

struct NetworkSmallObject;

#ifdef __cplusplus
extern "C" {
#endif

/* untyped: opaque handle passed through - only the writer band owns the layout */
void networkSmallObject_construct(void* self);

/* 0x803F8830 - sets the address object from `size` raw bytes of address kind `kind` (GUESS on the name). */
void networkSmallObject_setAddress(NetworkSmallObject* self, u8 kind, const u8* data, u32 size);

/* 0x803F8810 - true when the address object holds a valid address (GUESS on the name). */
s32 networkSmallObject_isValid(const NetworkSmallObject* self);

/* 0x803F88B0 - writes the address object's raw address into `out` (at most `size` bytes). */
void exportTo(const NetworkSmallObject* self, u8* out, u32 size);

/* 0x803F8904 - true when both address records are set and equal. */
u32 networkSmallObject_isEqual(const NetworkSmallObject* a, const NetworkSmallObject* b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORKUNIQUEID_H */
