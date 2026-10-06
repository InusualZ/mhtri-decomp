/*
 * Declarations for the symbols `src/userdata_item.cpp` owns that other units call (docs/plan.md 6.5, rule 2).
 */
#ifndef MHTRI_USERDATA_ITEM_H
#define MHTRI_USERDATA_ITEM_H

#include "types.h"

struct NetUserProfile;   /* Network/net_session_close.h */
struct _EQUIP;           /* the 12-byte equipment record - Pl/plw.h */
struct Q_UserData;       /* the 0x6000-byte save block - quest/quest_types.h */
struct IdValue;          /* the (id, value) pair - id_value.h */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004A20C - copies one 12-byte equipment record field by field (two bytes, two halfwords, the word at +0x06,
 * the halfword at +0x0A); `dst` comes back in r3.  NAME (a GUESS in the scheme of its neighbour `item_pair_copy`):
 * its ~240 callers copy the player's, the lobby's and the community profile's equipment slots with it. */
struct _EQUIP* equip_record_copy(struct _EQUIP* dst, const struct _EQUIP* src);

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

/* 0x8004C004 - how many of the `count` pairs at `table` are empty (id 0).  GUESS name. */
u32 item_slots_free_count(const struct IdValue* table, s32 count);
/* 0x8004C29C - how many of the save's equipment box records (`userdata_box_capacity` of them) are empty.
 * GUESS name. */
u16 userdata_equip_box_free_count(void);
/* 0x8004C514 / 0x8004C5BC - files a new equipment record of `kind`/`item_id` (a copy of `equip`) into the
 * first empty slot of the save's equipment box; the slot index, or 0xFFFF when the box is full.  GUESS names. */
u16 userdata_equip_box_add_new(struct Q_UserData* user, u8 kind, u16 item_id);
u16 userdata_equip_box_add(struct Q_UserData* user, const struct _EQUIP* equip);
/* 0x8004BA3C - adds `count` of item `id` to an `{id, value}` table of `n` slots, capping each at `max`; returns 3 when
 * the table could not take all of it (the callers' reading). */
s16 item_take(u16 id, s16 count, struct IdValue* table, s32 n, s32 flag, s32 max);

/* 0x8004B0A4 - how many of item `id` the user holds in the pouch, the box and the stock, capped at 999.  GUESS. */
s32 userdata_item_count_total(u16 id, u8* userdata);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_ITEM_H */
