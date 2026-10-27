/*
 * ai/ai_npc.h - the 0x802D0F34-0x802D44F4 bodies' view of the AI companion record `_AINPC_W` and of the attack record
 *   `_HIT_W` it embeds, read inside `ai/ai_npc.cpp`'s `namespace view_fn_802D0F34` (and by `ai/fn_802D0F34.h`).
 * SHAPES. `_AINPC_W` is the spelling the map's manglings use (`ai_area_ck__FP8_AINPC_W`, `ai_get_motion_no__FP8_AINPC_W`,
 *   `get_joint_wpos_ai__FP8_AINPC_WUlPQ34nw4r4math4VEC3`); the record embeds the actor's `MHchar` at +0x008
 *   (`move__6MHcharFUs`, `get_joint_wpos__6MHcharFUlPQ34nw4r4math4VEC3`, `setVisibility__6MHcharFUlb` take `self + 8`).
 *  - `ai/ainpc.h` is the other view of the same record and `Pl/hit_w.h` the shared `_HIT_W`; both pairs define the same
 *    tags, so a unit includes one of each pair.
 */
#ifndef MHTRI_AI_AI_NPC_H
#define MHTRI_AI_AI_NPC_H

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"
#include "Pl/em_weak_point.h"

/* One tuning row of a formation: what it adjusts (0 attack, 1 defence, 2 speed, 3 gauge) and the amount per
 * companion level.  size: 0xE */
struct AINPCTuning {
    /* +0x00 */ u8 kind;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ s16 values[6];
};

/* The formation record `+0x41C` points at: its two tuning rows and the attack rate `fn_802D4020` rolls
 * against.  size: 0x14 (the band reads +0x10; 4-aligned) */
struct AINPCFormation {
    /* +0x00 */ struct AINPCTuning* tuning_0x00;  /* two rows, applied by `ai_npc_behaviour_apply` */
    /* +0x04 */ u8 unused_0x04[0x10 - 0x04];
    /* +0x10 */ u8 attack_rate;        /* the percentage `fn_802D4020` rolls against */
};

/* A companion setup the lobby previews: the formation, the three skills and the three tuning parameters.
 * size: 0xA */
struct AINPCSetup {
    /* +0x0 */ u8 behaviour;
    /* +0x1 */ u8 skills[3];
    /* +0x4 */ u16 attack_level;
    /* +0x6 */ u16 gauge_level;
    /* +0x8 */ u16 level;
};

/* The status preview `ai_npc_status_preview` fills from a setup.  size: 0x1C */
struct AINPCStatus {
    /* +0x00 */ u8 attack_rank;
    /* +0x01 */ u8 gauge_rank;
    /* +0x02 */ s16 rate;            /* always 100 */
    /* +0x04 */ s16 gauge_bonus;     /* the formation's gauge adjustment */
    /* +0x06 */ s16 attack;
    /* +0x08 */ s16 attack_bonus;
    /* +0x0A */ u8 status_kind;      /* the status attack the skills give (0 none) */
    /* +0x0B */ u8 pad_0x0B;
    /* +0x0C */ s16 status_attack;
    /* +0x0E */ s16 status_bonus;
    /* +0x10 */ s16 defense;
    /* +0x12 */ s16 defense_bonus;
    /* +0x14 */ u8 level;
    /* +0x15 */ u8 pad_0x15;
    /* +0x16 */ s16 gauge;
    /* +0x18 */ s16 gauge_add;
    /* +0x1A */ u8 speed_rank;
    /* +0x1B */ u8 pad_0x1B;
};

/* One weak point of an enemy's list: `Pl/em_weak_point.h`'s `EmWeakPoint` (rule 1). */

/* One attack entry of the two the AI NPC carries at +0x240 (stride 0x5C).  `hit_flag_set` /
 * `hit_flags_clear` are the shared registry helpers and take this pointer, which is what fixes the
 * type (`hit_flag_set__FP6_HIT_WUl`). size: 0x5C */
