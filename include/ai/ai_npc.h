/*
 * ai/ai_npc.h - the AI-NPC work record `_AINPC_W` the `ai` bands operate on, and the attack
 * record `_HIT_W` embedded in it.
 *
 * `_AINPC_W` is the type name the target's own manglings use (`ai_area_ck__FP8_AINPC_W`,
 * `ai_get_motion_no__FP8_AINPC_W`,
 * `get_joint_wpos_ai__FP8_AINPC_WUlPQ34nw4r4math4VEC3`, `ai_skill_ck__FP8_AINPC_WUc` in
 * config/RMHE08/symbols.txt), so every unit that forwards this pointer must spell the type that
 * way or its definitions mangle to a name objdiff cannot pair.
 *
 * The record embeds the actor's `MHchar` model at +0x008: `move__6MHcharFUs`,
 * `get_joint_wpos__6MHcharFUlPQ34nw4r4math4VEC3` and `setVisibility__6MHcharFUlb` are all reached
 * with `self + 8`, and the motion number `ai_get_motion_no` returns is `MHchar::+0x50`.  `MHchar`
 * and `_PLW` come from `include/pl.h`, their owner.
 *
 * `_HIT_W` is this band's view of the attack record - `include/menu/menu_item.h` and
 * `include/Pl/fn_80295EF4.h` carry their own views of the same type (both record it as rule-1
 * debt), and none of the three names the offsets this band reads (+0x08, +0x18..+0x1E, +0x31,
 * +0x32, +0x40..+0x4E).  Folding the views into one union-aware definition is the rule-1
 * follow-up this header records for the unit.
 *
 * Every field carries its offset in ascending order and a name from the call sites that read it;
 * offsets no reconstructed function touches stay padding so the numbers stay exact.  `_AINPC_W`'s
 * size is a lower bound: the highest offset the band touches is +0x498.
 *
 * RULE-1 DEBT, filed for the next `ai` worker: `include/ai/ainpc.h` is the *other* home of this same
 * record - the view `src/ai/fn_802CC794.cpp` and src/ai/fn_802C474C.cpp share (same size, 0x49C, and
 * the same offsets for every field both name).  This header is the 0x802D0F34 band's view: it embeds
 * the actor model as `MHchar` (which is what makes `self + 8` and `MHchar::get_joint_wpos` type out)
 * and names the band's own tail.  Folding the two into `ai/ainpc.h` (or the reverse) is the follow-up;
 * until then a unit must include exactly one of them, because both define `_AINPC_W` and
 * `AINPCFormation`.
 */
#ifndef MHTRI_AI_AI_NPC_H
#define MHTRI_AI_AI_NPC_H

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"

/* The 0x10-byte formation record `+0x41C` points at - the AI skirmish line's member spacing, read
 * as a 0-100 percentage by `fn_802D4020`.  size: 0x14 (the band reads +0x10; 4-aligned) */
struct AINPCFormation {
    /* +0x00 */ u8 unused_0x00[0x10];
    /* +0x10 */ u8 attack_rate;        /* the percentage `fn_802D4020` rolls against */
};

/* One attack entry of the two the AI NPC carries at +0x240 (stride 0x5C).  `hit_flag_set` /
 * `fn_8029F538` are the shared registry helpers and take this pointer, which is what fixes the
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
                                        * `fn_8029F538` clears */
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
 * size: 0x49C (lower bound - the highest offset the band touches is +0x498) */
/* One slot of the +0x390 ring and of the +0x3B0 pending block: the attack id and its count.
 * size: 0x4 */
struct HitSlot {
    /* +0x0 */ u16 id;
    /* +0x2 */ s16 count;
};

