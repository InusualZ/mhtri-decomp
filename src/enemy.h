#ifndef ENEMY_H
#define ENEMY_H

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The enemy-module shared records.  `_ENEMY_WORK` is the per-enemy work record the enemy and effect
 * units both read (per-distance cost tables, animation state, the effect hand-off fields); `EnemyData`
 * its static data record.  Both were copied into more than one unit with only the fields the owner read
 * named; the definitions are the union of every copy.
 */

/* One 8-byte entry of the per-enemy effect queue (`ef/fn_8013BE60.c` carries the whole record). The
 * queue itself is `_ENEMY_WORK::queue_0x9AC`. size: 0x8 */
typedef struct _ENEMY_QUEUE_ENTRY {
    u8 a;      /* +0x0 */
    u8 b;      /* +0x1 */
    u8 pad_0x02[0x2];
    u32 value; /* +0x4 */
} _ENEMY_QUEUE_ENTRY;

struct _se_w; /* the sound-request handle; opaque here (defined in sound/fn_800D7F54.cpp) */
/* The action-script records `_ENEMY_WORK::seq_0x8D4` and its two cursors point at; `enemy/fn_8011D448.cpp` defines
 * them. */
struct EmSeqCmd;
struct EmSeqMotion;
struct EmSeqCond;

/* One carried roster record of `_ENEMY_WORK::carry_0x488`: only the roster index the carcass code tests is named.
 * size: 0x84 */
struct EmCarrySlot {
    /* +0x00 */ u8 unused_0x00[0x80];
    /* +0x80 */ s32 roster_0x80;       /* -1 = none */
};
/* One 6-byte record of `_ENEMY_WORK::parts_0x838`: `em_parts_damage_level_get` reads +0x00 and
 * `em_parts_damage_add` accumulates into it, `em_parts_hp_reset` writes the +0x02 value. size: 0x06 */
typedef struct _ENEMY_PART {
    /* +0x00 */ u8 damage_level;
    /* +0x01 */ u8 state_0x01;     /* `em_parts_state_add`/`em_parts_state_get` */
    /* +0x02 */ s16 hp_0x02;       /* the part's damage left before it flinches (`em_parts_hp_reset`) */
    /* +0x04 */ s16 limit_0x04;    /* the part's flinch-limit gauge (`em_parts_limit_reset`) */
} _ENEMY_PART;


/* The per-enemy work record.  Union of `ef/eft007.cpp`, `ef/eft009.cpp`, `ef/fn_80104BD0.c`,
 * `ef/fn_80114E34.cpp`, `enemy/fn_8012BA00.c`, `enemy/fn_8012BDF4.cpp`, `enemy/fn_8013BE60.c`,
 * `enemy/fn_80149D6C.c` and `enemy/fn_8014A1BC.c`.
 *
 * Disagreements found: the name `state` is used at +0x1E5 by `enemy/fn_8012BDF4.cpp` and at +0x005 by
 * `enemy/fn_8014A1BC.c` (two different bytes -- kept as `action_0x1E5` and `state_0x05` here, and the
 * clash is reported); `pos_0x1BC` is a `_CP_VECTOR` in `ef/eft007.cpp`/`ef/eft009.cpp` and a `VEC3` in
 * `ef/fn_80104BD0.c` (same size, different interpretation); `_se_w` is reached at +0xB14 by
 * `ef/eft009.cpp` and +0xB14 by `ef/fn_80114E34.cpp` (agree).
 * size: 0xB1C */
