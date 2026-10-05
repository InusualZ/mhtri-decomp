/*
 * The hit registry's record types, shared by `Pl/pl_coll.cpp` (the registry helpers' head) and `menu/menu_item.cpp` (the registry
 * helpers' tail and the attack-entry accessors).  Phase 4 fold: the two units' headers each carried a view of
 * `_HIT_W` and of the registry head `lbl_806AC8A8`; one TU cannot hold both, so the views are merged here (rule 1) - where the two
 * views name the same offset, the names share an anonymous union.
 */
#ifndef MHTRI_PL_HIT_W_H
#define MHTRI_PL_HIT_W_H

#include "types.h"

struct _HIT_SIZE_DATA;

/* One entry of the global hit registry's linked lists (the map's `_HIT_W`, spelled by
 * `hit_flag_set__FP6_HIT_WUl` / `hit_result_check__FP6_HIT_W`): the record a live attack is registered as.  `+0x00` is the list
 * link (`hit_attack_list_push`/`hit_body_list_push` write the previous head here), `+0x05` is tested for zero before the push
 * (and is the "result valid" byte `hit_result_check` reports from), `+0x06`/`+0x07` select the owner's type twice over and
 * `+0x10`/`+0x14` are the two owner pointers - `hit_data_apply` stores the hit id at `+0x0C` and reads the owner's area byte
 * through `+0x10`.  The accessors of `menu/menu_item.cpp` read the state bits at `+0x0A`, the result id at `+0x0E`, four stored
 * values at `+0x18..+0x1E`, the flag word at `+0x20` and the mask bytes at `+0x31`/`+0x5B`.
 * size: 0x60 (lower bound - the highest offset the units touch is +0x5B) */
struct _HIT_W {
    /* +0x00 */ _HIT_W* next_0x00;
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ union {
        u8 active_0x05;
        u8 result_valid;       /* zero means the entry carries no result yet */
    };
    /* +0x06 */ u8 owner_kind_0x06;   /* 0 = player, 1 = enemy, 2 = ?, 3 = AI NPC (`fn_80299EF8`) */
    /* +0x07 */ u8 kind_0x07;
    /* +0x08 */ union {
        _HIT_SIZE_DATA* size_data_0x08;   /* the hit-box size record `shell_attack_set` picks */
        struct {
            u16 unused_0x08;
            u16 state;         /* the state bits `fn_8029F5E4`/`fn_8029F5F8` test */
        };
    };
    /* +0x0C */ u16 hit_id_0x0C;
    /* +0x0E */ u8 result;     /* the result id `hit_result_check` reports */
    /* +0x0F */ u8 unused_0x0F;
    /* +0x10 */ void* owner_0x10;
    /* +0x14 */ void* owner_0x14;
    /* +0x18 */ u16 field_0x018;      /* the four values `fn_8029F4C4` stores */
    /* +0x1A */ u16 field_0x01A;
    /* +0x1C */ u16 field_0x01C;
    /* +0x1E */ u16 field_0x01E;
    /* +0x20 */ u32 flags;            /* the hit flag word the setters OR/ANDC into */
    /* +0x24 */ u8 unused_0x24[0x31 - 0x24];
    /* +0x31 */ union {
        u8 attack_flags_0x31;  /* the attack-flag byte `shell_attack_set` stores (bit 4 is dropped for a weak enemy master) */
        u8 field_0x031;        /* the mask byte `fn_8029F57C` tests */
    };
    /* +0x32 */ u8 attack_kind_0x32;
    /* +0x33 */ u8 unused_0x33[0x38 - 0x33];
    /* +0x38 */ f32 power_0x38;          /* the attack record's first s16, as a float */
    /* +0x3C */ union {
        f32 knock_0x3C;        /* its second s16, as a float */
        f32 field_0x03C;       /* the s16 `hit_knock_set` widens and stores */
    };
    /* +0x40 */ s16 life_0x40;           /* the hit's remaining frames (scaled by an enemy master's rate) */
    /* +0x42 */ u8 unused_0x42[0x48 - 0x42];
    /* +0x48 */ u16 field_0x048;     /* the attack-set band (`Pl/pl_act.cpp`) stores a rate word here */
    /* +0x4A */ u8 field_0x04A;
    /* +0x4B */ u8 field_0x04B;
    /* +0x4C */ u8 field_0x04C;
    /* +0x4D */ u8 field_0x04D;
    /* +0x4E */ s8 field_0x04E;      /* the signed hit-strength byte `Pl_attack_set_sub` scales */
    /* +0x4F */ u8 unused_0x4F;
    /* +0x50 */ f32 value_0x50;      /* `hit_data_apply` stores `-100.0f` or a lowered byte here */
    /* +0x54 */ f32 value_0x54;      /* the same for the second owner slot */
    /* +0x58 */ u8 unused_0x58[0x5A - 0x58];
    /* +0x5A */ u8 field_0x05A;
    /* +0x5B */ u8 field_0x05B;      /* the mask byte `fn_8029F51C` tests */
    /* +0x5C */ u8 unused_0x5C[0x60 - 0x5C];
};

/* The hit registry itself (`.bss` `lbl_806AC8A8`, size 0x10): two singly-linked lists of live hits,
 * their counts and the running id `get_hit_id()` hands out.  `fn_8029F60C` reads the word at +0x04 as an item id.
 * size: 0x10 */
struct HitRegistry {
    /* +0x00 */ _HIT_W* head_0x00;
    /* +0x04 */ union {
        _HIT_W* head_0x04;
        u32 field_0x004;       /* the item id `fn_8029F60C` hands back */
    };
    /* +0x08 */ u16 count_0x08;
    /* +0x0A */ u16 count_0x0A;
    /* +0x0C */ u16 next_id_0x0C;   /* `get_hit_id` increments it and wraps at 0xEA60 */
    /* +0x0E */ u16 unused_0x0E;
};

#endif /* MHTRI_PL_HIT_W_H */
