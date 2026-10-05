/* ef/eft_slot.cpp's header: the declarations the unit's consumers need, and the one call-site
 * declaration the unit itself cannot take from its owner's header.
 *
 * RULE 2 HOMES.  `enemy_data_find`/`enemy_data_grp`/`eft_slot_effect_key` are defined by
 * `ef/eft_slot.cpp` (map 0x803438E4 / 0x803439D4 / 0x8034539C) and called from the enemy band -
 * `enemy/fn_8013BE60.c`, `enemy/fn_80165FC8.cpp`, `enemy/fn_80170600.cpp`, `enemy/em_action.cpp`
 * (20+ call sites read the returned record as `_ENEMY_DATA`, `include/enemy/ENEMY_DATA.h`).  This
 * unit owns the addresses, so this is their home: the consumers include this header and keep no
 * declaration of their own.  `struct EftSlot` is this unit's own view of the 0x3C-byte record it
 * hands back, forward-declared so a caller with its own view (`_ENEMY_DATA`) can pass its pointer
 * meaning what this unit means; the type itself is defined in `include/ef/EftSlot.h`.
 *
 * `eft_net_send` (0x803386C4, size 0x144) is the ef band's slot-state sender: r3 the slot record, r4 the
 * mode, r5 the value the mode carries.  `ef/eft_slot.cpp` calls it nine times.  `hud/net_char_sync.cpp` owns
 * the address (its band is 0x80334568..0x80338808); the declaration is repeated here with the owner's own
 * signature, so this unit does not include `hud/net_char_sync.h`, which drags in the whole net message
 * family (an earlier probe measured that the wider declaration surface moved `eft_slot_work_update`).
 *
 * `copyVec3` itself (0x80041E40, `src/mh3_pad.cpp`) is no longer one of these: it comes from
 * `include/mh3_pad.h`, which this unit includes (the `(10197)` clash that used to make that header
 * unreachable is closed).
 */
#ifndef MHTRI_EF_EFT_SLOT_H
#define MHTRI_EF_EFT_SLOT_H

#include "types.h"
#include "hud/eft_net_send.h" /* the owner's leaf header (rule 2) */

#ifdef __cplusplus
extern "C" {
#endif

/* This unit's own view of the record its table hands back (the enemy band's is `_ENEMY_DATA`); the definition is `ef/EftSlot.h`. */
struct EftSlot;

/* The 10-entry table lookup: r3 the group index `enemy_data_grp` produced, r4 the enemy-data id;
 * returns the entry, or NULL when the table has none.  Its callers read the record's `+0x08` flag
 * byte and its `+0x17` key byte (both named in `include/enemy/ENEMY_DATA.h`). */
void* enemy_data_find(u8 grp, u8 id);

/* The group index that lookup is keyed on, from an enemy's kind byte and its variant. */
u8 enemy_data_grp(u8 kind, u8 variant);

/* The effect key the family's spawner stamps for an entry (its `+0x10` byte in mode 3, else `+0x14`).
 * The record is passed as this unit's `EftSlot` view - a caller holding an `_ENEMY_DATA*` casts. */
u8 eft_slot_effect_key(struct EftSlot* slot);

/* 0x80346268 - the two-argument request `enemy/em_action.cpp`'s entry action's case 11 makes when
 * the enemy data entry is latched.  The address is in this unit's range and its body is still
 * unwritten, so the declaration keeps the caller's view (`r3`/`r4`, no result). */
void fn_80346268(u32 a, u32 b);


struct _ENEMY_WORK;
u32 eft_slot_armed_ck(struct EftSlot* slot);
u32 eft_slot_persist_ck(struct EftSlot* slot);
void eft_slot_kind_set(struct EftSlot* slot, u8 key, u8 force);
void eft_slot_state_set(struct EftSlot* slot, u8 state, struct _ENEMY_WORK* work, u8 index);
/* 0x803461EC - stores the two marks a received mark message carries on the slot. */
void eft_slot_marks_set(struct EftSlot* slot, u8 a, u8 b);

/* 0x80349914 - the two-byte copy MWCC emits for a `u16` pair assignment (`*dst = *src`).  Added with
 * `menu/menu_row.cpp`, its consumer (rule 2: the address is in this unit's range). */
void fn_80349914(u16* dst, const u16* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_SLOT_H */
