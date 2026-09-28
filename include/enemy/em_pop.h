/*
 * The enemy population/roster records `src/enemy/em_pop.cpp` owns - the 0x224-byte per-monster
 * roster record the map's `.bss` band names (`em_bui_tbl`, `em_hagi_tbl`, `em_drop_tbl` at
 * 0x80794C28..0x80794C58 from the runtime dump), and the manager work those records live in.
 *
 * Sizes are traced from the object's own arithmetic: `work_mem_alloc(0x34A68)` allocates the work,
 * `0x11200` the record array (`128 x 0x224`) and `0x1600` the sub-record array; the record's
 * stride is the `mulli r0, r3, 0x224` in `em_roster_record_get`.
 *
 * `src/enemy/fn_8035E034.cpp` carries its own two-field view of the same record under the name
 * `EmRosterRec` (its size is 0x1F4, ending at the `+0x1F0` approach radius).  That view is a
 * prefix of this one; folding it into this header is the follow-up recorded in this unit's residual list.
 */
#ifndef MHTRI_ENEMY_EM_POP_H
#define MHTRI_ENEMY_EM_POP_H

#include "types.h"
#include "nw4r/math.h"

struct _g3d_work;
struct _ENEMY_WORK;

/* One roster record.  Written by the allocator (`state_0x000 = 1`), read by every
 * `em_roster_record_*` helper. */
typedef struct EmPopRec {
    /* +0x000 */ u8 state_0x000;      /* 1 = live; 0 = free (`em_roster_free_record_get` finds these) */
    /* +0x001 */ u8 field_0x001;
    /* +0x002 */ u8 index_0x002;      /* the record's own slot number, stamped by the allocator */
    /* +0x003 */ u8 action_0x003;     /* the action code `em_roster_record_result_get` switches on */
    /* +0x004 */ u8 kind_0x004;       /* the "team"/kind byte the kind searches compare against */
    /* +0x005 */ u8 field_0x005;
    /* +0x006 */ s8 field_0x006;
    /* +0x007 */ u8 pad_0x007;
    /* +0x008 */ u8 field_0x008;
    /* +0x009 */ u8 field_0x009;
    /* +0x00A */ u8 field_0x00A;
    /* +0x00B */ u8 pad_0x00B[9];
    /* +0x014 */ nw4r::math::VEC3 pos_0x014;   /* the placement position */
    /* +0x020 */ u8 pad_0x020[0x124];
    /* +0x144 */ struct _g3d_work* g3d_0x144; /* the model handle released before the record is cleared */
    /* +0x148 */ u8 pad_0x148[0x88];
    /* +0x1D0 */ nw4r::math::VEC3 aim_0x1D0;   /* the aim position (same vector as `pos_0x014`) */
    /* +0x1DC */ u8 pad_0x1DC[0x14];
    /* +0x1F0 */ f32 radius_0x1F0;
    /* +0x1F4 */ u8 pad_0x1F4[6];
    /* +0x1FA */ u16 slot_id_0x1FA;   /* the id `em_roster_record_slot_id_get` reports (-1 when unset) */
    /* +0x1FC */ u8 pad_0x1FC[0x28];
} EmPopRec; /* size: 0x224 */

/* The sub-record array the manager allocates alongside the roster (stride 0x20C).  Only the two
 * offsets the helpers touch are named. */
typedef struct EmPopSubRec {
    /* +0x000 */ u8 field_0x000;
    /* +0x001 */ u8 pad_0x001[0x143];
    /* +0x144 */ struct _g3d_work* g3d_0x144;
    /* +0x148 */ u8 pad_0x148[0xC4];
} EmPopSubRec; /* size: 0x20C */

