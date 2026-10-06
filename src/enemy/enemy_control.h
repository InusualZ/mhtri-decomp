/* The records `enemy/enemy_control.cpp` reads out of the enemy-control work blob `emc_work` (0x806A4590, 0xF50);
 * its 0x18-byte per-enemy slot is `EmcSlot` here and `EmcWork` in `enemy/em_kind.cpp` (one record, two definitions).
 */
#ifndef MHTRI_ENEMY_ENEMY_CONTROL_H
#define MHTRI_ENEMY_ENEMY_CONTROL_H

#include "types.h"
#include "nw4r/math.h" /* nw4r::math::VEC3 - the sparkle record's position (rule 11) */

/* one 0x18-byte per-enemy slot of `emc_work` (the array `fn_801413D0` scans and `fn_80141358`
 * clears; `enemy/em_kind.cpp` calls the same record `EmcWork`).
 * size: 0x18 */
struct EmcSlot {
    /* +0x00 */ u8 id;         /* the slot index `fn_80141358` stores, returned by `fn_80141470` */
    /* +0x01 */ u8 state;      /* 0 idle, 1 claimed, 2 released */
    /* +0x02 */ u8 kind;       /* the file kind `fn_80141470` claims the slot for */
    /* +0x03 */ u8 flags;
    /* +0x04 */ u8 field_0x04; /* cleared by `em_kind_slot_release` on release */
    /* +0x05 */ u8 unused_0x05[3];
    /* +0x08 */ s32 handle_0x08;
    /* +0x0C */ s32 handle_0x0C;
    /* +0x10 */ s32 handle_0x10;
    /* +0x14 */ s32 handle_0x14;
};

/* one 0x14-byte sparkle ("senko") record: the array at `emc_work + 0x90` (`fn_801416EC` returns its
 * first free element, `senko_set` fills it, `fn_80141690` frees an element).
 * size: 0x14 */
struct SenkoRec {
    /* +0x00 */ nw4r::math::VEC3 pos_0x00;
    /* +0x0C */ f32 value_0x0C;  /* `senko_set`'s float */
    /* +0x10 */ s16 countdown;   /* `senko_set`'s third value / the tick `fn_801417FC` steps */
    /* +0x12 */ u8 value_0x12;
    /* +0x13 */ u8 state;        /* 2 = free (set by `fn_80141690`), else in use */
};

/* one 0x24-byte record of the second marker array at `emc_work + 0x1F8` (the ten entries
 * `fn_80141B2C` clears the +0x09 byte of; `hud/pl_frame_sync.cpp` packs a record into a net message).
 * size: 0x24 */
struct Marker2Rec {
    /* +0x00 */ u16 field_0x00;
    /* +0x02 */ u16 enemy_id_0x02;   /* the `em_get_unique_work` key of the enemy the marker belongs to */
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 field_0x05;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ u8 field_0x08;
    /* +0x09 */ u8 field_0x09;
    /* +0x0A */ u8 field_0x0A;
    /* +0x0B */ u8 pad_0x0B;
    /* +0x0C */ nw4r::math::VEC3 pos_0x0C;
    /* +0x18 */ u32 field_0x18;
    /* +0x1C */ u32 field_0x1C;
    /* +0x20 */ u32 field_0x20;
};

/* the last enemy-control event the net sync replays: the enemy's id, phase and step and the event kind and
 * its three values.  size: 0x8 */
struct EmcStatus {
    void assign(const EmcStatus* src);   /* copies the record field by field */
    /* +0x0 */ u16 enemy_id;
    /* +0x2 */ u8 phase;
    /* +0x3 */ u8 step;
    /* +0x4 */ u8 kind;
    /* +0x5 */ u8 value_0x05;
    /* +0x6 */ u8 value_0x06;
    /* +0x7 */ u8 value_0x07;
};

/* the enemy-control work blob `emc_work`, as far as this unit's reconstructed functions read it.
 * Unnamed runs keep their offsets (rules 4/5); fields are added as the bodies need them.
 * size: 0xF50 */
