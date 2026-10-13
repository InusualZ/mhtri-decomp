/*
 * Declarations of `Pl/pl_act.cpp`'s equipment section (0x8027D684-0x802840DC) for its menu, quest, AI and enemy
 * consumers.
 */
#ifndef MHTRI_PL_FN_8027D684_H
#define MHTRI_PL_FN_8027D684_H

#include "types.h"

struct _PLW;

/* The blademaster weapon row `fn_8027ECAC` hands back (the 0x18-byte stride `fn_8027EC50` indexes kinds 7-11/14-15
 * with).  The element pairs are read by `menu/menu_infomation.cpp` (`fn_8031622C`, `fn_803164D0`): the type is
 * shown as `type + 6`, a negative value only under skill 201.  `slot_count` is a GUESS: the byte `fn_80316978`
 * hands to the 2061-sprite putter.  size: 0x18 (the row stride) */
typedef struct PlSkillRec {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ u8 level_a;          /* scaled by 0x32 and biased by 0x96 into `+0x56E` (`fn_802740F4`) */
    /* +0x09 */ u8 level_b;          /* the sibling level the same function scales */
    /* +0x0A */ u8 pad_0x0A[0x2];
    /* +0x0C */ u8 slot_count;       /* non-zero for the pieces `fn_80316978` reports */
    /* +0x0D */ s8 value;            /* the value `fn_802744A0` stores into `+0x570` */
    /* +0x0E */ u8 element_a_type;   /* 0 = none */
    /* +0x0F */ s8 element_a_value;  /* negative = hidden */
    /* +0x10 */ u8 element_b_type;   /* 0 = none */
    /* +0x11 */ s8 element_b_value;  /* negative = hidden */
    /* +0x12 */ u8 pad_0x12[0x6];
} PlSkillRec; /* size: 0x18 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027D738 - the player work record's "ready" byte (`_PLW` +0x268, gated by +0x655's high bit), read as the
 * boolean the menu's item pick waits on.  `u32`: the item menu's caller compares the result unsigned
 * (`bl fn_8027D738; cmplwi r3,0x1` at 0x802A008C), which an `s32` declaration turns into `cmpwi`. */
u32 fn_8027D738(struct _PLW* self);

/* 0x8027DFD0 - the player's +0x460 timer as a boolean (> 0). */
s32 Pl_timer_0x460_ck(struct _PLW* self);

/* 0x8027D76C - the action-complete hook the AI band calls once a motion resolves. */
void fn_8027D76C(struct _PLW* self);

/* 0x8027FF88 - classify one packed equipment word (`_EQUIP_INDEX` +0x00) and return the number of
 * colour/variant forms it has (0 = none); read by the equipment-information screen (`menu/menu_infomation.cpp`). */
u8 fn_8027FF88(u32 equip);

/* 0x8027F11C - the equipment "category weight" of one piece: `lbz` of its first byte, then a
 * 0/1/6/0xB..0xF switch (`NULL` maps to 0).  `fn_8027ECAC` / `fn_8027FFFC` are the two sub-record
 * accessors the same screen reads to classify a piece: the `_EQUIP` sub-record (NULL when absent)
 * and, for a kind-11/12 gun piece, the +0x15 flag of the weapon record it resolves to (1 when set). */
struct _EQUIP;
u8 fn_8027F11C(void* equip);
PlSkillRec* fn_8027ECAC(struct _EQUIP* equip);
u32 fn_8027FFFC(struct _EQUIP* equip);

/* 0x8027DC64 - the player's slot-occupied flag (the first byte of the record at 0x806BB7A0), read as a boolean;
 * `enemy/em020_handlers.cpp`'s `em020_condition_ck` mode 2 compares it against 1.  `u32`, not the definition's
 * `s32`: the caller compares the result unsigned (`bl fn_8027DC64; cmplwi r3,0x1`). */
u32 fn_8027DC64(void);

/* 0x8027E72C - the colour table's `index`-th 3-byte row as an opaque-alpha RGBA word; 0x8027E7BC - the
 * colour-table index the `_EQUIP` record's kind-row stores at +0x05 (0 when the row is absent);
 * 0x8027EFB4 - the equipment kind's row-table class (0 for kinds 1-6, 1 for 7-11 and 14-15, 2 for
 * 12-13, 0xFF otherwise); `quest/arenatask.cpp`'s `arena_equip_color_set` reads them. */
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

/* 0x8027E9F0 - the equipment piece's display name (its kind picks the string table, its id the row); the map
 * spells it `GetEquipName__FUcUs`, so it is not reached through an `extern "C"` declaration. */
u32 GetEquipName(u8 kind, u16 id);

/* 0x8027E7F0 - whether the piece's kind allows recolouring (1 when it does); the arena's colour pack
 * gates on it. */
u32 EnableChangeColor(struct _EQUIP* equip);

/* 0x8027F0C4 - whether a bowgun piece takes options (the map spells it `Gunner_opt_ok_ck__FP6_EQUIP`). */
u32 Gunner_opt_ok_ck(struct _EQUIP* equip);
#endif

#endif /* MHTRI_PL_FN_8027D684_H */