/* The manager work, allocated whole by the work allocator. */
typedef struct EmPopWork {
    /* +0x000 */ u8 pad_0x000[0x1C];
    /* +0x01C */ EmPopRec* recs;      /* 0x11200 B = 128 x 0x224 */
    /* +0x020 */ u32 rec_num;         /* 0x80 */
    /* +0x024 */ EmPopSubRec* subs;   /* 0x1600 B, stride 0x20C */
    /* +0x028 */ u32 sub_num;         /* 0x80 */
    /* +0x02C */ u8 pad_0x02C[0x34A08];
    /* +0x34A34 */ u8 field_0x34A34[0x10];
    /* +0x34A44 */ u8 field_0x34A44;
    /* +0x34A45 */ u8 field_0x34A45;
    /* +0x34A46 */ u8 pad_0x34A46;
    /* +0x34A47 */ u8 field_0x34A47;
    /* +0x34A48 */ u8 field_0x34A48;
    /* +0x34A49 */ s8 field_0x34A49[0x1F];
} EmPopWork; /* size: 0x34A68 */

/* The one work pointer the whole module goes through.  The map's symbol (`lbl_80794C58`) spans
 * 0x8 B and only its first word is ever read, so the second is carried as this unit's filler to
 * keep the object's `.sbss` contribution byte-exact. */
typedef struct EmPopWorkSlot {
    /* +0x00 */ EmPopWork* work;
    /* +0x04 */ u32 unused_0x04;
} EmPopWorkSlot; /* size: 0x8 */

#ifdef __cplusplus
extern "C" {
#endif

extern EmPopWorkSlot em_pop_w;

/* The roster accessor (0x803BDECC) - the one this unit's table searches go through. */
EmPopRec* em_roster_record_get(u32 index);
/* 0x803B50A8 - the work state word's bit 21, with the `self == NULL -> singleton` fallback every
 * accessor in this band carries.  Moved here from `include/unsplit/unknown.h` when this unit
 * registered the address (rule 2: the owner's header carries it). */
u32 em_work_state_bit21_ck(void);
/* 0x803B9588 - the per-slot effect/joint binding the range's entry points walk: r3 the work
 * record, r4 the slot index, r5 the slot pointer, r6/r7 two scalars. */
void em_roster_slot_effect_set(struct _ENEMY_WORK* self, u16 index, s32* slot, s32 a, s32 b);
s32 em_roster_record_slot_id_get(u32 index);
u32 em_roster_record_alive_ck(u32 index);
void em_roster_record_pos_set(s32 index, nw4r::math::VEC3* pos, u8 area);
/* 0x803B521C - the work state word's bit 7, same `self == NULL -> singleton` fallback as above.
 * 0x803B6078 - fills two u16s from the work's per-slot pair when the slot is armed.
 * 0x803B8E1C - the em_set work's own state word.
 * All three moved out of `include/unsplit/menu.h` when this unit registered their addresses
 * (rule 2: the owner's header carries them). */
u32 em_work_state_bit7_ck(s32 self);
void em_work_slot_pair_get(u16 index, u16* out);
s32 em_set_work_state_get(void);
void em_roster_record_release(u32 index);
EmPopRec* em_roster_record_clear(EmPopRec* rec);
u32 em_roster_sub_record_clear(EmPopSubRec* rec);
EmPopRec* em_roster_free_record_get(void);
EmPopSubRec* em_roster_sub_free_get(void);
void em_roster_record_copy(EmPopRec* dst, const EmPopRec* src);
void em_roster_record_unlink(EmPopRec* rec);
u32 em_roster_record_result_get(const EmPopRec* rec);

/* The quest-condition predicates that live at 0x803B4BEC..0x803B5120 - inside this unit's own range,
 * so this header is their owner's home (rule 2).  Each one takes the result record (NULL selects the
 * work's own through `quest_record_get`) and reports one condition bit of `rec->0x310`; the bits are
 * this header's own reading of the eight call sites, all in the quest board and the result screens.
 * Added with `quest/quest_entry.cpp`, whose entry predicate is the 0x800000 one. */
struct QuestRecord;
u32 quest_flag_800000_ck(struct QuestRecord* rec);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_POP_H */
