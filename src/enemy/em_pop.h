/* The enemy population/roster records `enemy/em_pop.cpp` owns: the 0x224-byte per-monster roster record (the `.bss`
 * tables `em_bui_tbl`, `em_hagi_tbl`, `em_drop_tbl` at 0x80794C28..0x80794C58 are runtime-dump names) and the manager
 * work they live in.  Sizes from the object's arithmetic: `work_mem_alloc(0x34A68)` for the work, `0x11200` for the
 * record array (`128 x 0x224`), `0x10600` for the sub-records (`128 x 0x20C`), and `mulli r0, r3, 0x224` in `em_roster_record_get`.
 * `enemy/em033_prog.cpp`'s `EmRosterRec` (0x1F4, ending at the +0x1F0 approach radius) is a prefix view of it.
 */
#ifndef MHTRI_ENEMY_EM_POP_H
#define MHTRI_ENEMY_EM_POP_H

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"              /* MHchar, the record's model (rule 1) */
#include "ef/cp_vector.h"     /* _CP_VECTOR (rule 1) */

struct _g3d_work;
struct _ENEMY_WORK;
struct ResEntry;
struct EmWeightEntry;

struct EmPopObj;
/* The per-frame step a record carries at +0x1BC; `em_pop_work_step` runs it over every live record and
 * sub-record. */
typedef void (*EmPopObjStep)(struct EmPopObj* obj);

/* The head the roster records and the sub-records share (0x1C8 bytes): the state bytes, the placement, the
 * model and its spin/fade state.  The model helpers of `enemy/em_model.cpp` take either record through it. */
struct EmPopObj {
    /* +0x000 */ u8 state_0x000;      /* 1 = live; 0 = free (`em_roster_free_record_get` finds these) */
    /* +0x001 */ u8 in_area_0x001;    /* whether the local player is in the record's area (`quest_move_area_sub_ck`) */
    /* +0x002 */ u8 index_0x002;      /* the record's own slot number, stamped by the allocator */
    /* +0x003 */ u8 action_0x003;     /* the record class: 0 a placed point, 1 a spare, 2 a drop */
    /* +0x004 */ u8 kind_0x004;       /* the area the record lives in; the kind searches compare against it */
    /* +0x005 */ u8 field_0x005;      /* the model kind: 0xFF none, 0x81 the glow, 0xA the spring, 0xC/0x10 the bobbing */
    /* +0x006 */ s8 field_0x006;      /* the source: 0 a pop-file point, 1 a spare record, 2 a drop */
    /* +0x007 */ u8 pad_0x007;
    /* +0x008 */ u8 field_0x008;      /* the step the record's step function switches on */
    /* +0x009 */ u8 field_0x009;      /* the sub-step */
    /* +0x00A */ u8 field_0x00A;      /* set once the fade has run out */
    /* +0x00B */ u8 pad_0x00B;
    /* +0x00C */ s16 timer_0x00C;     /* the spring's emission countdown */
    /* +0x00E */ s16 fade_0x00E;      /* the fade frames (0x14 down to 0) */
    /* +0x010 */ s32 counter_0x010;   /* the step's frame counter */
    /* +0x014 */ nw4r::math::VEC3 pos_0x014;   /* the placement position */
    /* +0x020 */ _CP_VECTOR rot_0x020;         /* the placement rotation */
    /* +0x02C */ nw4r::math::VEC3 scale_0x02C; /* the model scale (1.0 each while shown) */
    /* +0x038 */ MHchar model_0x038;           /* the record's model */
    /* +0x178 */ u8 pad_0x178[0x24];
    /* +0x19C */ _CP_VECTOR spin_0x19C[2];     /* the node rotations the world callback applies, by `spin_index_0x1B7` */
    /* +0x1B4 */ u16 spin_speed_0x1B4;         /* the per-frame rotation step */
    /* +0x1B6 */ u8 spin_dir_0x1B6;            /* the swing direction */
    /* +0x1B7 */ u8 spin_index_0x1B7;          /* which rotation the world callback applies */
    /* +0x1B8 */ s32 life_0x1B8;               /* the frames the record stays gatherable (0x708) */
    /* +0x1BC */ EmPopObjStep step_0x1BC;      /* the per-frame step */
    /* +0x1C0 */ u8 pad_0x1C0[4];
    /* +0x1C4 */ u8 callback_mode_0x1C4;       /* 0: the world callback swings the node; else it scales it */
    /* +0x1C5 */ u8 visible_0x1C5;             /* the glow step's "still shown" flag */
    /* +0x1C6 */ s8 frame_0x1C6;               /* the glow's key frame */
    /* +0x1C7 */ u8 pad_0x1C7;
}; /* size: 0x1C8 */