struct _HIT_W {
    /* +0x00 */ u8 unused_0x00[0x05];
    /* +0x05 */ u8 field_0x05;        /* cleared when the entry's offsets collapse */
    /* +0x06 */ u8 unused_0x06[0x08 - 0x06];
    /* +0x08 */ s32 field_0x08;        /* the motion/attack id `fn_802D2F7C` stores */
    /* +0x0C */ u8 unused_0x0C[0x0E - 0x0C];
    /* +0x0E */ u8 field_0x0E;        /* the attack's weak-point rate `fn_802D3F1C` takes the
                                        * highest of */
    /* +0x0F */ u8 unused_0x0F[0x18 - 0x0F];
    /* +0x18 */ u16 motion_no_0x18;    /* the NPC's motion number when the entry was filled */
    /* +0x1A */ u16 field_0x1A;
    /* +0x1C */ u16 field_0x1C;
    /* +0x1E */ u16 field_0x1E;
    /* +0x20 */ u32 flags;             /* the registry flag word `hit_flag_set` ORs into and
                                        * `hit_flags_clear` clears */
    /* +0x24 */ u8 unused_0x24[0x31 - 0x24];
    /* +0x31 */ u8 field_0x31;         /* the attack selector: 0x0A for a fresh entry, 1 on a
                                        * blocked one */
    /* +0x32 */ u8 field_0x32;         /* the second selector byte (4 for a fresh entry) */
    /* +0x33 */ u8 unused_0x33[0x38 - 0x33];
    /* +0x38 */ f32 offset_0x38;       /* the entry's distance offset `fn_802D2F7C` fills */
    /* +0x3C */ f32 offset_0x3C;
    /* +0x40 */ s16 field_0x40;        /* the damage/rate value `fn_802D2BB0` scales */
    /* +0x42 */ u16 field_0x42;        /* the hit kind (`0x11` for an active counter-attack) */
    /* +0x44 */ u8 unused_0x44[0x48 - 0x44];
    /* +0x48 */ u16 field_0x48;        /* the entry's own flag word (`fn_802D2BB0` ORs the attack
                                        * ids in, `fn_802D2F7C` sets 0x320) */
    /* +0x4A */ u8 field_0x4A;         /* the percentage `fn_802D2BB0` scales down */
    /* +0x4B */ u8 unused_0x4B;
    /* +0x4C */ u8 field_0x4C;         /* flag byte, 0x80 = the weak-point bonus was applied */
    /* +0x4D */ u8 unused_0x4D;
    /* +0x4E */ s8 field_0x4E;         /* the signed rate `fn_802D2BB0` scales */
    /* +0x4F */ u8 unused_0x4F[0x5C - 0x4F];
};

/* The AI-NPC work record.  `self + 8` is the actor model; the band's own state starts after it.
 * size: 0x4A8 (the map's size of the `ainpc_w` object) */
/* One slot of the +0x390 ring and of the +0x3B0 pending block: the attack id and its count.
 * size: 0x4 */
struct HitSlot {
    /* +0x0 */ u16 id;
    /* +0x2 */ s16 count;
};