struct EmcWork {
    /* +0x000 */ EmcSlot slot_0x000[6];
    /* +0x090 */ SenkoRec senko_0x090[8];
    /* +0x130 */ SenkoRec marker_0x130[10];
    /* +0x1F8 */ struct Marker2Rec marker2_0x1F8[10];
    /* +0x360 */ u32 field_0x360;
    /* +0x364 */ u32 field_0x364;
    /* +0x368 */ s16 field_0x368;
    /* +0x36A */ u8 field_0x36A;
    /* +0x36B */ u8 field_0x36B;
    /* +0x36C */ u8 unused_0x36C[0xD9C - 0x36C];
    /* +0xD9C */ EmcStatus status_0xD9C;   /* the last event `hud/pl_frame_sync.cpp` replays to its peers */
    /* +0xDA4 */ u8 unused_0xDA4[0xF48 - 0xDA4];
    /* +0xF48 */ u32* field_0xF48;
    /* +0xF4C */ u32 field_0xF4C;
};

#include "types.h"

struct _ENEMY_WORK;

struct _ENEMY_MINI_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* The mini-enemy event handlers the net sync (`hud/pl_frame_sync.cpp`) drives; the signatures are the call
 * sites' (GUESS: the roles). */
void emc_mini_event_b(struct _ENEMY_MINI_WORK* mini, s32 mode);
void emc_mini_event_a(struct _ENEMY_MINI_WORK* mini, s32 a, s32 b);
void emc_mini_step(struct _ENEMY_MINI_WORK* mini);
/* 0x80141F6C - replays a marker on a mini enemy: the marker's kinds, id and position, then a value list
 * (the last two arguments travel on the stack). */
void emc_marker_replay(struct _ENEMY_MINI_WORK* mini, u8 a, u8 b, u16 id, u8 c, u8 d, u8 e, const nw4r::math::VEC3* pos,
                 s32* values, s32 count);

/* the entry points this unit publishes to its consumers (docs/plan.md 6.5 rule 2).  `fn_80143174`
 * is `src/Pl/pl_act.cpp`'s - it is registered but not yet reconstructed (see the unit source's
 * Status). */
void* fn_80143174(void* a, void* b, s32 c);
/* The action band's arming helpers in this unit's range that `enemy/em015_prog.cpp` calls (unwritten; the
 * signatures are the call sites'). */
void em_demo_pos_set(struct _ENEMY_WORK* self, f32 a, f32 b, f32 c);
void em_demo_rot_set(struct _ENEMY_WORK* self, f32 a, f32 b, f32 c);


/* 0x801421E4 - r3 is narrowed with `clrlwi r3,r3,16` (a u16 id, 0xFFFF = the "no record" arm) and r4
 * is the out record `em_ground_rec_clear` prepared; returns a word the enemy program functions compare with 1. */
u32 fn_801421E4(u32 id, void* out);
/* 0x80146008 - reads the record's motion timer and returns 1 once the elapsed frame count has passed the value in
 * r3 (the call sites pass 0x96/0x12C/...). */
u32 em_demo_time_ck(u32 frames);
void em_demo_enable(struct _ENEMY_WORK* self);
/* 0x80145FE4 - the joint-effect slot allocator (returns the slot index, -1 when the set is full). */
s16 em_demo_frame_get(void);
/* 0x8014616C - r3 `self` and r4 the mode. */
void em_demo_reset(struct _ENEMY_WORK* self, s32 mode);
/* 0x801461A8 - r3 `self`, r4 the `s16` slot, r5/r6 the caller's two vectors. */
void em_demo_key3_apply(struct _ENEMY_WORK* self, s16 slot, void* a, void* b);
/* 0x801462A4 - r3 `self`, r4 the `s16` slot, r5/r6 two pointers and r7/r8 two scalars. */
void em_demo_key_apply(struct _ENEMY_WORK* self, s16 slot, void* a, void* b, s32 c, s32 d);

