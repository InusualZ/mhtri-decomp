/*
 * Declarations owned by `Pl/fn_8027D684.cpp` (docs/plan.md 6.5 rule 2): `fn_8027D738` is defined in
 * that source (`src/Pl/fn_8027D684.cpp:132`) and lives at 0x8027D738, inside the unit's `.text`
 * 0x8027D684-0x802840DC, so its declaration belongs in that unit's header and every consumer includes
 * it.  Added with `menu/menu_item.cpp`, which calls it through `fn_802A5444`/`fn_802A579C`.
 */
#ifndef MHTRI_PL_FN_8027D684_H
#define MHTRI_PL_FN_8027D684_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027D738 - the player work record's "ready" byte (`_PLW` +0x268, gated by +0x655's high bit),
 * read as the boolean the menu's item pick waits on.  The map name is a plain `fn_XXXXXXXX` stem, so
 * the owner defines it `extern "C"` and it is declared at C linkage here.  `u32`, and the owner
 * defines it `u32` too: the item menu's caller compares the result unsigned (`bl fn_8027D738;
 * cmplwi r3,0x1` at 0x802A008C), and an `s32` declaration there compiles to `cmpwi` and costs that
 * function 1.28 points. */
u32 fn_8027D738(struct _PLW* self);

/* 0x8027DFD0 - the player's +0x460 timer as a boolean (> 0). */
s32 Pl_timer_0x460_ck(struct _PLW* self);

/* 0x8027D76C - the owner's own action-complete hook (`fn_8027D684.cpp:147`, `extern "C" void`), the
 * call the AI band makes once a motion resolves.  Added with `ai/fn_802C474C.cpp`, its first consumer
 * (docs/plan.md 6.5 rule 2). */
void fn_8027D76C(struct _PLW* self);

/* 0x8027FF88 - classify one packed equipment word (`_EQUIP_INDEX` +0x00) and return the number of
 * colour/variant forms it has (0 = none).  Read by the equipment-information screen
 * (`menu/menu_infomation.cpp`), whose header is the first consumer. */
u8 fn_8027FF88(u32 equip);

/* 0x8027F11C - the equipment "category weight" of one piece: `lbz` of its first byte, then a
 * 0/1/6/0xB..0xF switch (`NULL` maps to 0).  `fn_8027ECAC` / `fn_8027FFFC` are the two sub-record
 * accessors the same screen reads to classify a piece: the `_EQUIP` sub-record (NULL when absent)
 * and the piece's "filled" flag.  All three are owned by this unit and were added with
 * `menu/menu_infomation.cpp`, their first consumer (docs/plan.md 6.5 rule 2). */
struct _EQUIP;
u8 fn_8027F11C(void* equip);
void* fn_8027ECAC(struct _EQUIP* equip);
u32 fn_8027FFFC(struct _EQUIP* equip);

/* 0x8027DC64 - the player's slot-occupied flag (`include/unsplit/Pl.h`'s +0x00 byte, the first byte
 * of the record at 0x806BB7A0), read as a boolean.  `enemy/em020_handlers.cpp`'s `em020_condition_ck`
 * mode 2 compares it against 1, so it is declared here - the owner defines it `extern "C"` at
 * `src/Pl/fn_8027D684.cpp:166` (rule 2: this range owns the address).  `u32`, not the owner's `s32`:
 * the target's caller compares the result unsigned (`bl fn_8027DC64; cmplwi r3,0x1`), and an `s32`
 * declaration compiles to `cmpwi` there and costs that function 1.28 points - the same finding as
 * the `fn_8027D738` declaration above. */
u32 fn_8027DC64(void);

/* 0x8027E72C - the colour table's `index`-th 3-byte row as an opaque-alpha RGBA word; 0x8027E7BC - the
 * colour-table index the `_EQUIP` record's kind-row stores at +0x05 (0 when the row is absent);
 * 0x8027EFB4 - the equipment kind's row-table class (0 for kinds 1-6, 1 for 7-11 and 14-15, 2 for
 * 12-13, 0xFF otherwise).  Added with `quest/arenatask.cpp`'s `arena_equip_color_set` (rule 2). */
struct _EQUIP;
s32 equip_color_rgba_get(u8 index);
u8 equip_color_index_get(struct _EQUIP* equip);
s32 equip_kind_table_class(u8 kind);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* C++-linkage declarations: the map's mangled name is the C++ front-end's own spelling of these
 * (rule 9), so they are declared outside the `extern "C"` block above. */

/* 0x8027E9F0 - the equipment piece's display name (its kind picks the string table, its id the
 * row).  Added with `menu/menu_infomation.cpp`, its consumer: the call site and the target both spell
 * it `GetEquipName__FUcUs`, so it must not be reached through an `extern "C"` declaration. */
u32 GetEquipName(u8 kind, u16 id);

/* 0x8027E7F0 - whether the piece's kind allows recolouring (1 when it does); the arena's colour pack
 * gates on it.  Added with `quest/arenatask.cpp` (rule 2). */
u32 EnableChangeColor(struct _EQUIP* equip);
#endif

#endif /* MHTRI_PL_FN_8027D684_H */