struct _AINPC_W {
    /* +0x000 */ u8 unused_0x000;
    /* +0x001 */ u8 in_area;           /* set from `area_0x1A4 == get_now_areano()` */
    /* +0x002 */ u8 field_0x002;       /* set to 1 by the state setter; zero suppresses +0x3C */
    /* +0x003 */ u8 unused_0x003[0x004 - 0x003];
    /* +0x004 */ u8 field_0x004;       /* the three bytes the motion setter clears */
    /* +0x005 */ u8 field_0x005;
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
    /* +0x190 */ _CP_VECTOR vec_0x190; /* the model rotation `fn_800FC0D4` copies out of the player
                                        * work; its y word is the model's facing angle */
    /* +0x19C */ f32 field_0x19C;      /* added to +0x188 before the move blend is applied */
    /* +0x1A0 */ f32 field_0x1A0;      /* the effect height offset */
    /* +0x1A4 */ u8 area;              /* the area the NPC lives in */
    /* +0x1A5 */ u8 unused_0x1A5[0x1B0 - 0x1A5];
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
    /* +0x1FA */ u8 unused_0x1FA[0x1FE - 0x1FA];
    /* +0x1FE */ u16 field_0x1FE;
    /* +0x200 */ u16 field_0x200;
    /* +0x202 */ u8 unused_0x202[0x210 - 0x202];
    /* +0x210 */ u16 field_0x210;      /* the attack id stamped into every hit entry */
    /* +0x212 */ u16 field_0x212;      /* the counter-attack window `fn_802D2264` drains */
    /* +0x214 */ s16 field_0x214;      /* a countdown set by `fn_802D3A2C` */
    /* +0x216 */ u8 field_0x216;       /* the gate at the top of `fn_802D2264` */
    /* +0x217 */ u8 unused_0x217[0x21A - 0x217];
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
                                        * `fn_802D29F8`'s dispatcher reads back */
    /* +0x2F8 */ u8 unused_0x2F8[0x321 - 0x2F8];
    /* +0x321 */ u8 field_0x321;       /* `fn_802D3E4C` sets it, `fn_802D3F08` reads it */
    /* +0x322 */ u8 unused_0x322[0x324 - 0x322];
    /* +0x324 */ u16 field_0x324;      /* the damage-type flag word `fn_802D4248` tests */
    /* +0x326 */ u8 unused_0x326[0x32C - 0x326];
    /* +0x32C */ nw4r::math::VEC3 pos_0x32C;
    /* +0x338 */ u8 unused_0x338;
    /* +0x339 */ u8 field_0x339;       /* the 0..0x14 progress `fn_802D3984`/`fn_802D3F70`
                                        * interpolate with */
    /* +0x33A */ u8 unused_0x33A[0x348 - 0x33A];
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
    /* +0x355 */ u8 unused_0x355[0x358 - 0x355];
    /* +0x358 */ u8 field_0x358;
    /* +0x359 */ u8 unused_0x359;
    /* +0x35A */ s16 field_0x35A;
    /* +0x35C */ u8 field_0x35C;
    /* +0x35D */ u8 unused_0x35D;
    /* +0x35E */ s16 field_0x35E;
    /* +0x360 */ u8 unused_0x360[0x374 - 0x360];
    /* +0x374 */ s16 field_0x374;
    /* +0x376 */ u8 field_0x376;       /* cleared with +0x378 by `fn_802D4238` */
    /* +0x377 */ u8 unused_0x377;
    /* +0x378 */ u8 field_0x378;
    /* +0x379 */ u8 unused_0x379[0x382 - 0x379];
    /* +0x382 */ u8 field_0x382;       /* cleared with +0x34B by `fn_802D3B24` */
    /* +0x383 */ u8 field_0x383;       /* set when the hit entry was filled */
    /* +0x384 */ u8 field_0x384;
    /* +0x385 */ u8 unused_0x385[0x389 - 0x385];
    /* +0x389 */ u8 field_0x389;
    /* +0x38A */ s16 field_0x38A;
    /* +0x38C */ s16 field_0x38C;      /* the aim-build timer `fn_802D1C90` runs up */
    /* +0x38E */ u8 field_0x38E;       /* latched when +0x38C reaches its limit */
    /* +0x38F */ u8 unused_0x38F[0x390 - 0x38F];
    /* +0x390 */ struct HitSlot slots[12];  /* slots 0-7 are the ring `fn_802D40A4` appends to,
                                             * 8-11 the four deferred slots `fn_802D40EC` fills; the
                                             * target indexes all twelve from the one base */
    /* +0x3C0 */ u16 entry_index;      /* the ring cursor, wrapping at 8 */
    /* +0x3C2 */ u8 unused_0x3C2[0x3C4 - 0x3C2];
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
    /* +0x3D2 */ u8 unused_0x3D2[0x3D4 - 0x3D2];
    /* +0x3D4 */ s16 field_0x3D4;
    /* +0x3D6 */ s16 field_0x3D6;
    /* +0x3D8 */ u8 unused_0x3D8[0x3DC - 0x3D8];
    /* +0x3DC */ s16 field_0x3DC;      /* the countdown that calls `fn_802D6B2C` */
    /* +0x3DE */ u8 field_0x3DE;       /* cleared by `fn_802D3D84` */
    /* +0x3DF */ u8 unused_0x3DF[0x408 - 0x3DF];
    /* +0x408 */ s16 field_0x408;      /* the block of countdowns `fn_802D36B8` decrements */
    /* +0x40A */ s16 field_0x40A;
    /* +0x40C */ s16 field_0x40C;
    /* +0x40E */ s16 field_0x40E;
    /* +0x410 */ s16 field_0x410;
    /* +0x412 */ s16 field_0x412;
    /* +0x414 */ u8 unused_0x414[0x416 - 0x414];
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
    /* +0x42E */ u8 unused_0x42E[0x43C - 0x42E];
    /* +0x43C */ u8 field_0x43C;       /* the count `fn_802D348C` bumps below 7 */
    /* +0x43D */ u8 field_0x43D;
    /* +0x43E */ s16 field_0x43E;      /* a countdown `fn_802D36B8` runs down */
    /* +0x440 */ u8 field_0x440;
    /* +0x441 */ u8 unused_0x441[0x445 - 0x441];
    /* +0x445 */ u8 field_0x445;
    /* +0x446 */ u8 unused_0x446;
    /* +0x447 */ u8 field_0x447;       /* the roll `fn_802D1710` stores */
    /* +0x448 */ u8 field_0x448;
    /* +0x449 */ u8 unused_0x449[0x44C - 0x449];
    /* +0x44C */ f32 field_0x44C;
    /* +0x450 */ u8 field_0x450;
    /* +0x451 */ u8 field_0x451;
    /* +0x452 */ u8 unused_0x452[0x460 - 0x452];
    /* +0x460 */ u8 field_0x460;
    /* +0x461 */ u8 field_0x461;
    /* +0x462 */ u16 field_0x462;
    /* +0x464 */ s16 field_0x464;
    /* +0x466 */ u8 unused_0x466[0x468 - 0x466];
    /* +0x468 */ u8 field_0x468;
    /* +0x469 */ u8 unused_0x469;
    /* +0x46A */ s16 field_0x46A;
    /* +0x46C */ u8 field_0x46C;
    /* +0x46D */ u8 unused_0x46D[0x47C - 0x46D];
    /* +0x47C */ s16 field_0x47C;
    /* +0x47E */ u8 field_0x47E;       /* non-zero arms +0x480 */
    /* +0x47F */ u8 unused_0x47F;
    /* +0x480 */ s16 field_0x480;
    /* +0x482 */ u8 field_0x482;
    /* +0x483 */ u8 unused_0x483[0x485 - 0x483];
    /* +0x485 */ u8 field_0x485;
    /* +0x486 */ s16 field_0x486;
    /* +0x488 */ u8 unused_0x488[0x492 - 0x488];
    /* +0x492 */ u8 field_0x492;
    /* +0x493 */ u8 unused_0x493[0x498 - 0x493];
    /* +0x498 */ u32 sound_handle_0x498;  /* the sound handler the attack drivers take */
};

#endif
