/*
 * Declarations of `Pl/pl_act.cpp`'s equipment section (0x8027D684-0x802840DC) for its menu, quest, AI and enemy
 * consumers.
 */
#ifndef MHTRI_PL_FN_8027D684_H
#define MHTRI_PL_FN_8027D684_H

#include "types.h"

struct _PLW;

/* The blademaster weapon row `equip_blade_rec_get` hands back (the 0x18-byte stride `equip_blade_rec_find` indexes kinds 7-11/14-15
 * with).  The element pairs are read by `menu/menu_infomation.cpp` (`weapon_element_compare`, `weapon_status_compare`): the type is
 * shown as `type + 6`, a negative value only under skill 201.  `slot_count` is a GUESS: the byte `weapon_slot_count_get`
 * hands to the 2061-sprite putter.  size: 0x18 (the row stride) */
typedef struct PlSkillRec {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ u8 level_a;          /* scaled by 0x32 and biased by 0x96 into `+0x56E` (`pl_weapon_record_read`) */
    /* +0x09 */ u8 level_b;          /* the sibling level the same function scales */
    /* +0x0A */ u8 pad_0x0A[0x2];
    /* +0x0C */ u8 slot_count;       /* non-zero for the pieces `weapon_slot_count_get` reports */
    /* +0x0D */ s8 value;            /* the value `pl_weapon_class_refresh` stores into `+0x570` */
    /* +0x0E */ u8 element_a_type;   /* 0 = none */
    /* +0x0F */ s8 element_a_value;  /* negative = hidden */
    /* +0x10 */ u8 element_b_type;   /* 0 = none */
    /* +0x11 */ s8 element_b_value;  /* negative = hidden */
    /* +0x12 */ u8 pad_0x12[0x1];
    /* +0x13 */ u8 phial_type;       /* a switch axe's phial (string table 0x2F's row); GUESS, from the info screen */
    /* +0x14 */ u8 pad_0x14[0x4];
} PlSkillRec; /* size: 0x18 */

/* The weapon/skill record an equipment slot resolves to (the 0x24-byte sibling table). size: 0x24 */
typedef struct PlWeaponRec {
    /* +0x00 */ u8 pad_0x00[0x6];
    /* +0x06 */ u8 bonus_0x06;   /* the mode-0 points `pl_weapon_stat_get` reads through `fn_8027E708` */
    /* +0x07 */ s8 bonus_0x07;   /* the points `pl_gun_recoil_get` accumulates */
    /* +0x08 */ u8 defense_bonus; /* the defence a weapon adds (the info screen sums a bowgun's three parts) */
    /* +0x09 */ s8 bonus_0x09;   /* the points `pl_gun_reload_get` accumulates */
    /* +0x0A */ s16 bonus_0x0A;  /* the bonus `pl_gun_deviation_get` scales by 1/100 */
    /* +0x0C */ u8 pad_0x0C[0x4];
    /* +0x10 */ u8 recoil_kind;   /* the row of the info screen's recoil tables (a frame's rating and icon, a stock's) */
    /* +0x11 */ u8 pad_0x11[0x1];
    /* +0x12 */ s8 bonus_0x12;   /* the points `pl_weapon_affinity_get`/`pl_weapon_class_refresh` add up */
    /* +0x13 */ u8 pad_0x13[0x2];
    /* +0x15 */ u8 bonus_flag_0x15; /* non-zero: a gun slot's bonus applies (`equip_gun_bonus_ck`) */
    /* +0x16 */ u8 pad_0x16[0xE];
} PlWeaponRec; /* size: 0x24 */

/* The per-slot record `equip_slot_rec_get` hands back (the 0x18-byte row `equip_slot_rec_find` indexes); `fn_80273ED8` sums one
 * signed byte per act kind out of it. size: 0x18 (the row stride) */
typedef struct PlSlotRec {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u8 dye_mode;      /* 2: the piece can always be recoloured (`EnableChangeColor`) */
    /* +0x05 */ u8 color_index;   /* the colour-table row `equip_color_index_get` hands back */
    /* +0x06 */ u8 wear_flags;    /* bit 2: a blademaster can wear it, bit 3: a gunner (the info screen's wear check) */
    /* +0x07 */ u8 pad_0x07[0x1];
    /* +0x08 */ s8 value_0x08;
    /* +0x09 */ s8 value_0x09;
    /* +0x0A */ s8 value_0x0A;
    /* +0x0B */ s8 value_0x0B;
    /* +0x0C */ s8 value_0x0C;
    /* +0x0D */ u8 pad_0x0D[0xB];
} PlSlotRec; /* size: 0x18 */

/* An RGBA colour built byte by byte (red first, alpha last) and handed back as one register word (the colour getters build it; `menu/menu_infomation.cpp` reads it back by channel). size: 0x4 */
typedef union PlRgba {
    /* +0x0 */ u8 channel[4];
    /* +0x0 */ u32 word;
} PlRgba; /* size: 0x4 */

/* The hunter's status sheet `pl_status_sheet_get` (0x80270F50) fills from the player work (+0x372..+0x3D4) after
 * applying a column array of equipment: what the equipment-information screen's status blocks print.
 * size: 0x22 */
typedef struct PlStatusSheet {
    /* +0x00 */ s16 health;
    /* +0x02 */ s16 stamina;        /* in sixths: the screen prints it divided by 6 */
    /* +0x04 */ s16 affinity;       /* GUESS, from the row the screen prints it in */
    /* +0x06 */ u16 attack;         /* the raw value; the screen scales it by the weapon class */
    /* +0x08 */ u16 defense;
    /* +0x0A */ u8 element_type;    /* 1.. (0 = none) */
    /* +0x0B */ u8 status_type;     /* 12.. (0 = none) */
    /* +0x0C */ s16 element_value;
    /* +0x0E */ s16 status_value;
    /* +0x10 */ s16 resist[5];      /* fire, water, ice, thunder, dragon (the screen's row order is 0, 1, 3, 2, 4) */
    /* +0x1A */ u8 cleared_0x1A[7]; /* zeroed by the getter, read by nobody here */
    /* +0x21 */ u8 pad_0x21;
} PlStatusSheet; /* size: 0x22 */

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

struct _EQUIP;

/* 0x8027FF88 - a blade's colour tier: the number of colour forms its row (+0x16, in steps of five) gives, 0 for
 * none; read by the equipment-information screen (`menu/menu_infomation.cpp`) on the shown piece. */
u8 equip_color_tier_get(struct _EQUIP* equip);

/* 0x8027F11C - a piece's decoration slot count: `lbz` of its first byte, then a 0/1/6/0xB..0xF switch (`NULL`
 * maps to 0).  `equip_blade_rec_get` / `equip_gun_bonus_ck` are the two sub-record accessors the same screen
 * reads to classify a piece: the `_EQUIP` sub-record (NULL when absent) and, for a kind-11/12 gun piece, the
 * +0x15 flag of the weapon record it resolves to (1 when set). */
u8 equip_deco_slots_get(struct _EQUIP* equip);
PlSkillRec* equip_blade_rec_get(struct _EQUIP* equip);
u32 equip_gun_bonus_ck(struct _EQUIP* equip);

/* 0x8027DC64 - the player's slot-occupied flag (the first byte of the record at 0x806BB7A0), read as a boolean;
 * `enemy/em020_handlers.cpp`'s `em020_condition_ck` mode 2 compares it against 1.  `u32`, not the definition's
 * `s32`: the caller compares the result unsigned (`bl pl_work_flag0_clear_ck; cmplwi r3,0x1`). */
u32 pl_work_flag0_clear_ck(void);

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
