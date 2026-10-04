/*
 * Declarations for the symbols `src/userdata_item.cpp` owns that other units call (docs/plan.md 6.5, rule 2).
 */
#ifndef MHTRI_USERDATA_ITEM_H
#define MHTRI_USERDATA_ITEM_H

#include "types.h"

struct NetUserProfile;   /* include/Network/net_session_close.h */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004A5AC - clears the 0x100-byte community profile and fills it from the save's user data (0 when there
 * is none).  GUESS name: the body copies the user record's bytes, names and equipment type into the profile
 * `Network/net_session_close.cpp` hands the community layer. */
s32 buildNetUserProfile(struct NetUserProfile* profile);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_ITEM_H */