/* The ground probe `em_pop_obj_tint_set` hands the collision query (+0x208 of a roster record).  size: 0x14
 * (approximation: only the two read fields are named) */
struct EmPopGround {
    /* +0x00 */ u16 kind_0x00;        /* 1 lowers the tint when the record sits below the ground plane */
    /* +0x02 */ u8 pad_0x02[5];
    /* +0x07 */ u8 tint_0x07;         /* the tint offset the stage colour table is read at */
    /* +0x08 */ u8 pad_0x08[0xC];
};

/* One placement of a spare record: its cylinder, position and rotation.  size: 0x24 */
struct EmPopSparePlace {
    /* +0x00 */ u16 valid_0x00;       /* nonzero when the placement is used */
    /* +0x02 */ u8 pad_0x02[2];
    /* +0x04 */ nw4r::math::VEC3 pos_0x04;
    /* +0x10 */ _CP_VECTOR rot_0x10;
    /* +0x1C */ f32 radius_0x1C;
    /* +0x20 */ f32 height_0x20;
};

/* One kind a spare record can take, drawn by weight.  size: 0x10 */
struct EmPopSpareKind {
    /* +0x00 */ u16 weight_0x00;
    /* +0x02 */ u16 item_0x02;        /* the item index into the stage's item table */
    /* +0x04 */ u16 model_0x04;       /* the model-kind index (0xFF none) */
    /* +0x06 */ u8 point_0x06;        /* the point id */
    /* +0x07 */ u8 valid_0x07;        /* nonzero when the kind is used */
    /* +0x08 */ u16 count_0x08;
    /* +0x0A */ u16 bonus_0x0A;
    /* +0x0C */ u16 uses_0x0C;
    /* +0x0E */ u16 uses_max_0x0E;
};

/* A spare record: the head `em_roster_record_copy` moves into the work's spare table, with its eight
 * placements and eight kinds.  size: 0x1A4 */
struct EmPopSpare {
    /* +0x000 */ u16 field_0x000;
    /* +0x002 */ u16 area_0x002;
    /* +0x004 */ EmPopSparePlace places_0x004[8];
    /* +0x124 */ EmPopSpareKind kinds_0x124[8];
};

/* One point of a pop file, as `em_roster_pop_entry_add` takes it (`menu/get_pop_dat_ptr.cpp` builds it from the file's
 * 0x38-byte record).  size: 0x34 */
struct EmPopEntry {
    /* +0x00 */ nw4r::math::VEC3 pos_0x00;
    /* +0x0C */ _CP_VECTOR rot_0x0C;
    /* +0x18 */ f32 radius_0x18;
    /* +0x1C */ f32 height_0x1C;
    /* +0x20 */ u16 point_0x20;
    /* +0x22 */ u16 point_kind_0x22;
    /* +0x24 */ u16 count_0x24;
    /* +0x26 */ u16 bonus_0x26;
    /* +0x28 */ u16 uses_0x28;
    /* +0x2A */ u16 uses_max_0x2A;
    /* +0x2C */ s8 model_0x2C;
    /* +0x2D */ u8 area_0x2D;           /* the area the point is filed under (the caller's second argument) */
    /* +0x2E */ u8 pad_0x2E[2];
    /* +0x30 */ u32 item_0x30;
};

/* One roster record.  Written by the allocator (`state_0x000 = 1`), read by every
 * `em_roster_record_*` helper. */