/* 0x80141B88 - the enemy spawn request (GUESS name).  The tenth argument is a placement `VEC3*` or null: the
 * callee loads the outgoing stack word into r21 and hands it to `copyVec3` as the second argument when it is
 * non-null (`enemy/em008_prog.cpp` passes a vector, `enemy/em012_prog.cpp` null). */
namespace nw4r { namespace math { struct VEC3; } }
void em_spawn_request(u16 a, s32 b, s32 c, u8 d, u8 e, s32 f, s32 g, s32 h, s32 i, nw4r::math::VEC3* j, s32 k);

/* 0x80147894 - builds the spawn handle of entry `index` of the area block `area` for the entry kind
 * `kind` (`quest/quest_entry.cpp`'s area list stores the result).  GUESS name and signature from that
 * call site. */
u32 em_area_entry_make(u8* area, u8 kind, u8 index);

/* 0x80143BF8 - no arguments; returns a word the effect band's slot scan (`ef/eft_slot.cpp`) compares with 1. */
u8 fn_80143BF8(void);

#ifdef __cplusplus
}

/* 0x801445E0 - the enemy record lookup by id: stores the record in `*out` (and its mini work in `*mini` when
 * non-null); a C++ free function, so the map row is the mangling (rule 9). */
struct _ENEMY_MINI_WORK;
u32 em_get_unique_work(u16 id, struct _ENEMY_WORK** out, struct _ENEMY_MINI_WORK** mini);
#endif


/* The quest spawn entry points (GUESS names from the quest callers): the ground record from a monster slot's
 * placement block (0x801423E0), the small and large spawns (0x80142BC0 / 0x80143654), the intruder spawn
 * (0x80142CE8), the check before a key monster makes way (0x80146B1C) and the area spawn clear (0x80143B84). */
struct EmGroundRec;
struct Q_ElementBlock;
struct QuestBossSpawn;
#ifdef __cplusplus
extern "C" {
#endif
void em_ground_rec_set(struct EmGroundRec* rec, const struct Q_ElementBlock* element);
struct _ENEMY_WORK* em_small_spawn(u8 monster, struct EmGroundRec* rec, u16 order);
struct QuestBossSpawn* em_large_spawn(u8 monster, struct EmGroundRec* rec, u16 order);
void em_intruder_spawn(u8 monster, struct EmGroundRec* rec, u16 row);
s32 em_kind_release_ck(u8 monster);
void em_area_spawn_clear(void);
/* 0x80143A54 - releases every live area entry of the current area (GUESS name). */
void em_area_entries_release(void);
/* 0x801477C0 - the first free enemy resource buffer of the control work's six, NULL when all are taken (GUESS name). */
u8* em_res_buffer_get(void);
/* 0x801414D4 - the control work's slot holding monster kind `kind` (0..5), 0xFF when none does (GUESS name). */
u8 em_kind_slot_find(u32 kind);
/* 0x801415A8 - releases control slot `index`: its sound bank, its resources and its kind (GUESS name). */
void em_kind_slot_release(u8 index);
/* 0x80143A40 - releases one live area entry (GUESS name). */
void em_area_entry_release(struct EmAreaEntry* entry);
/* 0x801465A0 - the enemy level the quest sets (GUESS name). */
void em_level_set(s8 level);
/* 0x801422FC - resets every enemy work and the effect slot pool (GUESS name). */
void enemy_work_reset(void);
/* 0x80142E1C / 0x80142EEC - spawn a free-hunt placement entry's monster with spawn order `order`: the small
 * spawn returns the enemy, the large one goes through the large spawn (GUESS names). */
struct EmSetEntry;
struct _ENEMY_WORK* em_set_entry_spawn(struct EmSetEntry* entry, u16 order);
s32 em_set_boss_spawn(struct EmSetEntry* entry, u16 order);

#ifdef __cplusplus
}
#endif

#endif
