/* The enemy-side work record (`get_move_work_adrs(3)`, 0xB18 apart: `mulli r30,r3,2840` at 0x800CFA0C in `move_work_kind_alloc`,
 * `ef/system_core.cpp`), the one shared home for `_ENEMY_WORK`: the union
 * of the field sets the enemy units measure, every field at its offset and named from its use, untouched bytes kept as
 * `unused_0xNN`.  `pos` is `nw4r::math::VEC3`, so the header is C++-only.
 */
#ifndef MHTRI_ENEMY_ENEMY_WORK_H
#define MHTRI_ENEMY_ENEMY_WORK_H

#include "types.h"
#include "nw4r/math.h"
#include "ef/cp_vector.h" /* _CP_VECTOR (rule 1) */

/* The sound-work record (`sound/se.h`) whose handle the `enemy` band's action steps hand
 * `se_req_pos_ps`/`shell_se_req`; forward-declared so this header stays light (rule 4's 0xB14). */
struct _se_w;
/* The player work record `_ENEMY_WORK::plw_0x32C`/`plw_0xA34` point at (`pl.h` owns its
 * definition; this header only needs the pointer's type, so it forward-declares it). */
struct _PLW;

/* One 0x03-byte entry of the enemy program table `_ENEMY_WORK::prog_0xA00` points at (`fn_8013F764`
 * indexes it with a stride of 3: `entries[count - 1 - index]`, or `entries[index]` when the
 * `field_0x9FD` bit is clear).
 * size: 0x03 */
struct EmProgEntry {
    /* +0x0 */ u8 code;       /* `em_route_entry_get`'s lookup key */
    /* +0x1 */ u8 field_0x01; /* the `fn_8013C36C` argument the state-1 branch hands over */
    /* +0x2 */ u8 field_0x02; /* the one the state-2 branch hands over */
};

/* The header of that program table: the entry count and the array `fn_8013F764` walks.
 * size: 0x08 */
struct EmProgTbl {
    /* +0x0 */ u8 count_0x00;
    /* +0x1 */ u8 unused_0x01[0x04 - 0x01];
    /* +0x4 */ EmProgEntry* entries_0x04;
};

/* One 0x08-byte `{code, sub, value}` record of the interpreter's save/restore set: the two the
 * `fn_801408B4` restores drive (`_ENEMY_WORK::rec_0x99C`/`rec_0x9E8`), the current one
 * (`_ENEMY_WORK::field_0x9DC`) and the six-entry array `_ENEMY_WORK::recs_0xAC`.
 * size: 0x08 */
struct EmCmdRec {
    /* +0x0 */ u8 code;
    /* +0x1 */ u8 sub;
    /* +0x2 */ u8 unused_0x02[2];
    /* +0x4 */ s32 value;
};

/* The storage of one of the enemy's two attack hit records: a `_HIT_W` (`Pl/hit_w.h`) whose last 4 bytes overlap the
 * next record, 0x5C apart (`em_hit_window_set`, `enemy/em_common.cpp`).
 * size: 0x5C */
struct EmHitSlot {
    union {
        /* +0x00 */ u8 bytes[0x5C];
        /* the fields `enemy/em032_half_hp_ck.cpp`'s live-attack search reads */
        struct {
            /* +0x00 */ u8 unused_0x00[0x05];
            /* +0x05 */ u8 active_0x05;    /* 1 while the attack record is live */
            /* +0x06 */ u8 unused_0x06[0x38 - 0x06];
            /* +0x38 */ f32 value_0x38;    /* tested against 0.0 for an enemy's own records */
            /* +0x3C */ f32 power_0x3C;    /* tested above 0.0 */
            /* +0x40 */ u8 unused_0x40[0x5C - 0x40];
        };
    };
};

/* The 0xB20-byte records `get_move_work_adrs(2)` hands back (`fn_8013FD98` steps a whole record at a
 * time and asks `pl_act_grabbed_ck` whether the candidate is the one it wants).  Only the bytes the search
 * and the distance test read are named; the rest is the record's own layout, not this unit's
 * business.
 * size: 0xB20 */
struct EmAreaWork {
    /* +0x000 */ u8 active;
    /* +0x001 */ u8 unused_0x001[0x008 - 0x001];
    /* +0x008 */ u8 field_0x008;      /* the kind `em_pl_slot_leaving_ck` classifies */
    /* +0x009 */ u8 unused_0x009[0x016 - 0x009];
    /* +0x016 */ u8 area_no;
    /* +0x017 */ u8 unused_0x017[0x03C - 0x017];
    /* +0x03C */ nw4r::math::VEC3 vec_0x3C; /* one position, two readers: the record position
                                      * `fn_801B45B0` measures the seat records against
                                      * (`enemy/em034_prog.cpp`) and the one
                                      * `enemy/em030_prog.cpp`'s `fn_801B06D4` passes to
                                      * `vec3_dist_sq` as its `(void*, VEC3*)` second argument */
    /* +0x048 */ u8 unused_0x048[0x484 - 0x048];
    /* +0x484 */ EmHitSlot hit_slots_0x484[2];  /* the record's two attack hit records */
    /* +0x53C */ u8 unused_0x53C[0xB20 - 0x53C];
};

/* One 0x06-byte record of the eight-part table `_ENEMY_WORK::parts_0x838`: the damage level a part has
 * reached and the two values the net sync carries with it.
 * size: 0x06 */
struct EmPartRec {
    /* +0x0 */ u8 damage_level;
    /* +0x1 */ u8 field_0x01;
    /* +0x2 */ s16 value_0x02;
    /* +0x4 */ s16 value_0x04;
};

/* The 0x20-byte ground/seat record `em_ground_rec_clear` prepares and `em_ground_rec_find` fills: both take it as
 * a `void* out` (their declarations are in `enemy/fn_801251D0.h` and
 * `enemy/enemy_control.h`), so it is the CALLERS' record and the two units that own one
 * share this one definition (docs/plan.md 6.5 rule 1): `enemy/em034_prog.cpp` names the seat flags
 * at +0x03 and the two trailing words, `enemy/em030_prog.cpp`'s em030 program passes its address
 * and reads `pos_0x08`.  Only the fields those two read are named.
 * size: 0x20 */
struct EmGroundRec {
    /* +0x00 */ u8 field_0x00;    /* the enemy data entry */
    union {
        /* +0x01 */ u8 unused_0x01[2];
        struct {
            /* +0x01 */ u8 area_0x01;     /* the area the record places in */
            /* +0x02 */ u8 mode_0x02;     /* the placement mode (`_ENEMY_WORK::field_0x00C`) */
        };
    };
    /* +0x03 */ u8 field_0x03;   /* the seat flags `fn_801B4C54` tests (bit 1); the team `enemy_work_ground_set` copies */
    union {
        /* +0x04 */ u32 field_0x04;
        struct {
            /* +0x04 */ u8 field_0x04_b;  /* `_ENEMY_WORK::field_0x010` */
            /* +0x05 */ u8 field_0x05;    /* `_ENEMY_WORK::frames_index_0x015` */
            /* +0x06 */ u8 field_0x06;    /* the area record's +0x15 byte */
            /* +0x07 */ u8 field_0x07;
        };
    };
    /* +0x08 */ nw4r::math::VEC3 pos_0x08;
    union {
        struct {
            /* +0x14 */ u32 field_0x14;
            /* +0x18 */ u32 field_0x18;
            /* +0x1C */ u32 field_0x1C;
        };
        /* +0x14 */ _CP_VECTOR rot_0x14;  /* the rotation `eft_rot_vec_copy` copies */
    };
};

/* One 0x04-byte `{code, value}` action record of the list `fn_80126494` returns the head of
 * (`fn_801409C8` scans it for the action `_ENEMY_WORK::field_0x7C8` names).
 * size: 0x04 */
struct EmActRec {
    /* +0x0 */ u8 code;
    /* +0x1 */ u8 unused_0x01;
    /* +0x2 */ s16 value;
};

/* The file row `em_res_file_row_copy` copies (the 19 words of the table `res_mem_index_get` indexes).  The unit
 * only copies the record - it reads none of its fields - so they stay `unused_0xNN`, at the offsets
 * the target's word-by-word copy names.
 * size: 0x4C */
/* The 0x4C-byte file row `em_res_file_row_copy` copies (the table `res_mem_index_get` indexes).  This unit only
 * copies the record - it reads none of its fields - so they stay `unused_0xNN`, at the offsets the
 * target's own copy names.  The middle 0x40 bytes are 8-byte aligned on both sides, which is why the
 * target copies them two words at a time (measured: an `f64 unused_0x08[8]` member reproduces the
 * exact `lwz r5/lwz r0/stw r5/stw r0` pairs, a byte array produces a loop instead).
 * size: 0x4C */
struct EmFileRow {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ u32 unused_0x04;
    /* +0x08 */ f64 unused_0x08[8];
    /* +0x48 */ u32 unused_0x48;
};

/* One 0x08-byte `{key, name}` entry of the two file tables `em_kind_release` walks (`lbl_80581E20`'s
 * entries are loaded through `fn_80140E48`, `lbl_80582348`'s through `fn_80140FB0`).
 * size: 0x08 */
struct EmFileEntry {
    /* +0x0 */ u32 key_0x00;   /* nonzero, the row is loaded */
    /* +0x4 */ char* name_0x04;
};

/* One 0x14-byte per-motion slot of `_ENEMY_WORK::slots_0x244` (`em_motion_slot_angle_avg` averages the angles
 * `calcVecAngXY` derives from the slot's vector).
 * size: 0x14 */
struct EmMotionSlot {
    /* +0x000 */ u16 flags;            /* bit 0x800 admits the slot to the average */
    /* +0x002 */ u8 kind;              /* the route-point set `enemy/em004_act.cpp`'s nearest-point pick reads */
    /* +0x003 */ u8 unused_0x003;
    /* +0x004 */ u32 value;            /* the slot's own angle/id, filtered when the kind is 1 */
    /* +0x008 */ nw4r::math::VEC3 vec; /* `calcVecAngXY`'s input */
};

/* One delayed action of `_ENEMY_WORK::delays_0xAF0`: the frames left (negative: none), the action pair and the
 * target to restore when it fires (`em_delay_act_set`/`em_delay_act_fire`, `enemy/em_common.cpp`).
 * size: 0x14 */
struct EmDelayRec {
    /* +0x00 */ s16 timer;
    /* +0x02 */ u8 act;
    /* +0x03 */ u8 sub;
    /* +0x04 */ u8 target_kind;
    /* +0x05 */ u8 target_sub;
    /* +0x06 */ u8 target_index;
    /* +0x07 */ u8 pad_0x07;
    /* +0x08 */ nw4r::math::VEC3 target;
};

/* One of the enemy's two breakable parts at `_ENEMY_WORK::break_parts_0x488` (0x84 apart: the `mulli r,r,0x84` of
 * every reader): its step, the area it runs in, the timers, the position it flies from, the model matrix and the
 * velocity and acceleration (`em_break_parts_reset`, `em_break_part_motion_step`, `enemy/em_common.cpp`).
 * size: 0x84 */
struct EmBreakPart {
    /* +0x00 */ u8 step;            /* the part's state step, advanced as it breaks off, falls and lands */
    /* +0x01 */ u8 field_0x01;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 active;          /* set when the part starts to break off */
    /* +0x05 */ u8 hit_kind;        /* the pending break hit (1/2), or-ed into +0x836 (`em_part_hit_set`) */
    /* +0x06 */ u8 area;            /* the area it was reset in (`area_no`) */
    /* +0x07 */ u8 kind;            /* 2: no part */
    /* +0x08 */ u8 pad_0x08;
    /* +0x09 */ u8 hold;            /* the break-off step waits while it is 0 */
    /* +0x0A */ s16 timer;
    /* +0x0C */ s16 counter_0x0C;
    /* +0x0E */ s16 counter_0x0E;
    /* +0x10 */ s16 duration;       /* the frames of the flight step */
    /* +0x12 */ u8 pad_0x12[2];
    /* +0x14 */ nw4r::math::VEC3 pos;
    /* +0x20 */ nw4r::math::VEC3 origin;
    /* +0x2C */ nw4r::math::MTX34 mtx;
    /* +0x5C */ nw4r::math::VEC3 vel;
    /* +0x68 */ nw4r::math::VEC3 accel;
    /* +0x74 */ _CP_VECTOR rot;
    /* +0x80 */ s32 effect;         /* the effect handle, -1 for none */
};