typedef struct _ENEMY_WORK _ENEMY_WORK;
struct _ENEMY_WORK {
    /* +0x000 */ u8 active;
    /* +0x001 */ union {
        u8 pad_0x1[0x1];              /* `ef/eft013_fx.cpp`'s spelling */
        u8 visible_0x001;             /* cleared once the carcass has faded (`em_seq_step_ck`) */
    };
    /* +0x002 */ u8 group;
    /* +0x003 */ u8 team;
    /* +0x004 */ u8 field_0x004;
    /* +0x005 */ u8 state_0x05;
    /* +0x006 */ u8 phase_0x06;
    /* +0x007 */ u8 step_0x07;
    /* +0x008 */ u8 field_0x008;
    /* +0x009 */ u8 state_0x009;
    /* +0x00A */ u8 field_0x00A;
    /* +0x00B */ u8 pad_0xB[0x1];
    /* +0x00C */ u8 state_0x00C;
    /* +0x00D */ u8 field_0x00D;      /* nonzero, the arena flag keeps the carcass (`em_seq_death_hold_ck`) */
    /* +0x00E */ u8 pad_0xE[0x3];
    /* +0x011 */ u8 field_0x011;
    /* +0x012 */ u8 pad_0x12[0x2];
    /* +0x014 */ s8 broken_groups_0x014; /* the part groups broken so far (`em_parts_break`) */
    /* +0x015 */ u8 pad_0x15[0x2];
    /* +0x017 */ u8 field_0x017;      /* `em_seq_run` sets 3 when the carcass has no spawn record left */
    /* +0x018 */ s16 timer_0x18;
    /* +0x01A */ union {
        u8 pad_0x1A[0x6];
        struct {
            /* +0x01A */ u16 field_0x01A;   /* passed to `em_spawn_request` (`enemy/fn_8035E034.cpp`) */
            /* +0x01C */ u8 unused_0x1C[0x4];
        };
    };
    /* +0x020 */ union {
        u32 field_0x20;
        s32 seq_timer_0x20;           /* the action script's countdown (`em_seq_step_ck`) */
    };
    /* +0x024 */ u8 pad_0x24[0x3C];
    /* +0x060 */ u8* field_0x60;
    /* +0x064 */ u8 pad_0x64[0x10C];
    /* +0x170 */ u8 state_0x170;
    /* +0x171 */ u8 pad_0x171[0x7];
    /* +0x178 */ VEC3 pos_0x178;
    /* +0x184 */ u8 pad_0x184[0x4];
    /* +0x188 */ VEC3 pos;
    /* +0x194 */ VEC3 prev_pos;
    /* +0x1A0 */ u8 pad_0x1A0[0xC];
    /* +0x1AC */ f32 field_0x1ac;
    /* +0x1B0 */ VEC3 aim;
    /* +0x1BC */ _CP_VECTOR pos_0x1BC;
    /* +0x1C8 */ u32 field_0x1C8;
    /* +0x1CC */ union {
        u8 pad_0x1CC[0x14];
        struct {
            /* +0x1CC */ f32 field_0x1CC;   /* `enemy/fn_8035E034.cpp` seeds the run speed */
            /* +0x1D0 */ u8 unused_0x1D0[0x4];
            /* +0x1D4 */ f32 alpha_0x1D4;   /* the action script's "faded out" test (== 0.0f) */
            /* +0x1D8 */ u8 unused_0x1D8[0x8];
        };
    };
    /* +0x1E0 */ u8 field_0x1E0;
    /* +0x1E1 */ u8 act_id;
    /* +0x1E2 */ u8 state_0x1E2;
    /* +0x1E3 */ u8 field_0x1E3;
    /* +0x1E4 */ u8 field_0x1E4;
    /* +0x1E5 */ u8 action_0x1E5;
    /* +0x1E6 */ u8 state_sub;
    /* +0x1E7 */ u8 state_0x1E7;
    /* +0x1E8 */ u8 field_0x1E8;
    /* +0x1E9 */ u8 field_0x1E9;
    /* +0x1EA */ u8 pad_0x1EA[0x2];
    /* +0x1EC */ u16 bits_0x1EC;
    /* +0x1EE */ u8 field_0x1EE;
    /* +0x1EF */ u8 field_0x1EF;
    /* +0x1F0 */ u16 field_0x1F0;
    /* +0x1F2 */ u16 counter_0x1F2;
    /* +0x1F4 */ u8 field_0x1F4;
    /* +0x1F5 */ u8 field_0x1F5;
    /* +0x1F6 */ s8 field_0x1F6;
    /* +0x1F7 */ u8 pad_0x1F7[0x1];
    /* +0x1F8 */ u8 field_0x1F8;
    /* +0x1F9 */ u8 state_0x1F9;
    /* +0x1FA */ u8 state_0x1FA;
    /* +0x1FB */ u8 pad_0x1FB[0x1];
    /* +0x1FC */ u8 state_0x1FC;
    /* +0x1FD */ u8 pad_0x1FD[0x1];
    /* +0x1FE */ u8 state_0x1FE;
    /* +0x1FF */ u8 state_0x1FF;
    /* +0x200 */ s16 field_0x200;
    /* +0x202 */ s16 field_0x202;
    /* +0x204 */ u32 joint_0x204;   /* the joint `em_seq_run` drops from */
    /* +0x208 */ u8 pad_0x208[0x4];
    /* +0x20C */ f32 field_0x20C;
    /* +0x210 */ f32 field_0x210;
    /* +0x214 */ f32 field_0x214;
    /* +0x218 */ u8 pad_0x218[0x10];
    /* +0x228 */ u16 joint_flags;
    /* +0x22A */ u8 pad_0x22A[0xE6];
    /* +0x310 */ VEC3 v_0x310;
    /* +0x31C */ union {
        struct {
            /* +0x31C */ u8 pad_0x31C[0x4];
            /* +0x320 */ union {
                VEC3 v_0x320;
                struct {
                    /* +0x320 */ u8 unused_0x320[0x8];
                    /* +0x328 */ union {
                        u16 field_0x328;   /* `enemy/fn_8035E034.cpp`'s em035 init (0x708) */
                        u8 bytes_0x328[0x2];   /* the effect-slot bytes `fn_80154E40` resets */
                    };
                    /* +0x32A */ union {
                        u16 field_0x32A;   /* ... and 0xB4 */
                        u8 bytes_0x32A[0x2];
                    };
                } init_0x320;
        };
        };
        /* +0x31C */ VEC3 v_0x31C;       /* the move acceleration the action script loads (`em_seq_move_setup`) */
    };
    /* +0x32C */ union {
        u16 field_0x32c;
        struct {
            /* +0x32C */ u8 field_0x32C;
            /* +0x32D */ u8 field_0x32D;
        } bytes_0x32C;
    };
    /* +0x32E */ u8 field_0x32E;
    /* +0x32F */ u8 field_0x32F;
    /* +0x330 */ u8 field_0x330;
    /* +0x331 */ u8 field_0x331;
    /* +0x332 */ union {
        /* the signed-short reading `enemy/fn_80176C58.cpp` uses (the death timer) */
        s16 timer_0x332;
        /* `fn_8015D194` of `enemy/fn_801550FC.cpp` clears the byte at +0x333 on its own */
        struct {
            /* +0x332 */ u8 field_0x332;
            /* +0x333 */ u8 field_0x333;
        } bytes_0x332;
    };
    /* +0x334 */ u8 field_0x334;
    /* +0x335 */ u8 field_0x335;
    /* +0x336 */ u8 field_0x336;
    /* +0x337 */ u8 pad_0x337[0x1];
    /* +0x338 */ s16 timer_0x338;   /* `fn_80155664` counts this down while it is positive */
    /* +0x33A */ union {
        u8 pad_0x33A[0x2];
        struct {
            /* +0x33A */ u8 unused_0x33A;
            /* +0x33B */ u8 field_0x33B;   /* the monster record id `enemy/fn_8035E034.cpp` caches */
        };
    };
    /* +0x33C */ u16 counter_0x33C; /* `fn_80155664` cycles it 0..0x31 */
    /* +0x33E */ union {
        u8 pad_0x33E[0x2E];
        struct {
            /* +0x33E */ u8 field_0x33E;   /* `enemy/fn_8035E034.cpp` returns this */
            /* +0x33F */ u8 field_0x33F;   /* the em033 approach phase */
            /* +0x340 */ u8 unused_0x340[0x2C];
        };
    };
    /* +0x36C */ VEC3 target;
    /* +0x378 */ f32 field_0x378;
    /* +0x37C */ u32 field_0x37c;
    /* +0x380 */ u8 special_type_0x380;
    /* +0x381 */ u8 state_0x381;
    /* +0x382 */ u8 special_part_0x382;
    /* +0x383 */ u8 state_0x383;
    /* +0x384 */ f32 value_0x384;
    /* +0x388 */ u8 pad_0x388[0x2];
    /* +0x38A */ u8 field_0x38A;
    /* +0x38B */ u8 pad_0x38B[0x1];
    /* +0x38C */ u8 field_0x38C;
    /* +0x38D */ u8 pad_0x38D[0x2];
    /* +0x38F */ u8 field_0x38F;
    /* +0x390 */ s32 values_0x390[5];
    /* +0x3A4 */ s32 values_0x3A4[31];
    /* +0x420 */ u8 flag_0x420;
    /* +0x421 */ u8 pad_0x421[0x1];
    /* +0x422 */ s16 count_0x422;
    /* +0x424 */ u32 timer_0x424;
    /* +0x428 */ s32 values_0x428[4];
    /* +0x438 */ u8 pad_0x438[0x1];
    /* +0x439 */ u8 field_0x439;
    /* +0x43A */ u8 field_0x43A;
    /* +0x43B */ u8 state_0x43B;
    /* +0x43C */ u8 state_0x43C;
    /* +0x43D */ u8 timed_0x43D;
    /* +0x43E */ u8 field_0x43E;
    /* +0x43F */ u8 pad_0x43F[0xD];
    /* +0x44C */ s16 value_0x44C;
    /* +0x44E */ s16 value_0x44E;
    /* +0x450 */ u8 pad_0x450[0x2];
    /* +0x452 */ s16 value_0x452;
    /* +0x454 */ f32 frames_0x454[8];
    /* +0x474 */ u8 pad_0x474[0xE];
    /* +0x482 */ u8 field_0x482;   /* the byte both action bands read: `enemy/fn_80178378.cpp`'s
                                    * `em015_act5_swim_turn` picks its fade constant with it, and
                                    * `enemy/fn_8015E854.cpp`'s `fn_8015EA24` gates the run-in float on it
                                    */
    /* +0x483 */ u8 pad_0x483[0x5];
    /* +0x488 */ struct EmCarrySlot carry_0x488[2]; /* the two carried roster records (`em_seq_step_ck`) */
    /* +0x590 */ u8 pad_0x590[0x34];
    /* +0x5C4 */ u8 flags_0x5C4;
    /* +0x5C5 */ u8 pad_0x5C5[0x1BF];
    /* +0x784 */ u8 field_0x784;
    /* +0x785 */ u8 pad_0x785[0xF];
    /* +0x794 */ u16 field_0x794;
    /* +0x796 */ s16 field_0x796;
    /* +0x798 */ u8 field_0x798;
    /* +0x799 */ u8 field_0x799;
    /* +0x79A */ u8 field_0x79A;
    /* +0x79B */ u8 field_0x79B;
    /* +0x79C */ u8 pad_0x79C[0x4];
    /* +0x7A0 */ s32 amount_0x7A0;   /* the clamped gauge fn_8011E658/6EC/760 keep in [0, max] */
    /* +0x7A4 */ u32 field_0x7a4;
    /* +0x7A8 */ u8 pad_0x7A8[0x8];
    /* +0x7B0 */ f32 field_0x7B0;
    /* +0x7B4 */ u8 pad_0x7B4[0x4];
    /* +0x7B8 */ f32 damage_rate_0x7B8; /* the damage multiplier `em_damage_bonus_apply` starts from */
    /* +0x7BC */ u8 pad_0x7BC[0xC];
    /* +0x7C8 */ u8 field_0x7c8;
    /* +0x7C9 */ u8 pad_0x7C9[0x1];
    /* +0x7CA */ s16 part_damage_0x7CA[8];     /* this frame's damage per part (`em_damage_hp_add`) */
    /* +0x7DA */ s16 kind_damage_0x7DA[2][8];  /* this frame's damage per part, per hit kind */
    /* +0x7FA */ u8 pad_0x7FA[0x2];
    /* +0x7FC */ u32 bonus_mask_0x7FC;  /* the sleep-bonus mask `em_damage_bonus_apply` latches */
    /* +0x800 */ u8 part_hit_0x800;     /* the parts hit this frame (`em_damage_part_hit_set`) */
    /* +0x801 */ u8 part_hit_0x801;     /* the copy the part-limit reaction keeps */
    /* +0x802 */ s16 ailment_0x802[4];  /* this frame's ailment build-up, kinds 9..12 (`em_damage_attr_add`) */
    /* +0x80A */ s16 stun_0x80A;        /* this frame's stun build-up (`em_damage_stun_add`) */
    /* +0x80C */ s16 exhaust_0x80C;     /* this frame's exhaust build-up */
    /* +0x80E */ u16 mask_0x80E;     /* the bit mask em_damage_flags_set ORs into */
    /* +0x810 */ u8 pad_0x810[0x6];
    /* +0x816 */ u16 hit_angle_0x816;   /* the facing `em_damage_record_clear` latches, `em_hit_dir_get` measures */
    /* +0x818 */ u8 pad_0x818[0x2];
    /* +0x81A */ s16 timer_0x81A;       /* positive, the damage reactions are held off */
    /* +0x81C */ s16 timer_0x81C;
    /* +0x81E */ s16 timer_0x81E;       /* picks the second wake-up motion pair (`em_event_settle_kind24`) */
    /* +0x820 */ u8 part_mask_0x820;
    /* +0x821 */ s8 part_count_0x821;
    /* +0x822 */ u8 pad_0x822[0x2];
    /* +0x824 */ u32 bits_0x824;     /* the per-enemy status bits em_status_bits_init..em_status_bits_ck own */
    /* +0x828 */ u16 field_0x828;       /* `em_damage_record_clear` clears it */
    /* +0x82A */ u16 hit_kind_0x82A;    /* the kind of the last hit (`em_damage_eval`) */
    /* +0x82C */ u16 react_kind_0x82C;  /* the reaction `em_event_settle_dispatch` starts */
    /* +0x82E */ u16 part_broken_0x82E; /* the parts whose damage ran out this frame */
    /* +0x830 */ u8 pad_0x830[0x1];
    /* +0x831 */ u8 react_part_0x831;   /* the part that took the most of it */
    /* +0x832 */ u8 limit_part_0x832;   /* the part whose limit ran out (0xFF = none) */
    /* +0x833 */ u8 hit_0x833;          /* a hit landed this frame */
    /* +0x834 */ u8 field_0x834;
    /* +0x835 */ u8 field_0x835;
    /* +0x836 */ u16 flags_0x836;    /* bit 0x8000 is set once the effect helper is attached (`fn_80154E90`) */
    /* +0x838 */ _ENEMY_PART parts_0x838[8]; /* the per-part damage table (stride 6, 8 parts) */
    /* +0x868 */ union {
        u32 values_0x868[14];
        struct {
            /* +0x868 */ u8 unused_0x868[0x20];
            /* +0x888 */ s32 roster_slot_0x888;  /* the roster record the carcass holds (-1 = none) */
            /* +0x88C */ u8 unused_0x88C[0x12];
            /* +0x89E */ u8 carve_count_0x89E;
            /* +0x89F */ u8 unused_0x89F[0x1];
        };
    };
    /* +0x8A0 */ u8 pad_0x8A0[0x2];
    /* +0x8A2 */ u16 counter_0x8A2;
    /* +0x8A4 */ u8 pad_0x8A4[0x2C];
    /* +0x8D0 */ u8 seq_index_0x8D0;    /* the action script's next record (`em_seq_exec`) */
    /* +0x8D1 */ u8 seq_flags_0x8D1;    /* the per-frame calls the script armed */
    /* +0x8D2 */ u8 seq_stage_0x8D2;    /* the death sequence's stage (`em_seq_run`) */
    /* +0x8D3 */ u8 field_0x8D3;
    /* +0x8D4 */ struct EmSeqCmd* seq_0x8D4; /* the action script */
    /* +0x8D8 */ u32 field_0x8D8;
    /* +0x8DC */ u32 field_0x8DC;
    /* +0x8E0 */ struct EmSeqMotion* seq_mot_0x8E0; /* the script's current motion record */
    /* +0x8E4 */ struct EmSeqCond* seq_cond_0x8E4;  /* the script's pending jump condition */
    /* +0x8E8 */ f32 seq_frame_0x8E8;   /* the motion frame the script waits for */
    /* +0x8EC */ f32 seq_turn_rate_0x8EC;
    /* +0x8F0 */ f32 seq_turn_0x8F0;
    /* +0x8F4 */ s32 seq_turn_angle_0x8F4;
    /* +0x8F8 */ u32 seq_rot_0x8F8;     /* the facing the script's move vectors are turned by */
    /* +0x8FC */ f32 seq_ground_0x8FC;  /* the ground height the landing test uses */
    /* +0x900 */ f32 seq_speed_0x900;   /* the motion speed the script sets (> 0.0f) */
    /* +0x904 */ f32 seq_value_0x904;   /* the value the script's waits compare */
    /* +0x908 */ f32 seq_move_frame_0x908;
    /* +0x90C */ f32 seq_move_gate_0x90C;
    /* +0x910 */ void (*seq_hook_0x910)(struct _ENEMY_WORK*); /* called every frame while set */
    /* +0x914 */ s16 seq_timer_0x914;
    /* +0x916 */ s16 field_0x916;
    /* +0x918 */ u8 hit_flag_0x918;     /* `em_damage_eval` takes and clears it */
    /* +0x919 */ u8 pad_0x919[0xF];
    /* +0x928 */ s16 extra_damage_0x928; /* damage applied outside the part table */
    /* +0x92A */ u8 pad_0x92A[0x14];
    /* +0x93E */ s16 field_0x93E;
    /* +0x940 */ s16 field_0x940;
    /* +0x942 */ u8 pad_0x942[0xE];
    /* +0x950 */ s16 field_0x950;    /* the counters the script's waits compare (`em_seq_wait_ck`) */
    /* +0x952 */ s16 field_0x952;
    /* +0x954 */ s32** field_0x954;
    /* +0x958 */ s32 field_0x958;
    /* +0x95C */ u8 field_0x95C;
    /* +0x95D */ u8 field_0x95D;
    /* +0x95E */ u8 pad_0x95E[0x1];
    /* +0x95F */ u8 state_0x95F;
    /* +0x960 */ u8 state_0x960;
    /* +0x961 */ u8 pad_0x961[0x1];
    /* +0x962 */ u8 state_0x962;
    /* +0x963 */ u8 bytes_0x963[0xA];
    /* +0x96D */ u8 pad_0x96D[0x3];
    /* +0x970 */ u32 arr_0x970[0xA];
    /* +0x998 */ u8 state_0x998;
    /* +0x999 */ u8 state_0x999;
    /* +0x99A */ u8 state_0x99A;
    /* +0x99B */ u8 pad_0x99B[0x1];
    /* +0x99C */ u8 state_0x99C;
    /* +0x99D */ u8 state_0x99D;
    /* +0x99E */ u8 pad_0x99E[0x2];
    /* +0x9A0 */ u32 ptr_0x9A0;
    /* +0x9A4 */ u32 ptr_0x9A4;
    /* +0x9A8 */ u8 state_0x9A8;
    /* +0x9A9 */ u8 pad_0x9A9[0x3];
    /* +0x9AC */ _ENEMY_QUEUE_ENTRY queue_0x9AC[6];
    /* +0x9DC */ u8 field_0x9DC;
    /* +0x9DD */ u8 field_0x9DD;
    /* +0x9DE */ u8 pad_0x9DE[0x2];
    /* +0x9E0 */ s32 field_0x9E0;
    /* +0x9E4 */ u8 field_0x9E4;
    /* +0x9E5 */ u8 pad_0x9E5[0x3];
    /* +0x9E8 */ u8 state_0x9E8;
    /* +0x9E9 */ u8 state_0x9E9;
    /* +0x9EA */ u8 pad_0x9EA[0x2];
    /* +0x9EC */ u32 ptr_0x9EC;
    /* +0x9F0 */ u32 ptr_0x9F0;
    /* +0x9F4 */ u8 state_0x9F4;
    /* +0x9F5 */ u8 state_0x9F5;
    /* +0x9F6 */ u8 state_0x9F6;
    /* +0x9F7 */ u8 state_0x9F7;
    /* +0x9F8 */ u8 state_0x9F8;
    /* +0x9F9 */ u8 state_0x9F9;
    /* +0x9FA */ u8 state_0x9FA;
    /* +0x9FB */ u8 state_0x9FB;
    /* +0x9FC */ u8 state_0x9FC;
    /* +0x9FD */ u8 state_0x9FD;
    /* +0x9FE */ u8 state_0x9FE;
    /* +0x9FF */ u8 state_0x9FF;
    /* +0xA00 */ u32 ptr_0xA00;
    /* +0xA04 */ u8 flags_0xA04;
    /* +0xA05 */ u8 field_0xA05;
    /* +0xA06 */ u8 field_0xA06;
    /* +0xA07 */ u8 field_0xA07;
    /* +0xA08 */ u8 pad_0xA08[0x5];
    /* +0xA0D */ u8 field_0xA0D;      /* `fn_801592CC` gates a request on it being clear */
    /* +0xA0E */ u8 pad_0xA0E[0x5B];
    /* +0xA69 */ u8 field_0xa69;
    /* +0xA6A */ u8 pad_0xA6A[0x14];
    /* +0xA7E */ u16 field_0xa7e;
    /* +0xA80 */ u8 pad_0xA80[0x70];
    /* +0xAF0 */ s16 field_0xaf0;
    /* +0xAF2 */ u8 pad_0xaf2[0x12];   /* 0xAF2..0xB04 */
    /* +0xB04 */ union {
        u8 run_0xB04[0x10];
        struct {
            /* +0xB04 */ u8 unused_0xB04[0xC];
            /* +0xB10 */ u16 run_angle_0xB10;   /* the em035 run angle block */
            /* +0xB12 */ u8 run_flags_0xB12;
            /* +0xB13 */ u8 unused_0xB13;
        };
    };
    /* +0xB14 */ _se_w* se_handle_0xB14;
    /* +0xB18 */ u8 pad_0xB18[0x4];
};

#include "enemy/EnemyData.h"

#ifdef __cplusplus
}
#endif

#endif /* ENEMY_H */
