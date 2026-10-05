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

/* 0x8004AC6C - takes a received community profile back into the user data; the sibling of `buildNetUserProfile`
 * (GUESS name). */
void applyNetUserProfile(const u8* profile);

/* 0x8004A7C4 / 0x8004A8DC / 0x8004A960 / 0x8004A9B8 - refresh parts of the community profile from the save, 0 when
 * there is no user data, else 1 (GUESS names from the bytes each copies and from their callers, the
 * `Network/net_session_close.cpp` profile writes): the head fields (+0x02..+0x7B), the bytes +0x7C..+0x94, the rank
 * and level bytes (+0xF2/+0xF3, then `applyNetUserProfile`) and the 0x56-byte mediator record (+0x9C). */
s32 fillNetUserProfileHead(struct NetUserProfile* profile);
s32 fillNetUserProfileRange7C(struct NetUserProfile* profile);
s32 fillNetUserProfileRank(struct NetUserProfile* profile);
s32 fillNetUserProfileRecord(struct NetUserProfile* profile);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_ITEM_H */