struct EmPartState {
    /* +0x000 (+0x740) */ u8 field_0x740;  /* the part flag `em015_part_flag_set` writes from its argument
                                          * and `em015_user_data_node_calc` sets */
    /* +0x001 (+0x741) */ u8 pad_0x741;
    /* +0x002 (+0x742) */ s16 field_0x742; /* the six per-part counters `em015_user_data_node_calc` clears */
    /* +0x004 (+0x744) */ s16 field_0x744;
    /* +0x006 (+0x746) */ s16 field_0x746;
    /* +0x008 (+0x748) */ s16 field_0x748;
    /* +0x00A (+0x74A) */ s16 field_0x74A;
    /* +0x00C (+0x74C) */ s16 field_0x74C;
};

/* The 0xC-byte colour/step block at +0x328 this range's MHchar material steppers read as one float
 * followed by the K-colour bytes (`em015_color_step` writes them, `em015_tev_color_update` reads them back).  It is
 * the same bytes the header's other +0x328 views spell as the aim vector / handle set / action block.
 * size: 0x0C */
struct EmColorBlock {
    /* +0x000 (+0x328) */ f32 field_0x328; /* the scalar `em015_color_step` steps toward the mode target */
    /* +0x004 (+0x32C) */ u8 field_0x32C;  /* GX K-colour red   (`em015_color_step`/`em015_tev_color_update`) */
    /* +0x005 (+0x32D) */ u8 field_0x32D;  /* its green */
    /* +0x006 (+0x32E) */ u8 field_0x32E;  /* its blue */
    /* +0x007 (+0x32F) */ u8 field_0x32F;  /* the aim/approach byte `em015_state_query` kind 2 returns
                                          * and `em015_area_start_ck` gates on (== 0) */
    /* +0x008 (+0x330) */ u8 field_0x330;  /* the "special part armed" byte `em015_area_start_ck` tests and
                                          * `em015_state_query` kind 6 returns as != 0 */
    /* +0x009 (+0x331) */ u8 pad_0x331;
    /* +0x00A (+0x332) */ s16 field_0x332; /* the part-3 timer `em015_state_query` kind 4 compares
                                          * against 900 */
};

/* The 0x2C-byte block this band's `fn_801D71C4` (`enemy/em007_act.cpp`) clears at +0x328: two
 * vectors, the float after them and the two flags its spawn code sets.  The bytes are the ones
 * `enemy/em012_prog.cpp` names `field_0x328`/`field_0x32A` on the other side of the union - the
 * two bands read the same 44 bytes as different things (this band never touches the timers).
 * size: 0x2C */
struct EmActionBlock {
    union {
        /* +0x000 (+0x328) */ nw4r::math::VEC3 vec_0x328;
        struct {
            /* +0x000 (+0x328) */ u8 unused_0x328[0x008];
            /* +0x008 (+0x330) */ u8 field_0x330; /* the byte `fn_801D6D24`
                                                   * arms and `fn_801D68A8`
                                                   * reports (0/1/0xFF); it
                                                   * shares the vector's z */
        } armed_0x328;
    };
    /* +0x00C (+0x334) */ nw4r::math::VEC3 vec_0x334;
    /* +0x018 (+0x340) */ f32 field_0x340;
    /* +0x01C (+0x344) */ u8 field_0x344;
    /* +0x01D (+0x345) */ u8 field_0x345;      /* the "this record is my aim target" byte */
    /* +0x01E (+0x346) */ u8 unused_0x346[0x2C - 0x1E];
};

/* The em025 program's own block at +0x328 (`enemy/em025_prog.cpp`): the four roster slot handles and the
 * area-table states beside them, the three joint angles and the scale its joint callback applies, and the
 * blend ratio, hold timer and mode its sway step drives.  The same bytes the views above spell field by field.
 * size: 0x2C */
struct Em025Block {
    /* +0x000 (+0x328) */ s32 handles_0x328[4];  /* the roster slot handles, -1 = empty */
    /* +0x010 (+0x338) */ u8 states_0x338[4];    /* the area-table index each slot holds, 0xFF = free */
    /* +0x014 (+0x33C) */ u32 rot_y_0x33C[2];    /* the Y rotations the joint callback gives joints 23/24, eased
                                                 * toward the nearest player by the head tracking */
    /* +0x01C (+0x344) */ u32 rot_x_0x344;       /* the X rotation of joints 28/29, from the sway curve */
    /* +0x020 (+0x348) */ f32 scale_0x348;       /* the joints' 28/29 Y scale, the curve's key cursor */
    /* +0x024 (+0x34C) */ f32 blend_0x34C;       /* the sway curve position the blend mode steps */
    /* +0x028 (+0x350) */ s16 hold_0x350;        /* the frames blend mode 1 has held */
    /* +0x02A (+0x352) */ u8 blend_mode_0x352;   /* 0 = ease out, 1 = hold near 0.5, 2 = ease in */
    /* +0x02B (+0x353) */ u8 flag_0x353;         /* the flag the kind-1 state query reports */
};

/* size: 0xB18 */
/* The saved position block em035's init (`enemy/em033_prog.cpp`) keeps at `_ENEMY_WORK::run_0xB04`.
 * size: 0x10 */
struct EmRunPos {
    /* +0x00 */ nw4r::math::VEC3 pos;  /* the position saved and restored */
    /* +0x0C */ u16 angle;             /* the heading saved with it */
    /* +0x0E */ u8 flags;              /* bit 0: a saved position is present */
    /* +0x0F */ u8 valid_0x0F;          /* set once the block holds a position (`enemy/em040_ai.cpp`) */
};