struct EmPopRec : EmPopObj {
    /* +0x1C8 */ f32 rise_0x1C8;               /* the bobbing model's vertical speed */
    /* +0x1CC */ u32 sway_0x1CC;               /* the bobbing model's sway angle */
    /* +0x1D0 */ nw4r::math::VEC3 aim_0x1D0;   /* the aim position (same vector as `pos_0x014`) */
    /* +0x1DC */ _CP_VECTOR aim_rot_0x1DC;     /* the aim rotation */
    /* +0x1E8 */ EmPopSpare* spare_0x1E8;      /* the spare record the placement and kind are drawn from */
    /* +0x1EC */ const struct EmWeightEntry* drops_0x1EC; /* the drop table the area searches hand back */
    /* +0x1F0 */ f32 radius_0x1F0;
    /* +0x1F4 */ f32 height_0x1F4;
    /* +0x1F8 */ u16 state_0x1F8;              /* the point id the area searches return (`enemy/em025_prog.cpp` sets 3) */
    /* +0x1FA */ u16 slot_id_0x1FA;            /* the gathers left (the id `em_roster_record_slot_id_get` reports) */
    /* +0x1FC */ u16 state_0x1FC;              /* the point kind (0xFFFF: not a point) */
    /* +0x1FE */ u16 bonus_0x1FE;              /* the gathers left before the skill roll */
    /* +0x200 */ u8 pad_0x200[4];
    /* +0x204 */ u16 uses_0x204;               /* the uses left (0xFFFF: unlimited) */
    /* +0x206 */ u16 uses_max_0x206;
    /* +0x208 */ EmPopGround ground_0x208;
    /* +0x21C */ u8 on_ground_0x21C;           /* the ground probe's result */
    /* +0x21D */ s8 mat_0x21D;                 /* the "B-L-p02-ankou" material index, -1 when absent */
    /* +0x21E */ u8 field_0x21E;               /* the spare's "shown this cycle" flag / the id the 0x11 points hand out */
    /* +0x21F */ u8 pad_0x21F;
    /* +0x220 */ s16 timer_0x220;              /* the spare's respawn countdown / the spring's frame count */
    /* +0x222 */ u8 pad_0x222[2];
}; /* size: 0x224 */

/* The sub-record array the manager allocates alongside the roster (stride 0x20C): the effect models (the spring
 * bubbles and the glow motes) the roster records spawn. */
struct EmPopSubRec : EmPopObj {
    /* +0x1C8 */ nw4r::math::VEC3 vel_0x1C8;   /* the bubble's direction */
    /* +0x1D4 */ f32 speed_0x1D4;
    /* +0x1D8 */ u32 wobble_x_0x1D8;           /* the two wobble angles */
    /* +0x1DC */ u32 wobble_z_0x1DC;
    /* +0x1E0 */ u8 pad_0x1E0[8];
    /* +0x1E8 */ s32 tilt_0x1E8;               /* the rise direction's X and Y angles */
    /* +0x1EC */ u32 turn_0x1EC;
    /* +0x1F0 */ u32 orbit_0x1F0;              /* the orbiting bubble's angle */
    /* +0x1F4 */ u8 pad_0x1F4[0xC];
    /* +0x200 */ nw4r::math::VEC3 base_0x200;  /* the glow mote's spawn position */
}; /* size: 0x20C */