struct _AINPC_W {
    /* +0x000 */ u8 active;            /* non-zero while the record is in use */
    /* +0x001 */ u8 in_area;           /* set from `area_0x1A4 == get_now_areano()` */
    /* +0x002 */ u8 field_0x002;       /* set to 1 by the state setter; zero suppresses +0x3C */
    /* +0x003 */ u8 phase;             /* 0 until `ainpc_start` has set the record up (it bumps it to 1);
                                        * `ainpc_frame` switches on it */
    /* +0x004 */ u8 state;             /* the motion's own sub-state every arm handler switches on (the setter clears
                                        * it with +0x005/+0x006) */
    /* +0x005 */ u8 sub_step;          /* the sub-state's counter (repeats left) */
    /* +0x006 */ u8 field_0x006;
    /* +0x007 */ u8 unused_0x007;
    /* +0x008 */ MHchar model;         /* size 0x140; the actor's joint/model block */
    /* +0x148 */ u8 unused_0x148[0x16C - 0x148];
    /* +0x16C */ _PLW* plw_0x16C;      /* the player work it targets (its +0x16 is the player's
                                        * area, its +0x3C the position) */
    /* +0x170 */ u8 variant;           /* 1 or 2 - selects the locomotion arm */
    /* +0x171 */ u8 motion;            /* the current motion/state id */
    /* +0x172 */ u16 motion_step;      /* the motion's sub-step */
    /* +0x174 */ u8 field_0x174;       /* set to 1 when the motion setter is called through
                                        * `fn_802D2ABC` */
    /* +0x175 */ u8 prev_motion;        /* the outgoing motion the setter saves */
    /* +0x176 */ u16 prev_motion_step;
    /* +0x178 */ nw4r::math::VEC3 pos_0x178;
    /* +0x184 */ nw4r::math::VEC3 pos_0x184;  /* the previous frame's position */
    /* +0x190 */ _CP_VECTOR vec_0x190; /* the model rotation `eft_rot_vec_copy` copies out of the player
                                        * work; its y word is the model's facing angle */
    /* +0x19C */ f32 field_0x19C;      /* added to +0x188 before the move blend is applied */
    /* +0x1A0 */ f32 field_0x1A0;      /* the effect height offset */
    /* +0x1A4 */ u8 area;              /* the area the NPC lives in */
    /* +0x1A5 */ u8 unused_0x1A5[0x1AC - 0x1A5];
    /* +0x1AC */ s32 timer_0x1AC;            /* the motion countdown the arm handlers seed with a random 5..20 */
    /* +0x1B0 */ nw4r::math::VEC3 pos_0x1B0;  /* the last accepted move target */
    /* +0x1BC */ f32 field_0x1BC;
    /* +0x1C0 */ f32 field_0x1C0;
    /* +0x1C4 */ u8 enemy_index;       /* scaled by 0xB18 into the `_ENEMY_WORK` array */
    /* +0x1C5 */ u8 unused_0x1C5[0x1C8 - 0x1C5];
    /* +0x1C8 */ u32 field_0x1C8;
    /* +0x1CC */ u8 field_0x1CC;
    /* +0x1CD */ u8 field_0x1CD;       /* set by `fn_802D41C8`, cleared by `fn_802D41D4` */
    /* +0x1CE */ s8 field_0x1CE;       /* the attack selector `fn_802D41E0` latches */
    /* +0x1CF */ u8 field_0x1CF;       /* set by `fn_802D4200` */
    /* +0x1D0 */ u8 field_0x1D0;       /* the step counter `fn_802D4200` clears */
    /* +0x1D1 */ u8 field_0x1D1;       /* 1 = `fn_802D4218`, 2 = `fn_802D4224` was called */
    /* +0x1D2 */ u8 unused_0x1D2[0x1D4 - 0x1D2];
    /* +0x1D4 */ nw4r::math::VEC3 pos_0x1D4;
    /* +0x1E0 */ nw4r::math::VEC3 pos_0x1E0;
    /* +0x1EC */ u16 field_0x1EC;      /* the flag word the accessors at +0x1EC test/set */
    /* +0x1EE */ u8 unused_0x1EE[0x1F0 - 0x1EE];
    /* +0x1F0 */ s32 gauge_0x1F0;      /* the motion gauge the drivers drain and refill */
    /* +0x1F4 */ s32 gauge_max_0x1F4;
    /* +0x1F8 */ s16 gauge_refill_0x1F8;
    /* +0x1FA */ u16 field_0x1FA;                 /* the attack `ai_npc_attack_update` derives */
    /* +0x1FC */ u16 field_0x1FC;                 /* the defence `ai_npc_defense_update` derives */
    /* +0x1FE */ u16 field_0x1FE;
    /* +0x200 */ u16 field_0x200;
    /* +0x202 */ u8 unused_0x202[0x204 - 0x202];
    /* +0x204 */ f32 field_0x204;
    /* +0x208 */ f32 field_0x208;
    /* +0x20C */ u32 behaviour_flags;        /* the behaviour flag word `fn_802D6888` assembles */
    /* +0x210 */ u16 field_0x210;      /* the attack id stamped into every hit entry */
    /* +0x212 */ u16 field_0x212;      /* the counter-attack window `fn_802D2264` drains */
    /* +0x214 */ s16 field_0x214;      /* a countdown set by `fn_802D3A2C` */
    /* +0x216 */ u8 field_0x216;       /* the gate at the top of `fn_802D2264` */
    /* +0x217 */ u8 unused_0x217;
    /* +0x218 */ u16 field_0x218;
    /* +0x21A */ u8 field_0x21A;       /* the attack selector the dispatcher switches on */
    /* +0x21B */ u8 unused_0x21B;
    /* +0x21C */ s16 field_0x21C;
    /* +0x21E */ s16 field_0x21E;
    /* +0x220 */ s16 field_0x220;
    /* +0x222 */ s16 field_0x222;
    /* +0x224 */ s16 field_0x224;
    /* +0x226 */ s16 field_0x226;
    /* +0x228 */ s16 field_0x228;
    /* +0x22A */ s16 field_0x22A;
    /* +0x22C */ s16 field_0x22C;
    /* +0x22E */ s16 field_0x22E;
    /* +0x230 */ s16 field_0x230;
    /* +0x232 */ s16 field_0x232;
    /* +0x234 */ s8 field_0x234;
    /* +0x235 */ u8 field_0x235;
    /* +0x236 */ s16 field_0x236;
    /* +0x238 */ s8 field_0x238;
    /* +0x239 */ u8 field_0x239;
    /* +0x23A */ s16 field_0x23A;
    /* +0x23C */ u8 field_0x23C;
    /* +0x23D */ u8 unused_0x23D;
    /* +0x23E */ s16 field_0x23E;      /* the countdown that clears +0x23C */
    /* +0x240 */ struct _HIT_W hit[2]; /* the two attack entries `fn_802D2F7C` fills and
                                        * `ai_npc_variant_set`'s dispatcher reads back */
    /* +0x2F8 */ u16 hit_ids[0x14];          /* the hit-id list `hit_id_list_clear` empties */
    /* +0x320 */ u8 hit_id_count;
    /* +0x321 */ u8 field_0x321;       /* `fn_802D3E4C` sets it, `fn_802D3F08` reads it */
    /* +0x322 */ u8 unused_0x322[0x324 - 0x322];
    /* +0x324 */ u16 field_0x324;      /* the damage-type flag word `fn_802D4248` tests */
    /* +0x326 */ u8 field_0x326;
    /* +0x327 */ u8 unused_0x327[0x32C - 0x327];
    /* +0x32C */ nw4r::math::VEC3 pos_0x32C;
    /* +0x338 */ u8 field_0x338;
    /* +0x339 */ u8 field_0x339;       /* the 0..0x14 progress `fn_802D3984`/`fn_802D3F70`
                                        * interpolate with */
    /* +0x33A */ u8 field_0x33A;
    /* +0x33B */ u8 unused_0x33B;
    /* +0x33C */ u16 field_0x33C;
    /* +0x33E */ u16 field_0x33E;
    /* +0x340 */ u16 field_0x340;
    /* +0x342 */ u16 field_0x342;
    /* +0x344 */ u16 hunt_bonus;             /* the hunt-rank bonus `ai_npc_bonus_add` adds up to 100 */
    /* +0x346 */ u16 time_bonus;             /* the time-rank bonus `ai_npc_bonus_add` adds up to 10 */
    /* +0x348 */ u8 field_0x348;
    /* +0x349 */ u8 field_0x349;
    /* +0x34A */ u8 field_0x34A;       /* the countdown `fn_802D36B8` runs down */
    /* +0x34B */ u8 field_0x34B;
    /* +0x34C */ u8 field_0x34C;
    /* +0x34D */ u8 field_0x34D;       /* cleared by `fn_802D3B10` */
    /* +0x34E */ u8 field_0x34E;       /* armed to 0xFF by `fn_802D3B10`, compared with 1 by
                                        * `fn_802D2BB0` */
    /* +0x34F */ u8 field_0x34F;       /* cleared by `fn_802D3A20` */
    /* +0x350 */ s16 field_0x350;
    /* +0x352 */ s16 field_0x352;      /* the countdown that arms +0x354 */
    /* +0x354 */ u8 field_0x354;
    /* +0x355 */ u8 unused_0x355;
    /* +0x356 */ u16 field_0x356;
    /* +0x358 */ u8 field_0x358;
    /* +0x359 */ u8 unused_0x359;
    /* +0x35A */ s16 field_0x35A;
    /* +0x35C */ u8 field_0x35C;
    /* +0x35D */ u8 unused_0x35D;
    /* +0x35E */ s16 field_0x35E;
    /* +0x360 */ u8 unused_0x360[0x362 - 0x360];
    /* +0x362 */ u8 field_0x362;
    /* +0x364 */ u32 field_0x364;
    /* +0x368 */ nw4r::math::VEC3 pos_0x368;
    /* +0x374 */ s16 field_0x374;
    /* +0x376 */ u8 field_0x376;       /* cleared with +0x378 by `fn_802D4238` */
    /* +0x377 */ u8 field_0x377;
    /* +0x378 */ u8 field_0x378;
    /* +0x379 */ u8 unused_0x379[0x37C - 0x379];
    /* +0x37C */ f32 field_0x37C;
    /* +0x380 */ u16 field_0x380;
    /* +0x382 */ u8 field_0x382;       /* cleared with +0x34B by `fn_802D3B24` */
    /* +0x383 */ u8 field_0x383;       /* set when the hit entry was filled */
    /* +0x384 */ u8 field_0x384;
    /* +0x385 */ u8 unused_0x385;
    /* +0x386 */ s16 field_0x386;
    /* +0x388 */ u8 unused_0x388;
    /* +0x389 */ u8 field_0x389;
    /* +0x38A */ s16 field_0x38A;
    /* +0x38C */ s16 field_0x38C;      /* the aim-build timer `fn_802D1C90` runs up */
    /* +0x38E */ u8 field_0x38E;       /* latched when +0x38C reaches its limit */
    /* +0x38F */ u8 unused_0x38F[0x390 - 0x38F];
    /* +0x390 */ struct HitSlot slots[12];  /* slots 0-7 are the ring `fn_802D40A4` appends to,
                                             * 8-11 the four deferred slots `fn_802D40EC` fills; the
                                             * target indexes all twelve from the one base */
    /* +0x3C0 */ u16 entry_index;      /* the ring cursor, wrapping at 8 */
    /* +0x3C2 */ u16 field_0x3C2;
    /* +0x3C4 */ u8 field_0x3C4;
    /* +0x3C5 */ u8 unused_0x3C5;
    /* +0x3C6 */ s16 field_0x3C6;      /* the three timers `fn_802D3B34` advances */
    /* +0x3C8 */ s8 field_0x3C8;       /* non-zero arms +0x3C6 */
    /* +0x3C9 */ u8 unused_0x3C9;
    /* +0x3CA */ s16 field_0x3CA;
    /* +0x3CC */ s16 field_0x3CC;
    /* +0x3CE */ s8 field_0x3CE;       /* non-zero arms +0x3CC */
    /* +0x3CF */ u8 unused_0x3CF;
    /* +0x3D0 */ s16 field_0x3D0;
    /* +0x3D2 */ u8 field_0x3D2;
    /* +0x3D3 */ u8 unused_0x3D3;
    /* +0x3D4 */ s16 field_0x3D4;
    /* +0x3D6 */ s16 field_0x3D6;
    /* +0x3D8 */ u8 unused_0x3D8[0x3DA - 0x3D8];
    /* +0x3DA */ s16 field_0x3DA;
    /* +0x3DC */ s16 field_0x3DC;      /* the countdown that calls `fn_802D6B2C` */
    /* +0x3DE */ u8 field_0x3DE;       /* cleared by `fn_802D3D84` */
    /* +0x3DF */ u8 unused_0x3DF;
    /* +0x3E0 */ nw4r::math::VEC3 pos_0x3E0;
    /* +0x3EC */ u16 field_0x3EC;
    /* +0x3EE */ u16 field_0x3EE;
    /* +0x3F0 */ u8 field_0x3F0;
    /* +0x3F1 */ u8 unused_0x3F1;
    /* +0x3F2 */ u16 field_0x3F2;
    /* +0x3F4 */ u16 field_0x3F4;
    /* +0x3F6 */ u8 field_0x3F6;
    /* +0x3F7 */ u8 field_0x3F7;
    /* +0x3F8 */ u8 field_0x3F8[4];
    /* +0x3FC */ u8 field_0x3FC[4];
    /* +0x400 */ u16 field_0x400[4];
    /* +0x408 */ s16 field_0x408;      /* the block of countdowns `fn_802D36B8` decrements */
    /* +0x40A */ s16 field_0x40A;
    /* +0x40C */ s16 field_0x40C;
    /* +0x40E */ s16 field_0x40E;
    /* +0x410 */ s16 field_0x410;
    /* +0x412 */ s16 field_0x412;
    /* +0x414 */ s16 field_0x414;
    /* +0x416 */ u8 field_0x416;       /* the distance band `fn_802D1C90` latches */
    /* +0x417 */ u8 field_0x417;
    /* +0x418 */ u8 field_0x418;
    /* +0x419 */ u8 unused_0x419[0x41C - 0x419];
    /* +0x41C */ struct AINPCFormation* formation_0x41C;
    /* +0x420 */ u8 field_0x420;
    /* +0x421 */ u8 unused_0x421;
    /* +0x422 */ s16 field_0x422;      /* the counter-attack countdown `fn_802D36B8` runs down */
    /* +0x424 */ s16 field_0x424;
    /* +0x426 */ u8 field_0x426;       /* the weak-point index `fn_802D1AFC` rolls */
    /* +0x427 */ u8 weak_point_0x427[5];  /* the five weak-point rates it bumps */
    /* +0x42C */ u16 field_0x42C;
    /* +0x42E */ u8 skill_0x42E[3];          /* the three skill slots `ai_skill_ck` compares against */
    /* +0x431 */ u8 field_0x431;
    /* +0x432 */ u8 unused_0x432[0x434 - 0x432];
    /* +0x434 */ s32 field_0x434;
    /* +0x438 */ s32 field_0x438;
    /* +0x43C */ u8 field_0x43C;       /* the count `fn_802D348C` bumps below 7 */
    /* +0x43D */ u8 field_0x43D;
    /* +0x43E */ s16 field_0x43E;      /* a countdown `fn_802D36B8` runs down */
    /* +0x440 */ u8 field_0x440;
    /* +0x441 */ u8 unused_0x441[0x445 - 0x441];
    /* +0x445 */ u8 field_0x445;
    /* +0x446 */ u8 field_0x446;
    /* +0x447 */ u8 field_0x447;       /* the roll `fn_802D1710` stores */
    /* +0x448 */ u8 field_0x448;
    /* +0x449 */ u8 unused_0x449[0x44C - 0x449];
    /* +0x44C */ f32 field_0x44C;
    /* +0x450 */ u8 field_0x450;
    /* +0x451 */ u8 field_0x451;
    /* +0x452 */ u8 unused_0x452[0x454 - 0x452];
    /* +0x454 */ nw4r::math::VEC3 pos_0x454;
    /* +0x460 */ u8 field_0x460;
    /* +0x461 */ u8 field_0x461;
    /* +0x462 */ u16 field_0x462;
    /* +0x464 */ s16 field_0x464;
    /* +0x466 */ u8 unused_0x466[0x468 - 0x466];
    /* +0x468 */ u8 field_0x468;
    /* +0x469 */ u8 unused_0x469;
    /* +0x46A */ s16 field_0x46A;
    /* +0x46C */ u8 field_0x46C;
    /* +0x46D */ u8 unused_0x46D[0x470 - 0x46D];
    /* +0x470 */ nw4r::math::VEC3 drift_vel;  /* added to the position each frame while `drift_active` is set */
    /* +0x47C */ s16 field_0x47C;
    /* +0x47E */ u8 drift_active;      /* non-zero applies `drift_vel` (`ainpc_move`) and arms +0x480 */
    /* +0x47F */ u8 unused_0x47F;
    /* +0x480 */ s16 field_0x480;
    /* +0x482 */ u8 field_0x482;
    /* +0x483 */ u8 field_0x483;
    /* +0x484 */ u8 field_0x484;
    /* +0x485 */ u8 field_0x485;
    /* +0x486 */ s16 field_0x486;
    /* +0x488 */ u32 field_0x488;
    /* +0x48C */ u32 field_0x48C;
    /* +0x490 */ u32 field_0x490;
    /* +0x494 */ u32 field_0x494;
    /* +0x498 */ u32 sound_handle_0x498;  /* the sound handler the attack drivers take */
    /* +0x49C */ u32 sound_handle_0x49C;  /* the second SE entry `se_entry_request(0xB, ...)` returns */
    /* +0x4A0 */ u8 unused_0x4A0[0x4A8 - 0x4A0];
};

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802D78FC - whether the player is inside the AI NPC's current attack reach; C linkage (the map row is
 * unmangled). */
s32 ai_npc_player_in_reach_ck(void);

/* 0x802D2B78 */
s32 ai_npc_flag_ck(struct _AINPC_W* self, s32 mask);

#ifdef __cplusplus
}
#endif

#endif
