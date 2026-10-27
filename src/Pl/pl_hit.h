/*
 * Pl/pl_hit.h - `Pl/pl_hit.cpp`'s outbound declarations and the body records `body_set` fills: the hit-entry accessors
 *   (`hit_flag_set`, `hit_result_check`, the flag and state predicates), the hit registry's body list and the id-list
 *   helpers.  `_HIT_W` / `HitRegistry` are `Pl/hit_w.h`'s (rule 1); they are only pointed at here, so a unit that reads
 *   `ai/ai_npc.h`'s view of `_HIT_W` can include this header too.
 */
#ifndef MHTRI_PL_PL_HIT_H
#define MHTRI_PL_PL_HIT_H

#include "types.h"
#include "nw4r/math.h"
#include "ef/cp_vector.h"
#include "Pl/em_weak_point.h"

struct _HIT_W;
struct HitRegistry;
struct _ENEMY_WORK;
struct HitSphere;
struct PlCapsule;

/* The record `body_set` copies the hunter's armour/appearance bytes out of; only the four bytes
 * the unit reads are named, and the record is at least 0x1E2 bytes (the highest offset touched).
 * size: 0x1E2 (approximate: max touched offset + 1) */
struct _BODY_DATA {
    /* +0x000 */ u8 unused_0x000[0x008];
    /* +0x008 */ u8 chunk_ofs;        /* kind 2's source byte */
    /* +0x009 */ u8 unused_0x009[0x016 - 0x009];
    /* +0x016 */ u8 area;             /* kind 0's source byte */
    /* +0x017 */ u8 unused_0x017[0x1A4 - 0x017];
    /* +0x1A4 */ u8 field_0x1A4;      /* kind 3's source byte */
    /* +0x1A5 */ u8 unused_0x1A5[0x1E1 - 0x1A5];
    /* +0x1E1 */ u8 field_0x1E1;      /* kind 1's source byte */
};

/* A placed object a body slot belongs to (`_BODY_W::owner_0x10`): its position and rotation, which carry the slot's
 * shape into the world (`body_shape_get`).
 * size: 0x30 (lower bound: the rotation at +0x24 is the last field read) */
struct BodyOwnerObj {
    /* +0x00 */ u8 unused_0x00[0x18];
    /* +0x18 */ nw4r::math::VEC3 pos;
    /* +0x24 */ _CP_VECTOR rot;
};

/* The body slot `body_set` fills in: one entry of the registry's body list (`hit_body_list_push`), the shape record
 * and the owner it was filled from, and the owner's area byte the push pass compares.  +0x03F is the last byte written.
 * size: 0x40 (approximate: max touched offset + 1) */
struct _BODY_W {
    /* +0x000 */ _BODY_W* next_0x00;   /* the list link `hit_body_list_push` writes */
    /* +0x004 */ u8 unused_0x004;
    /* +0x005 */ u8 field_0x005;      /* set to 1 on every call */
    /* +0x006 */ u8 field_0x006;      /* cleared on every call */
    /* +0x007 */ u8 kind;             /* the kind argument, kept */
    /* +0x008 */ u8 source;           /* the byte the kind's source field supplied */
    /* +0x009 */ u8 source_kind;      /* the kind's sub-code */
    /* +0x00A */ union {
        u16 field_0x00A;            /* as `body_set` clears it */
        s16 hit_timer_0x0A;         /* the frames before the slot can be hit again (`hit_registry_update`) */
    };
    /* +0x00C */ union {
        u32 data;                   /* the shape record the slot was filled from, as `body_set` stores it */
        EmWeakPoint* shape_0x0C;    /* the same word as the shape `body_shape_get` reads */
    };
    /* +0x010 */ union {
        u32 field_0x010;            /* the owner, as `body_set` stores it */
        BodyOwnerObj* owner_0x10;   /* the same word as the placed object `body_shape_get` reads */
    };
    /* +0x014 */ u16 field_0x014;
    /* +0x016 */ u16 hit_ids_0x16[20];  /* the hit ids that already struck the slot (`hit_id_list_absent_ck`) */
    /* +0x03E */ u8 hit_id_count_0x3E;
    /* +0x03F */ u8 field_0x03F;
};

/* The unit's own `.bss`/`.data` (declared, not yet emitted): the hit registry (0x10) and the 37-entry priority table of the
 * damage reactions `hit_reaction_priority_get` indexes. */
extern HitRegistry hit_registry;
extern u8 hit_reaction_priority_tbl[];

/* The unit's C++-mangled entry points (the map names are these manglings, rule 9). */
void body_set(_BODY_W* body, _BODY_DATA* data, u8 kind, u32 work, u8 mode);
void hit_flag_set(_HIT_W* hit, u32 flags);
u8 hit_result_check(_HIT_W* hit);
u16 get_hit_id(void);

#ifdef __cplusplus
extern "C" {
#endif

u32 hit_side_kind_get(_HIT_W* self);
u32 hit_reaction_priority_get(u32 id);
u32 hit_area_match_ck(u8 a, u8 b);
u32 hit_id_list_absent_ck(u16* ids, u16 id);
void hit_id_list_push(u16* ids, u8* count, u16 id);
void hit_motion_set(_HIT_W* hit, u16 a, u16 b, u16 c, u16 d);
u32 hit_mask_ck(_HIT_W* hit, u32 mask);
void hit_flags_clear(_HIT_W* hit);
void hit_flag_clear(_HIT_W* hit, u32 flags);
u32 hit_flag_ck(_HIT_W* hit, u32 flags);
u32 hit_attack_flag_ck(_HIT_W* hit, u32 mask);
void hit_knock_set(_HIT_W* hit, s16 value);
s32 em_part_sphere_get(struct _ENEMY_WORK* em, EmWeakPoint* part, PlCapsule* capsule, HitSphere* sphere);
u32 hit_push_enabled_ck(EmWeakPoint* part);
u32 hit_camera_push_enabled_ck(EmWeakPoint* part);
_BODY_W* hit_body_list_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_HIT_H */