/* The manager work, allocated whole by the work allocator. */
typedef struct EmPopWork {
    /* +0x000 */ u8 state_0x000;               /* 2: the files are loaded; 3: running */
    /* +0x001 */ u8 pad_0x001[7];
    /* +0x008 */ struct ResEntry* res_0x008;  /* the first population file's resource */
    /* +0x00C */ u32 res_file_0x00C;           /* its `ResFile` handle */
    /* +0x010 */ u8 pad_0x010[4];
    /* +0x014 */ struct ResEntry* res_0x014; /* the second file's resource */
    /* +0x018 */ u32 res_file_0x018;           /* its `ResFile` handle */
    /* +0x01C */ EmPopRec* recs;      /* 0x11200 B = 128 x 0x224 */
    /* +0x020 */ u32 rec_num;         /* 0x80 */
    /* +0x024 */ EmPopSubRec* subs;   /* 0x10600 B = 128 x 0x20C */
    /* +0x028 */ u32 sub_num;         /* 0x80 */
    /* +0x02C */ u32 frame_0x02C;              /* the frame counter at the last area search */
    /* +0x030 */ u32 frame_prev_0x030;         /* the one before it */
    /* +0x034 */ EmPopSpare spares_0x034[16][0x20];  /* per area, the spare records */
    /* +0x34834 */ u8 spare_num_0x34834[16][0x20];   /* per area and slot, the spawn count */
    /* +0x34A34 */ u8 field_0x34A34[5];
    /* +0x34A39 */ u8 field_0x34A39[5];
    /* +0x34A3E */ u8 pad_0x34A3E[2];
    /* +0x34A40 */ EmPopSpare** spare_tbl_0x34A40; /* the pushed spare records (0x200 B: 128 pointers) */
    /* +0x34A44 */ u8 field_0x34A44;           /* how many are pushed */
    /* +0x34A45 */ u8 field_0x34A45;           /* how many were spawned */
    /* +0x34A46 */ u8 field_0x34A46;           /* the next id the 0x11 points hand out */
    /* +0x34A47 */ s8 field_0x34A47;           /* the spawns left from the first list */
    /* +0x34A48 */ s8 field_0x34A48;           /* the spawns left from the second list */
    /* +0x34A49 */ s8 field_0x34A49[2][15];    /* the two lists of pushed indices (-1 free) */
    /* +0x34A67 */ u8 pad_0x34A67;
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

/* The roster accessors, the work pointer `em_pop_w` and the model-band helpers (0x803B936C..0x803BE30C) are
 * `enemy/em_model.cpp`'s, declared in `enemy/em_model.h`. */

/* 0x803B8E1C - the em_set work's own state word. */
s32 em_set_work_state_get(void);

#include "quest/quest_result_field_text_cur_get.h"  /* the quest head, quest/quest_entry.cpp's (rule 2) */

/* 0x803B88A8 / 0x803B88EC - the local slot's move-work sub-state: read whether it is 4 and set it to 4.  GUESS names. */
s32 quest_move_sub_state_4_get(void);
void quest_move_sub_state_4_set(u8 flag);

/* One entry of a 0xFFFF-terminated weight table: its `weight` and the `value` a pick returns. size: 0x4 */
typedef struct EmWeightEntry {
    /* +0x0 */ u16 weight;
    /* +0x2 */ u16 value;
} EmWeightEntry;
/* 0x803B8E40 - settles the carried-item pouch (`Q_ItemPair` x35): returns the credit, `flag` set when a
 * category-2 item was cleared.  GUESS name. */
struct Q_ItemPair;
s32 quest_pouch_items_settle(struct Q_ItemPair* pouch, u8* flag);

/* 0x803B8650 - warps the local player to the start of stage 1 (the hub): the stage's start position and
 * facing go through `pl_warp_start` with the player's chunk offset.  GUESS name from that use. */
void quest_warp_hub(void);

/* The free-hunt monster set (0x803B744C..0x803B9384, `enemy/em_set_work.h`): the entry and work setup, the
 * area set files, the rotation, the field setup and area entry, the kill tally and the stored result.  GUESS
 * names from the bodies. */
struct EmSetEntry;
struct EmSetSaveRec;
struct Q_MoveWork;
void em_set_entry_rec_apply(struct EmSetEntry* entry, struct EmSetSaveRec* rec);
void em_set_work_init(void);
void em_set_live_count(void);
void em_set_area_files_load(struct Q_MoveWork* work, s32 map);
void em_set_entries_spawn(struct Q_MoveWork* work);
void em_set_rotation_step(struct Q_MoveWork* work);
void em_set_start(void);
void em_set_field_setup(struct Q_MoveWork* work);
void quest_clock_real_step(s32 mode);
s32 em_set_move_start(struct Q_MoveWork* work);
s32 em_set_area_enter(struct Q_MoveWork* work);
void em_set_frame_step(struct Q_MoveWork* work);
s32 em_set_reward_item_add(u16 item);
void em_set_kill_record(struct _ENEMY_WORK* enemy, u8 mini);
s32 em_set_userdata_flag_ck(void);
void em_set_result_store(u8 end_kind);
void em_set_result_restore(void);
void em_set_enemy_detach(struct _ENEMY_WORK* enemy);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_POP_H */
