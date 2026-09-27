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
 * meaning what this unit means; the type itself is defined in `src/ef/eft_slot.cpp`.
 *
 * `fn_803386C4` (0x803386C4, size 0x144) is the ef band's slot-state setter: r3 the slot record, r4
 * the mode, r5 the value the mode carries.  `ef/eft_slot.cpp` calls it nine times.  main's
 * registration owns the address in `hud/fn_80334568.cpp` (its band is 0x80334568..0x80338808, so
 * 0x803386C4 is that unit's tail function), and its header is the declaration's rule-2 home.
 *
 * It cannot be included from here, twice over - measured, not assumed:
 *   * `hud/fn_80334568.h` pulls `enemy.h`, whose `_ENEMY_WORK` copy collides with
 *     `enemy/ENEMY_WORK.h`'s, which this unit's bodies need (`(10296) class redefined`); the two
 *     copies are main's parked rule-1 fold;
 *   * including it anyway (with that fold bypassed) perturbs this unit's codegen: the header's
 *     declaration surface changed `eft_slot_work_update` from 92.87 to 90.24, so the two views are not
 *     interchangeable for this unit's call sites.
 *
 * So this is the `fn_80041E40` pattern (`include/hud/fn_80334568.h`'s own note): the consumer keeps
 * the call-site spelling until the owner's header is reachable.  The declaration below MUST stay
 * identical to the owner's; the fold is a `shared-file` request in this unit's outbox.
 */
#ifndef MHTRI_EF_EFT_SLOT_H
#define MHTRI_EF_EFT_SLOT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This unit's own view of the record its table hands back (the enemy band's is `_ENEMY_DATA`). */
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

void fn_803386C4(void* slot, u32 mode, u32 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_SLOT_H */
