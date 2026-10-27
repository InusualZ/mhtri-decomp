/*
 * The hit registry's record types, shared by `Pl/pl_coll.cpp`, `Pl/pl_body_push.cpp` and `Pl/pl_hit.cpp` (the registry
 * and the hit-entry accessors), rule 1.  Where the two units name the same offset differently,
 * the names share an anonymous union.
 */
#ifndef MHTRI_PL_HIT_W_H
#define MHTRI_PL_HIT_W_H

#include "types.h"
#include "nw4r/math.h"

/* One hit-size record of an attack (a run of them ends with joint -1; `_HIT_W::size_data_0x08` points at the run): the
 * joint it sits on (1000: the owner's position, 1001: the owner's own body capsule), the capsule's second joint, its shape
 * (0 sphere, 1 capsule on one joint, 2 capsule across two joints, 3 a capsule turned by the offset's angles), the part
 * `em_flags836_ck` tests, the radius and the two offsets (`hit_shape_get`).  size: 0x28 */
struct _HIT_SIZE_DATA {
    /* +0x00 */ s16 joint;
    /* +0x02 */ s16 joint_b;
    /* +0x04 */ u16 shape;
    /* +0x06 */ u8 unused_0x06[0x08 - 0x06];
    /* +0x08 */ u16 part;
    /* +0x0A */ u8 unused_0x0A[0x0C - 0x0A];
    /* +0x0C */ f32 radius;
    /* +0x10 */ nw4r::math::VEC3 offset;
    /* +0x1C */ nw4r::math::VEC3 offset_b;
};

/* One entry of the global hit registry's linked lists (the map's `_HIT_W`, spelled by
 * `hit_flag_set__FP6_HIT_WUl` / `hit_result_check__FP6_HIT_W`): the record a live attack is registered as.  `+0x00` is the list
 * link (`hit_attack_list_push`/`hit_body_list_push` write the previous head here), `+0x05` is tested for zero before the push
 * (and is the "result valid" byte `hit_result_check` reports from), `+0x06`/`+0x07` select the owner's type twice over and
 * `+0x10`/`+0x14` are the two owner pointers - `hit_data_apply` stores the hit id at `+0x0C` and reads the owner's area byte
 * through `+0x10`.  The accessors of `Pl/pl_hit.cpp` read the state bits at `+0x0A`, the result id at `+0x0E`, four stored
 * values at `+0x18..+0x1E`, the flag word at `+0x20` and the mask bytes at `+0x31`/`+0x5B`.
 * size: 0x60 (lower bound - the highest offset the units touch is +0x5B) */