struct _ENEMY_WORK {
    /* +0x000 */ u8 active;             /* nonzero while the record is in use */
    /* +0x001 */ u8 visible_0x001;          /* the quest marker shows the record only while this is set (`quest_target_color`) */
    /* +0x002 */ u8 group;              /* the enemy group/entry index */
    /* +0x003 */ u8 team;
    /* +0x004 */ u8 field_0x004;        /* `em_work_die_ck` treats values below 2 as still alive */
    /* +0x005 */ u8 state;              /* the per-motion step the update functions switch on */
    /* +0x006 */ u8 state_0x006;        /* em015_act2_hit_chain's weapon sub-state */
    /* +0x007 */ u8 state_0x007;        /* em015_act2_hit_chain's second sub-state */
    /* +0x008 */ u8 field_0x008;
    /* +0x009 */ u8 field_0x009;        /* `fn_80170610` gates the position seat on it */
    /* +0x00A */ u8 field_0x00A;        /* the area `em_area_vacant_wait_ck` matches the work records'
                                        * `area_no` (+0x1E1) and the area table's entries against */
    /* +0x00B */ u8 field_0x00B;        /* the byte `em_mot_set_blend` hands the effect queue as its flag */
    /* +0x00C */ u8 field_0x00C;        /* the mode `fn_80166DF8` gates its case-0/case-3 blocks on
                                        * (3 = the "entry seated" state) */
    /* +0x00D */ u8 field_0x00D;        /* the byte `em_quest_element_set`/`em_quest_element_set_large` latch */
    /* +0x00E */ u8 field_0x00E;        /* `fn_80137C94` hands it to `em_act_advance` */
    /* +0x00F */ u8 field_0x00F;        /* `enemy/em034_prog.cpp`'s `fn_801B4D14` gates its
                                        * seat re-test on it being zero */
    /* +0x010 */ u8 field_0x010;       /* the value `fn_80140244`'s command reports */
    /* +0x011 */ u8 field_0x011;        /* the attack timer  arms */
    /* +0x012 */ u8 field_0x012;        /* the second counter `em_act_update`'s states run */
    /* +0x013 */ u8 area_slot_0x013;     /* the area list its spawn record was filed under */
    /* +0x014 */ u8 field_0x014;        /* the mode  maps to a two-state mask */
    /* +0x015 */ u8 frames_index_0x015;  /* the frame-table index `em_frames_get` reads */
    /* +0x016 */ u8 field_0x016;        /* the first counter `em_act_update`'s states run */
    /* +0x017 */ u8 state_0x017;        /* the state `em_act_update`'s second switch dispatches on */
    /* +0x018 */ s16 field_0x018;       /* `em_frames2_get`'s joint/motion index */
    /* +0x01A */ u16 field_0x01A;       /* passed to `em_spawn_request` */
    /* +0x01C */ u16 link_order_0x01C;  /* the spawn order of the record this one is counted with (`em_spawn_request`) */
    /* +0x01E */ u8 net_seq_0x01E;   /* the message counter every enemy net sender post-increments (the step `em_net_ident_set` stores) */
    /* +0x01F */ u8 unused_0x01F[1];
    /* +0x020 */ s32 timer_0x020;       /* em015_act1_wait runs this down */
    /* +0x024 */ u8 char_0x024[0x34];   /* the embedded `MHchar` base `mhchar_model_mtx_update` takes (0x40 bytes
                                        * in this band's view; its +0x34 byte is named below) */
    /* +0x058 */ u8 model_flag_0x058;   /* set, the tail of `em_act_update` refreshes the scene model */
    /* +0x059 */ u8 unused_0x059[7];
    /* +0x060 */ u8* field_0x60;
    /* +0x064 */ u8 unused_0x064[0x068 - 0x064];
    /* +0x068 */ f32 mot_frame_0x068;   /* the embedded MHchar's motion frame `em_mot_frame_started_ck` tests above 0 */
    /* +0x06C */ u8 unused_0x06C[0x070 - 0x06C];
    /* +0x070 */ u32 mot_flags_0x070;  /* the embedded MHchar's motion flags (bit 0: `em_hit_window_set` ignores the frame shift) */
    /* +0x074 */ u16 mot_no_0x074;      /* the embedded MHchar's current motion number (`em_get_mot_no`) */
    /* +0x076 */ u8 unused_0x076[0x080 - 0x076];
    /* +0x080 */ f32 base_scale_0x080;  /* the model's base scale `get_em_base_scale` returns */
    /* +0x084 */ u8 unused_0x084[0x098 - 0x084];
    /* +0x098 */ f32 mot_frame_0x098;       /* the embedded MHchar's current motion frame (`em_mot_frame_get`) */
    /* +0x09C */ u8 unused_0x09C[0x0C8 - 0x09C];
    /* +0x0C8 */ f32 mot_frame_end_0x0C8;       /* the current motion's end frame (`em_mot_frame_end_get`) */
    /* +0x0CC */ u8 unused_0x0CC[0x110 - 0x0CC];
    /* +0x110 */ s32 joint_0x110;   /* the joint `em_area_buff_apply` places its effect on when `joint_0x204` is -1 */
    /* +0x114 */ u8 unused_0x114;
    /* +0x115 */ u8 flag_0x115;  /* set by `em_flag_0x115_set` */
    /* +0x116 */ u8 flag_0x116;  /* set by `em_flag_0x116_set` */
    /* +0x117 */ u8 unused_0x117[0x130 - 0x117];
    /* +0x130 */ struct _g3d_work* g3d_0x130;  /* the scene work `enemy_work_release` hands back with `push_g3d_wk` */
    /* +0x134 */ u8 unused_0x134[0x138 - 0x134];
    /* +0x138 */ u32 res_mdl_0x138;     /* the model resource handle (the embedded `MHchar`'s +0x114 `ResMdl`) */
    /* +0x13C */ u32 field_0x13C;       /* the scene-model id `g3d_root_model_bind` is handed */
    /* +0x140 */ u8 unused_0x140[48];
    /* +0x170 */ u8 state_0x170;
    /* +0x171 */ u8 unused_0x171[7];
    /* +0x178 */ VEC3 pos_0x178;
    /* +0x184 */ u8 unused_0x184[4];
    /* +0x188 */ nw4r::math::VEC3 pos;
    /* +0x194..0x1AC is one 0x18-byte run; the two views are a union so the run keeps its size
     * (a split without the second view's trailing pad would shrink the struct by 0x0C and
     * shift every member above it). */
    union {
        struct {
            /* +0x194 */ Vec3 prev_pos;
            /* +0x1A0 */ nw4r::math::VEC3 move_0x1A0;  /* the per-frame move vector the height helpers scale (+0x1A4 is its y,
                                                    * the fall speed `enemy/em019_ai.cpp`'s dive clears) */
        };
        /* +0x194 */ nw4r::math::VEC3 vec_0x194;  /* the second target position the 0x803253BC band
                                        * measures its aim angle against (`em_act_aim` passes it
                                        * with `pos` to `calcVecAngX2`) */
    };
    /* +0x1AC */ f32 field_0x1AC;       /* the height `em_tall_ck` compares against 0.9 * the model scale */
    /* +0x1B0 */ nw4r::math::VEC3 aim;   /* one position, two readers: the second position
                                         * `enemy/em030_prog.cpp`'s `fn_801B0810` copies the matching
                                         * record's `pos` into, and the aim point `fn_801D75D0`
                                         * copies the target record's position into (`enemy/em007_act.cpp`) */
    /* +0x1BC */ u32 field_0x1BC;       /* a rotation angle (wraps at 0x10000) */
    /* +0x1C0 */ u32 field_0x1C0;       /* the second rotation angle `em_angle_approach` steps */
    /* +0x1C4 */ u32 field_0x1C4;       /* the third rotation angle */
    /* +0x1C8 */ u32 field_0x1C8;       /* bit 0x80 suppresses `fn_8012C6F4` */
    /* +0x1CC */ f32 field_0x1CC;       /* the seat-preference weight `enemy/em034_prog.cpp`'s
                                        * `fn_801B4D14` scales by 0.85/1.3/1.1 for the seat mode
                                        * at +0x0A */
    /* +0x1D0 */ f32 field_0x1D0;       /* the scale `em_size_scale_roll` returns */
    /* +0x1D4 */ f32 field_0x1D4;        /* the alpha ratio `fn_801A9210` scales by 255 */
    /* +0x1D8 */ f32 param_target_0x1D8;  /* the value `em_motion_param_step` steps +0x1D4 towards */
    /* +0x1DC */ s16 param_steps_0x1DC;  /* the frames left in that step */
    /* +0x1DE */ u8 field_0x1DE;        /* `em_act_update`'s request-to-redraw byte */
    /* +0x1DF */ u8 height_set_0x1DF;  /* set when `em_height_set` stores +0x1AC */
    /* +0x1E0 */ u8 field_0x1E0;        /* passed to `stage_map_kind_get` (map lookup) */
    /* +0x1E1 */ u8 area_no;
    /* +0x1E2 */ u8 field_0x1E2;
    /* +0x1E3 */ u8 field_0x1E3;
    /* +0x1E4 */ u8 field_0x1E4;  /* `enemy/em025_prog.cpp`'s `fn_8019E908`: bit 0 is the * "armed" flag and the byte counts down while it is set * (`lbz`/`clrlwi ...,31` guard, then `addi -1`).  Both views agree on bit 0; bit 0 is the `fn_801A9384` kind-0 flag */
    /* +0x1E5 */ u8 action;             /* the action id `em_act_ck` matches */
    /* +0x1E6 */ u8 state_sub;          /* em015_act1_dispatch's dispatch index */
    /* +0x1E7 */ u8 field_0x1E7;        /* set to 1 once the action's entry block has run */
    /* +0x1E8 */ u8 field_0x1E8;
    /* +0x1E9 */ u8 field_0x1E9;
    /* +0x1EA */ u8 queued_act_0x1EA;  /* the action `em_act_queued_arm` re-arms */
    /* +0x1EB */ u8 queued_sub_0x1EB;  /* its sub step */
    /* +0x1EC */ u16 bits_0x1EC;      /* the 5-bit value `enemy/em016_prog.cpp`'s `fn_8018493C`
                                        * reads as `lhz` + `clrlwi ...,27` to derive its 0x96/0x5A timer */
    /* +0x1EE */ u8 field_0x1EE;
    /* +0x1EF */ u8 field_0x1EF;
    /* +0x1F0 */ u16 field_0x1F0;
    /* +0x1F2 */ u16 field_0x1F2;
    /* +0x1F4 */ u8 field_0x1F4;
    /* +0x1F5 */ u8 field_0x1F5;
    /* +0x1F6 */ s8 owner_slot_0x1F6;   /* the player slot the enemy is bound to (compared with `my_player_no`) */
    /* +0x1F7 */ u8 idle_flag_0x1F7;  /* the idle flag `em_finish_flag_set` raises and `em_idle_flag_get` reports */
    /* +0x1F8 */ u8 field_0x1F8;
    /* +0x1F9 */ u8 field_0x1F9;        /* nonzero, `fn_801400BC`'s command is skipped */
    /* +0x1FA */ u8 field_0x1FA;
    /* +0x1FB */ u8 field_0x1FB;        /* set by `em001_event_notify`'s case 9 */
    /* +0x1FC */ u8 field_0x1FC;        /* the action-record / "special part" request flag: both
                                        * `enemy/em001_prog.cpp`'s `em001_action_select` and
                                        * `enemy/em007_act.cpp`'s `fn_801DF8EC` arm it with
                                        * +0x1FE/+0x1FF */
    /* +0x1FD */ u8 field_0x1FD;        /* nonzero, `fn_8013FC60`'s command is skipped */
    /* +0x1FE */ u8 field_0x1FE;        /* the mode/timer byte `em001_action_select` arms (0x0A/0x1E) and
                                        * `fn_801DF8EC` writes (0x0A), beside +0x1FC/+0x1FF */
    /* +0x1FF */ u8 field_0x1FF;        /* the id `em001_action_select`/`fn_801DF8EC` arm beside
                                        * +0x1FC/+0x1FE */
    /* +0x200 */ s16 field_0x200;
    /* +0x202 */ s16 field_0x202;
    /* +0x204 */ u32 joint_0x204;      /* the joint `enemy/em019_ai.cpp`'s jump and dive drop from and
                                        * `em_paralysis_fx_update` hands the spark effect */
    /* +0x208 */ s32 ground_mode_0x208;  /* 1: `UpdateValue`/`em_ground_ck` lift the floor by the scaled height */
    /* +0x20C */ f32 field_0x20C;        /* one height, three readers: the effect spawn height
                                         * `fn_801D4DD8`/`fn_801D428C` add to the work record's y,
                                         * the height `fn_801A4504` compares its joint samples
                                         * against, and the height `enemy/em030_prog.cpp`'s
                                         * `fn_801B1B30` copies onto `pos.y` */
    /* +0x210 */ f32 field_0x210;       /* the second effect spawn height `enemy/em007_act.cpp`'s
                                        * `fn_801DB978` adds to the work record's y (the +0x20C
                                        * sibling is the other branch's) */
    /* +0x214 */ f32 field_0x214;      /* the effect spawn height `enemy/em009_act.cpp`'s
                                        * `fn_8038874C`/`fn_8038A7B8` add to the joint position
                                        * before `eft009_set_pos` */
    /* +0x218 */ u32 field_0x218;       /* nonzero, `em_motion_slot_angle_avg` has slots to average */
    /* +0x21C */ nw4r::math::VEC3 move_param_0x21C;  /* the move-mode parameter vector `em_move_mode_set` copies from the enemy data */
    /* +0x228 */ u16 field_0x228;      /* `enemy/em018_prog.cpp`'s `fn_8019D8B8` case 4: the low
                                        * two bits are the "blocked" gate (`lhz` + `clrlwi ...,30`). */
    /* +0x22A */ u8 water_area_0x22A;  /* bit 7: lava; bits 0-6: the water area `em_phase_get` returns */
    /* +0x22B */ u8 unused_0x22B[0x22F - 0x22B];
    /* +0x22F */ u8 cell_0x22F;        /* the stage cell colour index `enemy/em040_ai.cpp`'s tint reads */
    /* +0x230 */ u8 unused_0x230[0x23C - 0x230];
    /* +0x23C */ u8 field_0x23C;       /* the action-ready byte `enemy/em004_act.cpp`'s
                                        * `em004_state_query` case 0 reports */
    /* +0x23D */ u8 field_0x23D;        /* cleared with +0x23C when the work is set up (`em_act_advance`) */
    /* +0x23E */ u8 unused_0x23E[0x240 - 0x23E];
    /* +0x240 */ f32 ceiling_0x240;  /* the ceiling `em_ground_snap_offset_ck` holds the position under while +0x23C is set */
    /* +0x244 */ EmMotionSlot slots_0x244[10];  /* the per-motion slot set `em_motion_slot_angle_avg` averages */
    union {
        struct {
            /* +0x30C */ u8 frame_flag_0x30C;  /* the per-frame flag `em_frame_flag_set` raises */
            /* +0x30D */ u8 frame_flag_0x30D;  /* the per-frame flag `em_hide_set` raises */
            /* +0x30E */ u8 no_ceiling_0x30E;  /* set, `em_height_limit_update` skips the ceiling clamp in move mode 1 */
            /* +0x30F */ u8 no_surface_0x30F;  /* set, `em_height_limit_update` skips the water-surface clamp in move mode 2 */
            /* +0x310 */ u8 unused_0x310[0x314 - 0x310];
            /* +0x314 */ f32 field_0x314;  /* the effect scale `em001_act1_5` writes */
            /* +0x318 */ f32 field_0x318;  /* the effect radius `em001_spawn_step` writes from `em_key_curve_eval` */
        };
        /* the offset vector TWO units rotate and hand the position helpers: `enemy/em005_act.cpp`'s
         * `fn_801CC5DC` and `enemy/em030_prog.cpp`'s `fn_801B1F94`, the em030
         * program's, which rotates it with `rotVecY`; `vec_0x310.y`/`.z` are the two floats above. */
        struct {
            /* +0x30C */ u8 unused_0x30Cb[0x310 - 0x30C];
            /* +0x310 */ nw4r::math::VEC3 vec_0x310;
        } offset_0x30C;
    };
    union {
        struct {
            /* +0x31C */ u8 unused_0x31C[0x320 - 0x31C];
            /* +0x320 */ f32 field_0x320;  /* the third effect scale `fn_80167770` seeds, and the one
                                            * `enemy/em030_prog.cpp`'s `fn_801B1F94` arms beside
                                            * the +0x310 triple */
            /* +0x324 */ f32 field_0x324;  /* the effect scale `em001_spawn_step` clamps */
        };
        /* the second offset vector `enemy/em007_act.cpp`'s `fn_801D83C0` rotates (`rotVecY` on
         * +0x31C; `.y`/`.z` are the two floats above, so the two views share the bytes). */
        /* +0x31C */ nw4r::math::VEC3 vec_0x31C;
    };
    /* +0x328 */ union {
        /* the countdown/flag view `fn_80170600`/`fn_80170804` use. */
        struct {
            /* +0x328 */ s16 field_0x328;  /* the countdown `fn_80170804` ticks */
            /* +0x32A */ u8 field_0x32A;   /* the flag `fn_80170600`/`fn_80170758` clear */
            /* +0x32B */ u8 unused_0x32B[1];
            /* +0x32C */ u16 field_0x32c;
            /* +0x32E */ u8 unused_0x32E[1];
            /* +0x32F */ u8 field_0x32F;
            /* +0x330 */ u8 unused_0x330[8];
            /* +0x338 */ u8 field_0x338[4]; /* `fn_801A4504`: per-slot data-table index, 0xFF = none */
        };
        /* the `enemy/em015_prog.cpp` view of +0x328..+0x338: the s16 death timer at +0x332 */
        struct {
            /* +0x328 */ u8 unused_0x328z[0x332 - 0x328];
            /* +0x332 */ s16 timer_0x332;
            /* +0x334 */ u8 unused_0x334z[0x338 - 0x334];
        };
        /* +0x328 */ f32 float_0x328;     /* the third component of `enemy/em001_prog.cpp`'s +0x320 vector */
        /* +0x328 */ s32 handles_0x328[4]; /* `fn_801A4504`: per-slot joint-effect handles, -1 = empty */
        /* the rotation accumulator `enemy/em009_act.cpp`'s action band steps (the same 32-bit word
         * as `handles_0x328[0]`, read/written as one unsigned angle and compared against 0xC000). */
        struct {
            /* +0x328 */ u32 angle_0x328;
            /* +0x32C */ u8 unused_0x32Cb[0x338 - 0x32C];
        };
        /* `enemy/em025_prog.cpp` walks the same bytes as a four-slot handle/state set: `fn_8019ECD4`
         * arms every slot, `fn_8019EA04` releases the live ones through `fn_803B9994`. */
        struct {
            /* +0x328 */ u8 unused_0x328b[0x10];
            /* +0x338 */ u8 states_0x338[4]; /* 0xFF = free */
        };
        /* the flat field view `fn_8019ECD4` seeds and `fn_801A9384` tests (the same bytes as the
         * three views above, reached field by field). */
        struct {
            /* +0x328 */ u8 unused_0x328c[0x14];
            /* +0x33C */ s32 field_0x33C;     /* `fn_8019ECD4` clears the triple. */
            /* +0x340 */ s32 field_0x340;
            /* +0x344 */ s32 field_0x344;
            /* +0x348 */ f32 field_0x348;     /* `fn_8019ECD4` seeds it from the pool (lbl_80798528). */
            /* +0x34C */ f32 field_0x34C;     /* ditto (lbl_8079852C). */
            /* +0x350 */ u16 field_0x350;     /* `fn_8019ECD4` clears it. */
            /* +0x352 */ u8 field_0x352;      /* `fn_8019ECD4` sets it to 1. */
            /* +0x353 */ u8 field_0x353;      /* the kind-1 flag `fn_801A9384` tests; `fn_8019ECD4`
                                               * clears it */
        };
        /* the signed-short TEV view `enemy/em016_prog.cpp`'s `fn_80191038` steps: the same
         * +0x350/+0x352 bytes as the flat view above, read as `lha`/`sth` s16 colour words (the
         * +0x350 halfword is stepped by 2 and clamped, the +0x352 one set from it); a union member because the byte view
         * above already owns those bytes. */
        struct {
            /* +0x328 */ u8 unused_0x328e[0x350 - 0x328];
            /* +0x350 */ s16 tev_0x350;
            /* +0x352 */ s16 tev_0x352;
        };
        /* the float/rotation view `enemy/em001_prog.cpp`'s `em001_part_rot_update` steps (the same bytes
         * as the flat s32 view above, read as the animation angles the effect rotates by). */
        struct {
            /* +0x328 */ u8 unused_0x328d[0x08];
            /* +0x330 */ s32 field_0x330;     /* the rotation triple `em001_user_data_node_calc` hands to `cpSetRotMatrix` */
            /* +0x334 */ s32 field_0x334;
            /* +0x338 */ s32 field_0x338;
            /* +0x33C */ f32 field_0x33C;     /* the translation `em001_user_data_node_calc` gives joint 18 */
            /* +0x340 */ f32 field_0x340;
            /* +0x344 */ f32 field_0x344;
            /* +0x348 */ f32 field_0x348;     /* the translation `em001_user_data_node_calc` gives joint 10 */
            /* +0x34C */ f32 field_0x34C;
            /* +0x350 */ f32 field_0x350;
        } rot_0x328;
        /* the byte view `enemy/em003_prog.cpp`'s `em003_timers_clear` resets (the free-slot states of
         * the effect slot set). */
        struct {
            /* +0x328 */ u8 slots_0x328[0x10];
            /* +0x338 */ s16 field_0x338;
            /* +0x33A */ s16 field_0x33A;  /* the second timer `enemy/em009_act.cpp`'s
                                           * `fn_8038729C` runs down */
            /* +0x33C */ s16 field_0x33C;
        } init_0x328;
        /* the em003 program's own field view `enemy/em003_prog.cpp` reaches: the three TEV K-colours its colour
         * update eases (`em003_color_update`, `em003_tev_color_update`), the reinforcement-call state its event
         * hook and state query keep, and the hit id and distance ratio its attack steps latch.  A union member
         * because the views above already own these bytes.  size: 0x1C */
        struct {
            /* +0x328 */ u8 kcolor1_r;       /* K-colour 1 red (`setTevKColor(0, 1)`) */
            /* +0x329 */ u8 kcolor1_g;
            /* +0x32A */ u8 kcolor1_b;
            /* +0x32B */ u8 kcolor2_r;       /* K-colour 2 (`setTevKColor(0, 2)`) */
            /* +0x32C */ u8 kcolor2_g;
            /* +0x32D */ u8 kcolor2_b;
            /* +0x32E */ u8 kcolor2_a;
            /* +0x32F */ u8 kcolor3_a;       /* K-colour 3 alpha (`setTevKColor(1, 3)`) */
            /* +0x330 */ u8 call_group;      /* the enemy group the call event summons (0 = none) */
            /* +0x331 */ u8 call_done;       /* set once the call has gone out */
            /* +0x332 */ u8 call_count;      /* calls made, capped at 0xFF */
            /* +0x333 */ u8 rage;            /* set by the action-2 sub-state-6 event; state query kind 3 */
            /* +0x334 */ u8 call_kind;       /* the summoned-kind id `em_spawn_request` is handed */
            /* +0x335 */ u8 call_kind_done;  /* set once a kind-0x0A call went out (`em003_skip_ck`) */
            /* +0x336 */ u8 flag_0x336;      /* set by event 0xD/1; `em003_area_start_ck`'s area-2 gate */
            /* +0x337 */ u8 unused_0x337;
            /* +0x338 */ s16 call_timer;     /* armed to 900 with `call_group`, counted down per tick */
            /* +0x33A */ u16 hit_id;         /* the `get_hit_id` latched by the attack steps */
            /* +0x33C */ u16 tick;           /* the 0..49 frame counter `em003_frame_update` cycles */
            /* +0x33E */ u8 unused_0x33E[2];
            /* +0x340 */ f32 dist_ratio;     /* the `em_target_dist_ratio_get` ratio action 7 scales by */
        } em003_0x328; /* size: 0x1C */
        /* the action block `enemy/em007_act.cpp` clears (`fn_801D71C4`). */
        struct EmActionBlock action_0x328;
        /* the signed-short countdown view the action band `enemy/em008_prog.cpp` uses: the same
         * 0x32A bytes as one halfword, where the first view above reads a `u8` flag there
         * (`fn_8015DB68` stores 0x384 into it and `fn_8015DD6C` `lha`s it down while positive).
         * Two views of the same bytes, so a union member and not a re-typing. */
        struct {
            /* +0x328 */ u8 unused_0x328d[0x2];
            /* +0x32A */ s16 timer_0x32A;  /* armed to 0x384 at action 1's sub-state 6 */
            /* +0x32C */ u8 unused_0x32Cc[0x338 - 0x32C];
        };
        /* the float/word/short-timer view the action band of `enemy/em004_act.cpp` and `enemy/em005_act.cpp` writes and reads
         * (`fn_801CAA8C` clears 0x328/0x32C, `em004_state_query` reads the three 0x330/0x332/0x334
         * halfwords; the 0x330 byte flag `armed_0x328.field_0x330` above is the same byte). */
        struct {
            /* +0x328 */ f32 field_0x328;
            /* +0x32C */ s32 field_0x32C;
            /* +0x330 */ s16 field_0x330;
            /* +0x332 */ s16 field_0x332;
            /* +0x334 */ s16 field_0x334;
            /* +0x336 */ u8 unused_0x336j[0x354 - 0x336];
        } timer_0x328;
        /* the glow/height view `enemy/em004_act.cpp` keeps: the K-colour alpha it fades, the height bias the
         * size scale gives, and its three halfword action timers (the state query reads them). */
        struct {
            /* +0x328 */ f32 fade_0x328;
            /* +0x32C */ f32 height_bias_0x32C;
            /* +0x330 */ s16 timer_0x330;
            /* +0x332 */ s16 count_0x332;
            /* +0x334 */ s16 timer_0x334;
            /* +0x336 */ u8 unused_0x336e4[0x354 - 0x336];
        } em004_0x328; /* size: 0x2C */
        /* the em022 program's own field view `enemy/em022_prog.cpp` reaches: the jaw pulse steps and the jaw
         * angle `em022_jaw_update` steps (the joint callback applies it as an X rotation), the turn angle,
         * the sighting counter and its alert timer.  A union member because the views above already own
         * these bytes.  size: 0x2C */
        struct {
            /* +0x328 */ u8 jaw_open_0x328;    /* set while the target is in sight: the jaw eases open */
            /* +0x329 */ u8 unused_0x329em22;
            /* +0x32A */ u16 pulse_a_0x32A;    /* the fast jaw pulse step (1..4), 0 = off */
            /* +0x32C */ u16 pulse_b_0x32C;    /* the slow jaw pulse step (1..10), 0 = off */
            /* +0x32E */ u16 jaw_angle_0x32E;  /* the jaw joint's X rotation */
            /* +0x330 */ u16 turn_0x330;       /* the turn angle the side-step and circling motions aim at */
            /* +0x332 */ s16 leg_0x332;        /* the frames the strafe has spent on its current leg */
            /* +0x334 */ s16 sight_0x334;      /* frames the target has been in sight, capped at 90 */
            /* +0x336 */ s16 alert_0x336;      /* the alert timer: 90 once sighted, counted down after */
            /* +0x338 */ s8 point_0x338;      /* the area data point (6 or 7) the zone charge aims at */
            /* +0x339 */ u8 unused_0x339em22[0x354 - 0x339];
        } em022_0x328; /* size: 0x2C */
        /* the seated flag `enemy/em027_prog.cpp` keeps (its state query kind 0 reports it; the init restores it
         * from `seated_keep_0xB05`). */
        struct {
            /* +0x328 */ u8 seated_0x328;
        } em027_0x328; /* size: 0x01 */
        /* the em033 program's own block (`enemy/em033_prog.cpp`): the colour tween's step counter and its
         * wind-down vector, the roster record it hunts and the hold kind that record reports, and the body
         * colour/scale the tween writes. */
        struct {
            /* +0x328 */ s32 timer_0x328;          /* the colour tween's step (0..4) */
            /* +0x32C */ nw4r::math::VEC3 vec_0x32C; /* the climb's remaining shift, spent over the timer */
            /* +0x338 */ u8 color_step_0x338;
            /* +0x339 */ u8 color_mode_0x339;
            /* +0x33A */ u8 hold_kind_0x33A;       /* 1..4 from the record's +0x05 kind, 0 = none */
            /* +0x33B */ u8 record_0x33B;          /* the roster record hunted (0xFF = none) */
            /* +0x33C */ u8 unused_0x33C;
            /* +0x33D */ u8 hit_prev_0x33D;        /* last frame's +0x43D hit flag */
            /* +0x33E */ u8 attack_kind_0x33E;     /* the attack variant the state query reports */
            /* +0x33F */ u8 phase_0x33F;           /* the hunt phase (0 idle, 1 locked, 2 tracking, 3 held) */
            /* +0x340 */ u8 color_0x340[3];        /* the body K-colour the tween writes */
            /* +0x343 */ u8 unused_0x343;
            /* +0x344 */ u8 color_target_0x344[3]; /* the colour the tween starts from */
            /* +0x347 */ u8 unused_0x347;
            /* +0x348 */ nw4r::math::VEC3 scale_0x348; /* the body scale the tween grows */
        } em033_0x328; /* size: 0x2C */
        /* the effect-seat view `enemy/em034_prog.cpp` keeps: the selected seat record index at
         * +0x328 (0xFF = none) and its armed flag at +0x329. */
        struct {
            /* +0x328 */ u8 slot_0x328;
            /* +0x329 */ u8 flag_0x329;
            /* +0x32A */ u8 unused_0x32Af[0x354 - 0x32A];
        };
        /* this range's colour/step view (`em015_color_step`/`em015_tev_color_update`/`em015_area_start_ck`/`em015_state_query`). */
        struct EmColorBlock color_0x328;
        /* the em030 program's own field view `enemy/em030_prog.cpp` reaches field by field
         * (`plw_0x32C` is the +0x328 move that unit latched); a union member because the views above already own these
         * bytes. */
        struct {
            /* +0x328 */ u16 item_0x328;    /* the item `enemy/em030_prog.cpp`'s steal latched (0 = none) */
            /* +0x32A */ u8 unused_0x32Aem[0x32C - 0x32A];
            /* +0x32C */ _PLW* plw_0x32C;  /* the player work `fn_801B0230` latched the +0x328 move
                                            * on (NULL when nothing is latched) */
            /* +0x330 */ u8 field_0x330;   /* the value the em030 condition hook
                                            * (`em030_condition_ck`) reports for action 7 */
            /* +0x331 */ u8 field_0x331;   /* the em030 hook's own latch */
            /* +0x332 */ u8 field_0x332;   /* the em030 action-state ring 0..3 */
            /* +0x333 */ u8 field_0x333;   /* the em030 two-state toggle */
            /* +0x334 */ u8 field_0x334;   /* the em030 "in range" latch */
            /* +0x335 */ u8 unused_0x335[0x354 - 0x335];
        };
        /* the two one-shot latches `enemy/em035_prog.cpp`'s action-11 effect trigger and its
         * K-colour helper set: the same +0x32C/+0x32D bytes the views above read as a word/colour,
         * reached here as the two bytes the object's `lbz`/`stb` pair touches. */
        struct {
            /* +0x328 */ u8 unused_0x328fm[0x32C - 0x328];
            /* +0x32C */ u8 effect_latch_0x32C;  /* set once the action-11 trigger has run */
            /* +0x32D */ u8 kcolor_latch_0x32D;  /* set once the K-colour override has run */
            /* +0x32E */ u8 unused_0x32Efm[0x354 - 0x32E];
        };
        /* the UV-model slot view `enemy/em032_half_hp_ck.cpp` keeps: the slot index
         * `em032_slots_clear` clears, the "armed" flag cleared beside it, and the "model created" flag
         * `em032_event_notify` arms for the two idle modes and `em032_state_byte_get` reads back by index.  A union
         * member because the views above already own these bytes. */
        struct {
            /* +0x328 */ u8 slot_0x328;
            /* +0x329 */ u8 armed_0x329;
            /* +0x32A */ u8 unused_0x32A[0x330 - 0x32A];
            /* +0x330 */ nw4r::math::VEC3 pos_0x330;  /* the position the charge step measured from */
            /* +0x33C */ u8 unused_0x33C[0x33E - 0x33C];
            /* +0x33E */ u8 created_0x33E;  /* the model has been placed for this slot */
            /* +0x33F */ u8 unused_0x33F[0x354 - 0x33F];
        } uv_model_0x328;
        /* the per-part value run `ef/eft052.cpp` reads: the four
         * halves at +0x32C/+0x32E/+0x330/+0x332 are loaded `lha` and stored `sth` there, one per
         * part index, and `eft052_part_gauge_add` steps +0x32C and clamps it to 0..500.  A union member
         * because the byte/pointer views above already own those bytes. */
        struct {
            /* +0x328 */ u8 unused_0x328pv[0x32C - 0x328];
            /* +0x32C */ s16 part_value_0x32C;
            /* +0x32E */ s16 part_value_0x32E;
            /* +0x330 */ s16 part_value_0x330;
            /* +0x332 */ s16 part_value_0x332;
        } part_values_0x32C;
        /* the em020 program's own field view `enemy/em020_prog.cpp` reaches:
         * the +0x334/+0x336 halfword pair its init clears, the +0x338 per-frame latch and the
         * +0x33E countdown `em020_timers_tick` ticks, and the +0x339/+0x33A/+0x33B triple
         * `em020_glow_color_get` fills.  A union member because the views above already own these bytes. */
        struct {
            /* +0x328 */ nw4r::math::VEC3 home_0x328; /* the target position the circling charge saves and restores */
            /* +0x334 */ s16 timer_0x334;   /* the per-frame countdown `em020_timers_tick` ticks */
            /* +0x336 */ u16 value_0x336;   /* the stage light id the reset slot takes, which the light update publishes */
            /* +0x338 */ u8 latch_0x338;    /* "map 7 and area 3" for this frame */
            /* +0x339 */ u8 sel_0x339;      /* out of the `em020_glow_color_get` lookup */
            /* +0x33A */ u8 sel_0x33A;
            /* +0x33B */ u8 sel_0x33B;
            /* +0x33C */ u8 flag_0x33C;
            /* +0x33D */ u8 pad_0x33D;
            /* +0x33E */ s16 timer_0x33E;   /* the second countdown, gated on the byte stack */
            /* +0x340 */ u8 unused_0x340gt[0x354 - 0x340];
        } em020_0x328;
        /* the same three bytes read as the material K-colour triple `enemy/em020_prog.cpp`'s
         * `em020_model_refresh` copies into the 6/3 TEV K-colour slot one byte at a time
         * (`lbz`/`stb` per component, the alpha byte left as the getter returned it).  A second
         * view of the bytes `em020_0x328.sel_0x339..0x33B` holds, which is why it is a union
         * member and not a field: the em020_prog side names them as the lookup result it stores,
         * this side as the colours it publishes. */
        struct {
            /* +0x328 */ u8 unused_0x328kc[0x339 - 0x328];
            /* +0x339 */ u8 kcolor_r_0x339;
            /* +0x33A */ u8 kcolor_g_0x33A;
            /* +0x33B */ u8 kcolor_b_0x33B;
            /* +0x33C */ u8 unused_0x33Ckc[0x354 - 0x33C];
        } em020_kcolor_0x328;
        /* the em023 program's state bytes (`lobby/fn_802FA9A0.cpp`): the rise flag event 5 latches, the route leg
         * the action-12 walk is on, the enemy-data slot state, the sync and arrival flags, the two look headings
         * and the action-11 timer.  size: 0x0C */
        struct {
            /* +0x328 */ u8 flag_0x328;        /* set by event 5 sub 19, cleared by sub 14 */
            /* +0x329 */ s8 leg_0x329;         /* the route leg (`em023_route_step`) */
            /* +0x32A */ u8 slot_state_0x32A;  /* 0 waiting, 1 latched, 2 standing in the slot's shape */
            /* +0x32B */ u8 synced_0x32B;
            /* +0x32C */ u8 arrived_0x32C;
            /* +0x32D */ u8 pad_0x32D;
            /* +0x32E */ u16 aim_x_0x32E;      /* the look heading `em023_move_look` turns to */
            /* +0x330 */ u16 aim_y_0x330;      /* its pitch */
            /* +0x332 */ s16 timer_0x332;      /* the action-11 frames the surface clamp eases in over */
        } em023_0x328; /* size: 0x0C */
        /* the em029 program's hit pair (`enemy/em029_prog.cpp`): the flag the busy action holds this
         * frame and last frame's copy the per-frame slot shifts into it.  size: 0x2C */
        struct {
            /* +0x328 */ u8 hit_0x328;
            /* +0x329 */ u8 hit_prev_0x329;
            /* +0x32A */ u8 unused_0x32Aem029[0x354 - 0x32A];
        } em029_0x328; /* size: 0x2C */
        /* the em024 monster program's own field view `enemy/em024_ai.cpp` reaches: the three state bytes, the four
         * `s16` gauge/timer words and the two position vectors the program latches.  A union member because the views above already own these
         * bytes. */
        struct {
            /* +0x328 */ u8 part_lock_0x328;   /* 1 while the six alternate part records are armed */
            /* +0x329 */ u8 tev_ramp_0x329;    /* 1 while the TEV colors ramp up, 0 while they fade out */
            /* +0x32A */ u8 flag_0x32A;    /* the swing-window flag the event handler sets */
            /* +0x32B */ u8 pad_0x32B;
            /* +0x32C */ s16 gauge_0x32C;  /* the part gauge (`eft052_part_gauge_add`), 500 at rest */
            /* +0x32E */ s16 timer_0x32E;  /* the state-1 countdown */
            /* +0x330 */ s16 timer_0x330;  /* the state-0 countdown */
            /* +0x332 */ s16 hold_timer_0x332; /* the 150-frame countdown the scatter action arms */
            /* +0x334 */ nw4r::math::VEC3 tev_color_0x334;   /* RGB written to TEV konst slots 0, 4 and 1 (GUESS: the main glow) */
            /* +0x340 */ nw4r::math::VEC3 tev_color2_0x340;  /* RGB written to TEV konst slots 2 and 3 (GUESS: the secondary glow) */
            /* +0x34C */ u8 unused_0x34Cem[0x354 - 0x34C];
        } em024_0x328;
        /* the em017 program's alert pair (`enemy/em016_prog.cpp`): the flag its event hook latches for event kind 0xA
         * and the "a ranked ally is near" flag its per-frame area scan sets (both GUESS names).  size: 0x2C */
        struct {
            /* +0x328 */ u8 alert_0x328;
            /* +0x329 */ u8 ally_near_0x329;
            /* +0x32A */ u8 unused_0x32Aem017[0x354 - 0x32A];
        } em017_0x328; /* size: 0x2C */
        /* the em025 program's block (`enemy/em025_prog.cpp`). */
        struct Em025Block em025_0x328;
        /* the em040 program's own fields (`enemy/em040_ai.cpp`): the tint latch, the escape flag, the call timers,
         * the leader it follows and the leap's landing and peak points.  The leap's drift vector starts at +0x34C
         * and runs on into the +0x354 word, so the view keeps its first two floats and the unit reads it as a
         * vector there.  A union member because the views above already own these bytes.  size: 0x2C */
        struct {
            /* +0x328 */ u8 tint_done_0x328;      /* set once the water tint has been applied */
            /* +0x329 */ u8 escape_0x329;         /* the landing point is out of reach: leave through action 11 */
            /* +0x32A */ s16 call_timer_0x32A;    /* the frames left to wait for the leader's call (1800) */
            /* +0x32C */ s16 call_delay_0x32C;    /* the staggered delay before answering, -1 = not yet ranked */
            /* +0x32E */ u8 call_state_0x32E;     /* 0 waiting, 1 called, 2 answered */
            /* +0x32F */ u8 se_alt_0x32F;         /* picks the second landing sound */
            /* +0x330 */ struct _ENEMY_WORK* leader_0x330;  /* the live kind-9 work found at init */
            /* +0x334 */ nw4r::math::VEC3 land_0x334;   /* where the leap ends */
            /* +0x340 */ nw4r::math::VEC3 peak_0x340;   /* the leap's top point */
            /* +0x34C */ f32 drift_0x34C[2];            /* the body's offset from the landing point (x, y; z is +0x354) */
        } em040_0x328; /* size: 0x2C */
    };
    /* +0x354 */ union {
        /* the s16 view `em001_frame_update` counts and the byte view `em001_event_notify` flags - the same four
         * bytes, in the two spellings the reconstructed units use (rule 1: one home). */
        struct {
            /* +0x0 */ s16 field_0x354;   /* the 50-frame action counter `em001_frame_update` ticks and wraps */
            /* +0x2 */ u8 field_0x356;    /* the action's "hold" flag `em001_event_notify` arms/clears */
            /* +0x3 */ u8 field_0x357;    /* the action's case-6 flag `em001_event_notify` sets */
        };
        /* the 32-bit action bit set `enemy/em025_prog.cpp` clears (`fn_8019E948`), sets
         * (`fn_8019E960`) and tests (`fn_8019E9AC`): `slot = id / 8` clamped to 3,
         * `mask_0x354[slot] |= 1 << (id % 8)`. */
        u8 mask_0x354[4];
        /* the s16 TEV pair view `enemy/em016_prog.cpp`'s `fn_80191038` steps (`lha`/`sth` at
         * +0x354/+0x356, the same bytes as `field_0x354`/`field_0x356` above). */
        struct {
            /* +0x0 */ s16 tev_0x354;
            /* +0x2 */ s16 tev_0x356;
        };
    };
    /* +0x358 */ union {
        struct {
            /* +0x358 */ u8 field_0x358;       /* the action's "run" flag `em001_event_notify` arms/clears */
            /* +0x359 */ u8 field_0x359;       /* `enemy/em018_prog.cpp`'s `fn_801994F4` sets it with
                                                * +0x35B at the two state-0/1 action starts. */
            /* +0x35A */ u8 unused_0x35A;
            /* +0x35B */ u8 field_0x35B;       /* the same action-start pair as +0x359. */
            /* +0x35C */ u8 field_0x35C;       /* the two action-completion flags `fn_8019E398`
                                                * sets/clears (bit 0) and bit 1, and `fn_8019D8B8`
                                                * cases 1/2 test. */
            /* +0x35D */ u8 unused_0x35D[0x360 - 0x35D];
        };
        /* the signed-short TEV view `enemy/em016_prog.cpp`'s `fn_80191038`/`fn_801913FC` step
         * (`lha`/`sth` at +0x358/+0x35A/+0x35C, the same bytes as the byte view above; +0x35E
         * is the shadow flag the same unit arms). */
        struct {
            /* +0x358 */ s16 tev_0x358;
            /* +0x35A */ s16 tev_0x35A;
            /* +0x35C */ s16 tev_0x35C;
            /* +0x35E */ u8 tev_0x35E;
            /* +0x35F */ u8 tev_0x35F;     /* cleared beside `tev_0x35E` by `enemy/em016_prog.cpp`'s init */
        };
    };
    /* +0x360 */ s16 field_0x360;      /* fn_8013FD98's "already in this mode" countdown */
    /* +0x362 */ s16 field_0x362;      /* `enemy/em018_prog.cpp`'s `fn_8019D8B8` case 7: `<= 0`
                                        * answers 1 (`lha` + `cmpwi 0` + `bgt`). */
    /* +0x364 */ u8 unused_0x364[0x368 - 0x364];
    /* +0x368 */ f32 fall_height_0x368;  /* the fall height `em_fall_height_latch` copies from the enemy data */
    /* +0x36C */ nw4r::math::VEC3 vec_0x36C;  /* the target position `calcVecDistXZ` measures against
                                        * `pos` (`fn_8013F764`'s distance test) */
    /* +0x378 */ f32 value_0x378;       /* em015_act2_drift/em015_act2_drift_long clamp this against a pool float */
    /* +0x37C */ u32 field_0x37C;       /* the rotation word `fn_801CEF44` latches from `field_0x1C0`
                                        * and hands `em_move_offset_rot_apply` (`enemy/em005_act.cpp`), and the
                                        * angle `enemy/em030_prog.cpp`'s `fn_801B2298` computes with
                                        * `calcVecAngXY` and steps through `em_angle_approach` */
    /* +0x380 */ u8 field_0x380;        /* the "special part armed" selector `fn_801D6758` matches
                                         * against 1 (`enemy/em005_act.cpp`), and the em030
                                         * homing/action state `enemy/em030_prog.cpp` keeps here */
    /* +0x381 */ u8 state_0x381;        /* the picker mode `enemy/em030_prog.cpp`'s em030 hands to
                                         * `em_move_work_pick`, and the record index
                                         * `enemy/em005_act.cpp`'s `fn_801D6758` hands to the same
                                         * function */
    /* +0x382 */ u8 field_0x382;        /* `fn_8013FD1C` writes it from the stream; 0xFF means
                                         * "no special part", which `fn_801D6758` rejects */
    /* +0x383 */ u8 field_0x383;        /* set once `em_route_entry_get`'s record is latched */
    /* +0x384 */ f32 field_0x384;       /* the record's +0x04 float, copied in with it */
    /* +0x388 */ u16 field_0x388;
    /* +0x38A */ u8 field_0x38A;
    /* +0x38B */ u8 field_0x38B;       /* `em001_event_notify`'s per-step flag (cleared on entry) */
    /* +0x38C */ u8 field_0x38C;        /* bit 0 gates `fn_8012C870` */
    /* +0x38D */ u8 field_0x38D;        /* `fn_80137C9C`'s action-end block clears it */
    /* +0x38E */ u8 field_0x38E;        /* `fn_80137C9C`'s action-end block clears it */
    /* +0x38F */ u8 field_0x38F;        /* the per-attacker bit set `em_act_pick` masks against */
    /* +0x390 */ s32 values_0x390[5];   /* index 4 is `fn_8012C204` */
    union {
        struct {
        /* +0x3A4 */ s32 values_0x3A4[37];  /* the chosen attacker's damage */
        };
        /* the byte/half/word view of +0x420..+0x428 `fn_8012BA00`'s cost code and `enemy/em_kind.cpp`'s timer code read */
        struct {
            /* +0x3A4 */ u8 unused_0x3A4[0x420 - 0x3A4];
            /* +0x420 */ u8 flag_0x420;
            /* +0x421 */ u8 unused_0x421;
            /* +0x422 */ s16 count_0x422;
            /* +0x424 */ u32 timer_0x424;
            /* +0x428 */ s32 hate_0x428[4];  /* per player, the hate `em_hate_add` keeps in [0, 20000] */
        };
    };
    /* +0x438 */ u8 spotted_0x438;  /* the players `em_player_spotted_set` has marked, one bit each */
    /* +0x439 */ u8 field_0x439;
    /* +0x43A */ u8 field_0x43A;
    /* +0x43B */ u8 field_0x43B;        /* the per-motion kind gate `em_motion_blend_scale_get` tests against 2 */
    /* +0x43C */ u8 field_0x43C;        /* the second half of the em030 state pair `fn_801B0A28`
                                        * dispatches on (named by `enemy/em030_prog.cpp`) */
    /* +0x43D */ u8 field_0x43D;
    /* +0x43E */ u8 field_0x43E;
    /* +0x43F */ u8 field_0x43F;        /* `em_roar_latch_set` sets it, `fn_80137648` reads it back */
    /* +0x440 */ s16 field_0x440;    /* the action frame counter `enemy/em034_prog.cpp`'s
                                        * `fn_801B47A4` tests against 0x1C2 and `enemy/em030_prog.cpp`'s
                                        * em030 `fn_801B0A28` against 300/1800 */
    /* +0x442 */ s16 status_timer_0x442;  /* the frames the +0x43D status has run */
    /* +0x444 */ s16 sight_timer_0x444[4];  /* per player, the frames its sight mark (+0x38F) holds */
    /* +0x44C */ s16 value_0x44C;
    /* +0x44E */ s16 value_0x44E;
    /* +0x450 */ s16 field_0x450;       /* the first of the two `lha` gates `enemy/em007_act.cpp`'s
                                        * `fn_801DF8EC` compares (+0x452 is its sibling) */
    /* +0x452 */ s16 value_0x452;       /* the 0x384-frame gate `fn_801D6DA4` tests */
    union {
        struct {
        /* +0x454 */ u8 unused_0x454[0x464 - 0x454];
        /* +0x464 */ f32 field_0x464;      /* the motion parameter `em001_act3_4` hands to `em_mot_layer_blend_step` */
        /* +0x468 */ u16 sync_flag_0x468;   /* cleared by every enemy net send and receive */
        /* +0x46A */ u8 freeze_0x46A;  /* set, `em_timers_update` runs only the motion and shake timers */
        /* +0x46B */ s8 event_slot_0x46B;  /* the enemy-control event target it was matched to, -1 = none */
        /* +0x46C */ u8 field_0x46C;       /* the area/entry byte `fn_80170804` latches to 0xFF */
        /* +0x46D */ u8 state_0x46D;
        /* +0x46E */ u8 state_0x46E;
        /* +0x46F */ u8 state_0x46F;
        /* +0x470 */ nw4r::math::VEC3 pos_offset_0x470;   /* the offset the target message adds when the enemy changes area */
        };
        /* `fn_8012BA00`'s view of +0x454..+0x474 */
        /* +0x454 */ f32 frames_0x454[8];  /* the per-part frame cost `fn_8012BA00`'s distance test reads */
    };
    /* +0x47C */ u32 approach_yaw_0x47C;  /* the fixed approach yaw `em_approach_start` latches with flag 8 */
    /* +0x480 */ u16 approach_flags_0x480;  /* the `em_approach_start` flags `em_approach_step` steers by */
    /* +0x482 */ u8 field_0x482;       /* nonzero picks the second approach float in
                                        * `enemy/em016_prog.cpp`'s `fn_80184CE0` */
    /* +0x483 */ u8 turn_flags_0x483;  /* the `em_turn_seq_start` flags `em_turn_seq_step` and `em_turn_apply` read */
    /* +0x484 */ struct EmUserDataObj* user_data_0x484;  /* the user-data accessor `enemy_work_release` deletes */
    union {
        struct {
            /* +0x488 */ u8 unused_0x488[0x48E - 0x488];
            /* +0x48E */ u8 field_0x48E;        /* the part-kind byte `enemy/em007_act.cpp`'s `fn_801DF2F8`
                                                * compares against 4 and 6 */
            /* +0x48F */ u8 field_0x48F;      /* the part-damage special-case byte `em015_tev_color_update` tests
                                                * against 1 before painting the 0x1E4 colour white */
            /* +0x490 */ u8 unused_0x490[0x491 - 0x490];
            /* +0x491 */ u8 field_0x491;      /* `enemy/em008_prog.cpp`'s `fn_8015DB68` sets it at action
                                              * 0xA's sub-state 0xC8 */
            /* +0x492 */ u8 unused_0x492[0x590 - 0x492];
        }; /* size: 0x108 */
        /* +0x488 */ EmBreakPart break_parts_0x488[2];  /* the two breakable parts (`em_break_parts_reset`) */
    };
    /* +0x590 */ u8 unused_0x590[0x5C4 - 0x590];
    /* +0x5C4 */ u8 flags_0x5C4;
    /* +0x5C5 */ u8 unused_0x5C5[67];
    /* +0x608 */ u32 field_0x608;      /* the X rotation word `fn_801661FC` hands `rotMatrixX` */
    /* +0x60C */ u8 unused_0x60C[0x610 - 0x60C];
    /* +0x610 */ u32 field_0x610;      /* the Z rotation word the same function hands `rotMatrixZ` */
    /* +0x614 */ u8 unused_0x614[0x740 - 0x614];
    /* +0x740 */ struct EmPartState part_0x740; /* the per-part flags/counters `em015_part_flag_set`/
                                        * `em015_user_data_node_calc` write */
    /* +0x74E */ u8 unused_0x74E[0x750 - 0x74E];
    /* +0x750 */ u32 field_0x750;  /* cleared by `em_record_0x750_clear` */
    /* +0x754 */ u32 field_0x754;  /* cleared by `em_record_0x750_clear` */
    /* +0x758 */ u32 field_0x758;  /* cleared by `em_record_0x750_clear` */
    /* +0x75C */ s32 field_0x75C;  /* set to -1 by `em_record_0x750_clear` */
    /* +0x760 */ u8 field_0x760;  /* cleared by `em_record_0x750_clear` */
    /* +0x761 */ u8 field_0x761;       /* one byte, three readers: `enemy/em025_prog.cpp`'s slot bit
                                        * map (`fn_8019EA04` clears it, `fn_8019EA80` scans its low
                                        * 8 bits for free slot indices), `enemy/em005_act.cpp`'s
                                        * `fn_801D4F78` (bit 0 the "aim target live" flag it mirrors
                                        * from `em_alt_mode_ck`/the area test, bit 1 its one-shot latch),
                                        * and `enemy/em030_prog.cpp`'s em030 action bitmap (bit 0x01
                                        * the running flag `fn_801B0A28` arms, 0x10/0x20/0x40/0x80 the
                                        * exclusive motion-state bits). */
    /* +0x762 */ u8 field_0x762;       /* `enemy/em005_act.cpp`'s latch `fn_801D4F78` checks before
                                        * setting bit 1, and the em030 motion flag
                                        * `enemy/em030_prog.cpp`'s `fn_801B055C` state ring reads */
    /* +0x763 */ u8 unused_0x763[0x764 - 0x763];
    /* +0x764 */ s16 latch_timer_0x764;  /* the +0x762 latch's timer (`em_latch_0x762_roll`) */
    /* +0x766 */ u8 field_0x766;        /* cleared when the work is set up (`em_act_advance`) */
    /* +0x767 */ u8 unused_0x767;
    /* +0x768 */ s16 field_0x768;       /* cleared when the work is set up (`em_act_advance`) */
    /* +0x76A */ u8 unused_0x76A[0x76C - 0x76A];
    /* +0x76C */ nw4r::math::VEC3 vec_0x76C; /* the position `calcVecAngX` is handed in
                                         * `fn_801D4CE0` */
    /* +0x778 */ nw4r::math::VEC3 vec_0x778;  /* zeroed with `vec_0x76C` when the work is set up (`em_act_advance`) */
    /* +0x784 */ u8 field_0x784;
    /* +0x785 */ u8 approach_3d_0x785;  /* set while the approach runs in 3D (`em_hit_arm_set`) */
    /* +0x786 */ u8 fly_state_0x786;  /* 0 while the flight heads for its target, 1 once past it (`em_fly_step`) */
    /* +0x787 */ u8 unused_0x787[0x788 - 0x787];
    /* +0x788 */ nw4r::math::VEC3 fly_start_0x788;  /* the position the flight started from (`em_fly_start`) */
    /* +0x794 */ u16 field_0x794;
    /* +0x796 */ s16 field_0x796;
    /* +0x798 */ u8 field_0x798;
    /* +0x799 */ u8 field_0x799;
    /* +0x79A */ u8 field_0x79A;
    /* +0x79B */ u8 field_0x79B;
    /* +0x79C */ s16 shake_timer_0x79C;  /* the camera shake request `em_shake_req_set` arms */
    /* +0x79E */ u8 unused_0x79E[0x7A0 - 0x79E];
    /* +0x7A0 */ u32 field_0x7A0;       /* the numerator of the ratio `fn_8014001C` reports */
    /* +0x7A4 */ u32 field_0x7A4;       /* its denominator */
    /* +0x7A8 */ f32 hp_ratio_0x7A8;   /* the share of `field_0x7A4` the work starts with (capped at 1.0, `em_act_advance`) */
    /* +0x7AC */ u32 field_0x7AC;       /* the second frame counter `em_team_damage_over_ck` measures against `field_0x7A0` */
    /* +0x7B0 */ f32 field_0x7B0;       /* the `team == 0xf` threshold `em_damage_area_ok_ck` tests against 0.0 */
    /* +0x7B4 */ f32 shell_rate_0x7B4;  /* the factor `shell_attack_set` scales a shell's hit life by */
    /* +0x7B8 */ f32 field_0x7B8;  /* the second rank rate `em_shell_rate_init` sets */
    /* +0x7BC */ f32 field_0x7BC;       /* the effect radius `em_motion_blend_scale_get` scales */
    /* +0x7C0 */ f32 field_0x7C0;       /* its second factor */
    /* +0x7C4 */ f32 field_0x7C4;       /* the value `em_mot_set_blend` clears to 1.0f */
    /* +0x7C8 */ u8 field_0x7C8;        /* the action id `em_stat_level_roll` picks */
    /* +0x7C9 */ u8 unused_0x7C9[0x801 - 0x7C9];
    /* +0x801 */ u8 field_0x801;        /* cleared when the work is set up (`em_act_advance`) */
    /* +0x802 */ u8 unused_0x802[0x810 - 0x802];
    /* +0x810 */ s16 mark_timer_0x810;  /* the frames the enemy stays marked (`em_net_flag_apply_0`) */
    /* +0x812 */ u16 field_0x812;       /* the action-end block clears it */
    /* +0x814 */ u16 field_0x814;       /* the action-end block clears it */
    /* +0x816 */ u16 field_0x816;
    /* +0x818 */ s16 field_0x818;       /* the motion timer `fn_8012F110` arms before its mode switch */
    /* +0x81A */ s16 field_0x81A;     /* the motion timer `em015_area_follow` gates its arming on
                                        * (<= 0) */
    /* +0x81C */ s16 timer_0x81C;  /* the two-frame timer `em_timer_0x81C_arm` arms */
    /* +0x81E */ s16 timer_0x81E;  /* the two-frame timer `em_timer_0x81E_arm` arms */
    /* +0x820 */ u8 status_bits_0x820;  /* the bits `em_status_bit_set` raises for two frames */
    /* +0x821 */ s8 status_bits_timer_0x821;  /* their timer */
    /* +0x822 */ u8 unused_0x822[0x833 - 0x822];
    /* +0x833 */ u8 field_0x833;       /* the "approach armed" byte TWO readers branch on:
                                        * `enemy/em030_prog.cpp`'s em030 `fn_801B0A28` and
                                        * `enemy/em034_prog.cpp`'s `fn_801B47A4` */
    /* +0x834 */ u8 field_0x834;      /* the "already seated" byte `fn_801671AC` tests against 1 */
    /* +0x835 */ u8 field_0x835;      /* the seat flag `fn_80166DF8` sets */
    /* +0x836 */ u16 flags_0x836;       /* bit 0x8000 is the "aim target found" flag `fn_801D75D0`
                                         * mirrors `self->action_0x328.field_0x345` into */
    /* +0x838 */ EmPartRec parts_0x838[8];   /* the eight part records the net sync packs (0x838..0x868) */
    union {
        /* +0x868 */ u32 values_0x868[14];  /* the 14-word view `em_common`'s indexed reader takes (0x868..0x8A0) */
        /* +0x868 */ u8* part_tbls_0x868[8];  /* per part, the 9-byte part table `em_part_rec_reset` points at */
        struct {
            /* +0x868 */ u8 unused_0x868[0x888 - 0x868];
            /* +0x888 */ s32 field_0x888;       /* the effect handle `enemy/em034_prog.cpp`'s `fn_801B4E3C`
                                                * releases through `fn_803B9994` and clears to -1; `fn_801B6C38`
                                                * tests it against -1 */
            /* +0x88C */ s32 drop_rec_0x88C[3];   /* the three drop records `em_drop_item_spawn` places (-1 free) */
            /* +0x898 */ s16 drop_timer_0x898[3]; /* each drop's life, copied from the record's +0x1B8 */
            /* +0x89E */ s8 drop_left_0x89E;      /* the drops the enemy may still spawn */
            /* +0x89F */ u8 field_0x89F;        /* the mode `em_motion_mode_set` latches */
        };
    };
    /* +0x8A0 */ u16 stamina_max_0x8A0;  /* the stamina maximum `em_stamina_add` caps +0x8A2 at */
    /* +0x8A2 */ u16 field_0x8A2;       /* the counter `fn_801CD400` gates its target search on
                                        * (`lhz` + `cmplwi 0xFA`) */
    /* +0x8A4 */ s16 field_0x8A4;       /* the timer `em_motion_mode_set` arms from `fn_80126494` */
    /* +0x8A6 */ s16 field_0x8A6;       /* the timer `fn_801409C8` compares against the enemy data's */
    /* +0x8A8 */ s16 field_0x8A8;  /* the hold timer `em_anger_add` tests (<= 0) before raising the anger state */
    /* +0x8AA */ u8 mode_0x8AA;         /* `em_alt_mode_set` latches it (0/1); `em_alt_mode_ck` tests it
                                        * against 1 */
    /* +0x8AB */ u8 faint_0x8AB;  /* set while the enemy is knocked out (`em_tint_hold_start`) */
    /* +0x8AC */ u8 flag_0x8AC;  /* set by `em_rest_flag_set` */
    /* +0x8AD */ u8 flag_0x8AD;  /* set by `em_flag_0x8AD_set` */
    /* +0x8AE */ u8 flag_0x8AE;  /* set by `em_guard_set` */
    /* +0x8AF */ u8 flag_0x8AF;  /* set by `em_flag_0x8AF_set` */
    /* +0x8B0 */ u8 flag_0x8B0;  /* set by `em_idle_flag2_set` once the enemy is idle */
    /* +0x8B1 */ u8 unused_0x8B1[0x8B3 - 0x8B1];
    /* +0x8B3 */ u8 flags_0x8B3;        /* the action bitmap `fn_801376BC`/`DC`/`04` mask */
    /* +0x8B4 */ u8 unused_0x8B4[0x8C2 - 0x8B4];
    /* +0x8C2 */ s16 fx_timer_0x8C2;  /* the effect timer `em_fx_flag_set` arms */
    /* +0x8C4 */ s16 fx_timer_0x8C4;  /* the effect timer `em_fx_flag2_set` arms */
    /* +0x8C6 */ s16 fx_timer_0x8C6;  /* the effect timer `em_fx_flag3_set` arms */
    /* +0x8C8 */ u32 field_0x8C8;       /* `fn_801260BC`'s value `em_static_data_latch` latches */
    /* +0x8CC */ u32 field_0x8CC;       /* `fn_801260E0`'s value `em_static_data_latch` latches */
    /* +0x8D0 */ u8 unused_0x8D0[0x8D2 - 0x8D0];
    /* +0x8D2 */ u8 field_0x8D2;        /* nonzero, `em_area_entry_store` keeps the record's own timer */
    /* +0x8D3 */ u8 field_0x8D3;        /* `fn_80137614` matches it against 1 */
    /* +0x8D4 */ u8 unused_0x8D4[0x914 - 0x8D4];
    /* +0x914 */ s16 field_0x914;       /* the frames `em_area_entry_store` subtracts from the kind's wait (capped at 300) */
    /* +0x916 */ s16 field_0x916;
    /* +0x918 */ u8 level_kind_0x918;  /* the kind `em_level_raise` was last raised with */
    /* +0x919 */ u8 field_0x919;
    /* +0x91A */ u8 recover_count_0x91A[2];  /* the two recovery kinds' trigger counts (`em_recover_timer_set`) */
    /* +0x91C */ u8 field_0x91C;
    /* +0x91D */ u8 paralysis_count_0x91D;  /* the paralysis triggers so far (`em_ailment_limit_raise`) */
    /* +0x91E */ u8 sleep_count_0x91E;  /* the sleep triggers so far */
    /* +0x91F */ u8 stun_count_0x91F;  /* the stun triggers so far */
    /* +0x920 */ u8 exhaust_count_0x920;  /* the exhaust triggers so far */
    /* +0x921 */ u8 unused_0x921[0x922 - 0x921];
    /* +0x922 */ s16 poison_limit_0x922;  /* the poison buildup that triggers the poison */
    /* +0x924 */ s16 field_0x924;
    /* +0x926 */ s16 field_0x926;
    /* +0x928 */ s16 poison_damage_0x928;  /* the poison damage of this frame */
    /* +0x92A */ s16 poison_decay_0x92A;  /* the frames to the next poison decay */
    /* +0x92C */ s16 paralysis_limit_0x92C;  /* the paralysis buildup that triggers the paralysis */
    /* +0x92E */ s16 field_0x92E;
    /* +0x930 */ s16 paralysis_decay_0x930;  /* the frames to the next paralysis decay */
    /* +0x932 */ s16 sleep_limit_0x932;  /* the sleep buildup that puts the enemy to sleep */
    /* +0x934 */ s16 field_0x934;
    /* +0x936 */ s16 sleep_decay_0x936;  /* the frames to the next sleep decay */
    /* +0x938 */ s16 field_0x938;       /* the motion timer `em_motion_window_ck` gates its window test on */
    /* +0x93A */ s16 field_0x93A;
    /* +0x93C */ s16 ailment8_decay_0x93C;  /* the frames to the next status-8 decay (300) */
    /* +0x93E */ s16 field_0x93E;
    /* +0x940 */ s16 stun_limit_0x940;  /* the stun buildup that stuns the enemy */
    /* +0x942 */ s16 stun_decay_0x942;  /* the frames to the next stun decay */
    /* +0x944 */ s16 field_0x944;
    /* +0x946 */ s16 exhaust_limit_0x946;  /* the exhaust buildup that exhausts the enemy */
    /* +0x948 */ s16 exhaust_decay_0x948;  /* the frames to the next exhaust decay */
    /* +0x94A */ u16 field_0x94A;       /* the action-end block clears it */
    /* +0x94C */ u16 field_0x94C;       /* the action-end block clears it */
    /* +0x94E */ u8 ailment_0x94E;  /* the running ailments: 1 sleep, 2 poison, 4 paralysis (`em_status_ck`) */
    /* +0x94F */ u8 field_0x94F;
    /* +0x950 */ s16 sleep_timer_0x950;  /* the frames the sleep lasts (`em_sleep_start`) */
    /* +0x952 */ s16 recover_timer_0x952;  /* the recovery timer `em_recover_timer_set` arms */
    /* +0x954 */ s32** field_0x954;
    /* +0x958 */ s32 field_0x958;
    /* +0x95C */ u8 field_0x95C;
    /* +0x95D */ u8 field_0x95D;
    /* +0x95E */ u8 unused_0x95E[1];
    /* +0x95F */ u8 state_0x95F;
    /* +0x960 */ u8 state_0x960;
    union {
        struct {
        /* +0x961 */ u8 stack_0x961[0x0F];  /* the interpreter's byte stack: `fn_8013FEF4` writes
                                            * `stack_0x961[arg[0]]`, index 1 is the depth `fn_801408B4`
                                            * pops through (`stack_0x961[2 + depth]` is the byte it
                                            * restores, `values_0x970[depth]` the word) */
        };
        /* `enemy/em_kind.cpp`'s view of the byte stack: depth at +0x962, bytes from +0x963 */
        struct {
            /* +0x961 */ u8 unused_0x961;
            /* +0x962 */ u8 state_0x962;
            /* +0x963 */ u8 bytes_0x963[0xA];
            /* +0x96D */ u8 unused_0x96D[3];
        };
    };
    /* +0x970 */ s32 values_0x970[10];  /* the words belonging to `stack_0x961` */
    /* +0x998 */ u8 state_0x998;
    /* +0x999 */ u8 state_0x999;
    /* +0x99A */ u8 field_0x99A;        /* the high byte of the special part id a kind-8 record carries */
    /* +0x99B */ u8 field_0x99B;        /* the net-synced flag `em_net_send_state` sends as flag bit 0x400 */
    union {
        struct {
        /* +0x99C */ EmCmdRec rec_0x99C;
        };
        /* `enemy/em_kind.cpp`'s view of the saved command record */
        struct {
            /* +0x99C */ u8 state_0x99C;
            /* +0x99D */ u8 state_0x99D;
            /* +0x99E */ u8 unused_0x99E[2];
            /* +0x9A0 */ u32 ptr_0x9A0;
        };
    };
    /* +0x9A4 */ EmCmdRec* recs_0x9A4; /* the record array `fn_8013FB08` reports through */
    /* +0x9A8 */ u8 count_0x9A8;       /* the count `fn_801408B4` case 1 pops */
    /* +0x9A9 */ u8 unused_0x9A9[0x9AC - 0x9A9];
    /* +0x9AC */ EmCmdRec recs_0x9AC[6]; /* the 8-byte entries `fn_801408B4` case 1 pops */
    /* +0x9DC */ u8 field_0x9DC;
    /* +0x9DD */ u8 field_0x9DD;
    /* +0x9DE */ u8 unused_0x9DE[2];
    /* +0x9E0 */ s32 field_0x9E0;
    /* +0x9E4 */ u8 field_0x9E4;
    /* +0x9E5 */ u8 unused_0x9E5[0x9E8 - 0x9E5];
    /* +0x9E8 */ u8 field_0x9E8;        /* the record `fn_801408B4` case 0xA restores */
    /* +0x9E9 */ u8 field_0x9E9;
    /* +0x9EA */ u8 unused_0x9EA[0x9EC - 0x9EA];
    /* +0x9EC */ s32 field_0x9EC;
    /* +0x9F0 */ u32 ptr_0x9F0;
    /* +0x9F4 */ u8 state_0x9F4;
    /* +0x9F5 */ u8 state_0x9F5;
    /* +0x9F6 */ u8 field_0x9F6;       /* one state byte, two readers: `enemy/em018_prog.cpp`'s
                                        * `fn_8019DAC0` (area 3 arms the done state only when it
                                        * reads 2) and `enemy/em005_act.cpp`'s `fn_801D6D24`
                                        * (matches it against 7 before arming the +0x330 flag). */
    /* +0x9F7 */ u8 field_0x9F7;        /* matched against `area_no` by `fn_8013FF5C` */
    /* +0x9F8 */ u8 field_0x9F8;        /* the value `fn_8013FD28`'s command reports */
    /* +0x9F9 */ u8 state_0x9F9;
    /* +0x9FA */ u8 state_0x9FA;
    /* +0x9FB */ u8 state_0x9FB;
    /* +0x9FC */ u8 field_0x9FC;        /* the program index `fn_8013F764`/`fn_8013F994` step */
    /* +0x9FD */ u8 field_0x9FD;        /* bit 0 walks the program table backwards */
    /* +0x9FE */ u8 field_0x9FE;        /* the run's state: 0 = not started, 1/2 = the distance test */
    /* +0x9FF */ u8 field_0x9FF;        /* set by every `fn_8013F764` call */
    /* +0xA00 */ EmProgTbl* prog_0xA00; /* the enemy's program table (NULL → nothing to do) */
    /* +0x0A04 */ u8 flags_0xA04;       /* bit 0 stun, bit 1 sleep, bit 2 the sleep result */
    /* +0x0A05 */ u8 field_0xA05;
    /* +0x0A06 */ u8 field_0xA06;
    /* +0x0A07 */ u8 field_0xA07;
    union {
        struct {
            /* +0x0A08 */ u8 hit_0xA08[0xA0D - 0xA08];  /* the hit reference `enemy/em018_prog.cpp`'s `fn_80198150`
                                                        * hands `hit_list_capsule_ck` with its capsule */
            /* +0x0A0D */ u8 field_0xA0D;       /* em015_act2_hit_chain's per-frame gate */
            /* +0x0A0E */ u8 unused_0xA0E[0xA14 - 0xA0E];
            /* +0x0A14 */ u16 shell_flags_0xA14; /* the flag word `enemy/em015_prog.cpp` hands the `ShellSetFuncs` +0x2C job */
            /* +0xA16 */ u8 field_0xA16;      /* the byte `enemy/em030_prog.cpp`'s `fn_801B203C` tests against
                                                * 0xFF/5 before it latches the counter-attack */
            /* +0xA17 */ u8 unused_0xA17[0xA34 - 0xA17];
            /* +0xA34 */ _PLW* plw_0xA34;     /* the player work `fn_801B203C` hands `fn_801B0230` */
            /* +0xA38 */ u8 field_0xA38;      /* the third gate `fn_801B203C` tests */
            /* +0xA39 */ u8 unused_0xA39[0xA4C - 0xA39];
            /* +0xA4C */ u8 hit_angle_0xA4C;  /* the first hit record's direction byte (the high byte of a
                                               * 16-bit angle) `enemy/em003_prog.cpp`'s act-7 swipe steers */
            /* +0xA4D */ u8 unused_0xA4D[0xA64 - 0xA4D];
            /* +0xA64 */ union {
                /* +0xA64 */ u8 hit_0xA64[0x60];  /* the record's own `_HIT_W` (`Pl/hit_w.h`, size 0x60): `enemy/em029_prog.cpp`'s
                                                   * `em_action3_sub0` hands it to `hit_mask_ck`.  A byte view: `_HIT_W` has two
                                                   * definitions (`Pl/hit_w.h`, `ai/ai_npc.h`) and this header includes neither */
                struct {
                    /* +0xA64 */ u8 pad_0xA64[0xAC0 - 0xA64];
                    /* +0xAC0 */ u16 hit_ids_0xAC0[2];  /* the head of the hit id list (`hit_id_list_clear`), running
                                                      * on to its count at +0xAE8 */
                };
                struct {
                    /* +0xA64 */ u8 unused_0xA64[0xA69 - 0xA64];
                    /* +0xA69 */ u8 field_0xA69;       /* em015_act2_hit_chain's second gate (the `_HIT_W`'s +0x05 byte) */
                    /* +0xA6A */ u8 unused_0xA6A[20];
                    /* +0xA7E */ u16 field_0xa7e;      /* the `_HIT_W`'s +0x1A halfword */
                };
            };
        };
        /* +0xA08 */ EmHitSlot hit_slots_0xA08[2];  /* the two attack hit records (`_HIT_W`, 0x5C apart) `em_hit_window_set` arms */
    };
    /* +0xAC4 */ u8 unused_0xAC4[0xAE8 - 0xAC4];
    /* +0xAE8 */ u8 hit_id_count_0xAE8;  /* the length of the hit id list at +0xAC0 (`hit_id_list_clear`) */
    /* +0x0AE9 */ u8 field_0xAE9;       /* the one-shot flag `enemy/em009_act.cpp`'s
                                        * `fn_80389C50` tests (== 1) to choose its motion set */
    /* +0x0AEA */ u16 field_0xAEA;      /* the eighteen-bit flag word `enemy/em007_act.cpp`'s
                                        * effect calls hand the shell callback */
    /* +0xAEC */ u8 hit_flags_0xAEC[2];  /* per hit record, bit 0: the window was set with flag 2 (`em_hit_window_set`) */
    /* +0xAEE */ u8 field_0xAEE;        /* `em_act_update`'s one-shot action-setup request */
    /* +0xAEF */ u8 unused_0xAEF[0xAF0 - 0xAEF];
    /* +0xAF0 */ EmDelayRec delays_0xAF0[1];  /* the delayed action `em_delay_act_set` queues */
    union {
        /* +0xB04 */ u8 data_0xB04[0x10];  /* the 16 bytes the area entry keeps across a store (`em_area_entry_store`) */
        struct {
            /* +0xB04 */ u8 resume_mode_0xB04;  /* the hold `enemy/em027_prog.cpp`'s frame update latches and its init re-arms */
            /* +0xB05 */ u8 seated_keep_0xB05;  /* the seated flag that unit's init copies back into its +0x328 view */
        }; /* size: 0x02 */
        /* +0xB04 */ struct EmRunPos run_0xB04; /* the position em035's init saves and restores */
    };
    /* +0xB14 */ struct _se_w* se_0xB14;  /* the sound-work handle `enemy/em006_prog.cpp`'s
                                           * `se_req_pos_ps`/`shell_se_req` calls take */
};

#endif /* MHTRI_ENEMY_ENEMY_WORK_H */