struct _HIT_W {
    /* +0x00 */ _HIT_W* next_0x00;
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ union {
        u8 active_0x05;
        u8 result_valid;       /* zero means the entry carries no result yet */
    };
    /* +0x06 */ u8 owner_kind_0x06;   /* 0 = player, 1 = enemy, 2 = shell, 3 = AI NPC (`hit_data_apply` reads each one's area) */
    /* +0x07 */ u8 kind_0x07;
    /* +0x08 */ union {
        _HIT_SIZE_DATA* size_data_0x08;   /* the hit-box size record `shell_attack_set` picks */
        struct {
            u16 unused_0x08;
            u16 state;         /* the state bits `hit_push_enabled_ck`/`hit_camera_push_enabled_ck` test */
        };
    };
    /* +0x0C */ u16 hit_id_0x0C;
    /* +0x0E */ u8 result;     /* the result id `hit_result_check` reports */
    /* +0x0F */ u8 area_0x0F;   /* the owner's area the hit lives in (`hit_data_apply`); only that area's bodies are hit */
    /* +0x10 */ void* owner_0x10;
    /* +0x14 */ void* owner_0x14;
    /* +0x18 */ u16 field_0x018;      /* the four values `hit_motion_set` stores */
    /* +0x1A */ u16 field_0x01A;
    /* +0x1C */ u16 field_0x01C;
    /* +0x1E */ u16 field_0x01E;
    /* +0x20 */ u32 flags;            /* the hit flag word the setters OR/ANDC into */
    /* +0x24 */ f32 scale_0x24;          /* set to 1 by `hit_data_apply` */
    /* +0x28 */ u32 field_0x028;         /* cleared by `hit_data_apply` */
    /* +0x2C */ u32 field_0x02C;         /* cleared by `hit_data_apply` */
    /* +0x30 */ u8 field_0x030;          /* cleared by `hit_data_apply` */
    /* +0x31 */ union {
        u8 attack_flags_0x31;  /* the attack-flag byte `shell_attack_set` stores (bit 4 is dropped for a weak enemy master) */
        u8 field_0x031;        /* the mask byte `hit_attack_flag_ck` tests */
    };
    /* +0x32 */ u8 attack_kind_0x32;
    /* +0x33 */ u8 side_flags_0x33;   /* bit 0/1: the first/second side value started counting down (`hit_life_update`) */
    /* +0x34 */ u8 unused_0x34[0x38 - 0x34];
    /* +0x38 */ union {
        f32 power_0x38;          /* the attack record's first s16, as a float */
        f32 delay_0x38;          /* the same word as the frames before the hit goes live (`hit_life_update`) */
    };
    /* +0x3C */ union {
        f32 knock_0x3C;        /* its second s16, as a float */
        f32 field_0x03C;       /* the s16 `hit_knock_set` widens and stores */
        f32 time_0x3C;         /* the same word as the live frames left (`hit_life_update`) */
    };
    /* +0x40 */ s16 life_0x40;           /* the hit's remaining frames (scaled by an enemy master's rate) */
    /* +0x42 */ u8 kind_0x42;     /* the attack kind the targets filter on (0x0A/0x0B, 0x0E/0x0F, 0x10, 0x20) */
    /* +0x43 */ u8 flags_0x43;    /* bit 4: the knock direction comes from the hit position (`hit_dir_y_get`) */
    /* +0x44 */ u8 dir_ofs_0x44;  /* the knock direction's offset, in 256ths of a turn */
    /* +0x45 */ u8 field_0x045;   /* the attack record's +0x09 byte */
    /* +0x46 */ u16 mask_0x46;  /* the attack mask `em_hit_buff_apply` (`enemy/em_common.cpp`) tests against the enemy data's buff */
    /* +0x48 */ u16 field_0x048;     /* the attack-set band (`Pl/pl_act.cpp`) stores a rate word here */
    /* +0x4A */ u8 field_0x04A;
    /* +0x4B */ u8 field_0x04B;
    /* +0x4C */ u8 field_0x04C;
    /* +0x4D */ u8 field_0x04D;
    /* +0x4E */ s8 field_0x04E;      /* the signed hit-strength byte `Pl_attack_set_sub` scales */
    /* +0x4F */ u8 unused_0x4F;
    /* +0x50 */ f32 value_0x50;      /* `hit_data_apply` stores `-100.0f` or a lowered byte here */
    /* +0x54 */ f32 value_0x54;      /* the same for the second owner slot */
    /* +0x58 */ u8 reaction_0x58;  /* the owner reaction value `hit_owner_reaction_set` stores */
    /* +0x59 */ u8 field_0x059;   /* the attack record's +0x16 byte */
    /* +0x5A */ u8 field_0x05A;
    /* +0x5B */ u8 field_0x05B;      /* the mask byte `hit_mask_ck` tests */
    /* +0x5C */ u8 unused_0x5C[0x60 - 0x5C];
};

/* The hit registry itself (`.bss` `hit_registry`, size 0x10): two singly-linked lists of live hits (attacks at +0x00,
 * bodies at +0x04, which `hit_body_list_get` hands out), their counts and the running id `get_hit_id()` hands out.
 * size: 0x10 */
struct HitRegistry {
    /* +0x00 */ _HIT_W* head_0x00;
    /* +0x04 */ union {
        _HIT_W* head_0x04;
        u32 field_0x004;       /* the same head as a word */
        struct _BODY_W* body_head_0x04;   /* the same head as the body slots it links (`Pl/pl_hit.h`) */
    };
    /* +0x08 */ u16 count_0x08;
    /* +0x0A */ u16 count_0x0A;
    /* +0x0C */ u16 next_id_0x0C;   /* `get_hit_id` increments it and wraps at 0xEA60 */
    /* +0x0E */ u16 unused_0x0E;
};

#endif /* MHTRI_PL_HIT_W_H */
