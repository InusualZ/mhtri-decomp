/* The enemy-side work record (`get_move_work_adrs(3)`, 0xB18 apart) - the one shared home for
 * `_ENEMY_WORK` (docs/plan.md 6.5 rule 1: a type more than one unit uses is defined once and
 * included where needed).
 *
 * The layout is the union of the field sets the `enemy/` units have measured so far.  Every field
 * carries its offset (rule 4) and a name from its call site (rule 5); bytes the reconstructed
 * functions never touch are kept as `unused_0xNN` padding so the offsets stay exact.  First landed
 * with `enemy/fn_80177890.cpp`; the `_ENEMY_WORK` definitions still in `src/enemy/*` are the
 * pre-header copies the residual sweep will fold into this include.
 *
 * `pos` is `nw4r::math::VEC3`, so this header is C++-only (as `include/nw4r/math.h` is).
 */
#ifndef MHTRI_ENEMY_ENEMY_WORK_H
#define MHTRI_ENEMY_ENEMY_WORK_H

#include "types.h"
#include "nw4r/math.h"

/* One 0x03-byte entry of the enemy program table `_ENEMY_WORK::prog_0xA00` points at (`fn_8013F764`
 * indexes it with a stride of 3: `entries[count - 1 - index]`, or `entries[index]` when the
 * `field_0x9FD` bit is clear).
 * size: 0x03 */
struct EmProgEntry {
    /* +0x0 */ u8 code;       /* `fn_801262BC`'s lookup key */
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

/* The 0xB20-byte records `get_move_work_adrs(2)` hands back (`fn_8013FD98` steps a whole record at a
 * time and asks `fn_8027D530` whether the candidate is the one it wants).  Only the two bytes the
 * search reads are named; the rest is the record's own layout, not this unit's business.
 * size: 0xB20 */
struct EmAreaWork {
    /* +0x000 */ u8 active;
    /* +0x001 */ u8 unused_0x001[0x016 - 0x001];
    /* +0x016 */ u8 area_no;
    /* +0x017 */ u8 unused_0x017[0xB20 - 0x017];
};

/* One 0x04-byte `{code, value}` action record of the list `fn_80126494` returns the head of
 * (`fn_801409C8` scans it for the action `_ENEMY_WORK::field_0x7C8` names).
 * size: 0x04 */
struct EmActRec {
    /* +0x0 */ u8 code;
    /* +0x1 */ u8 unused_0x01;
    /* +0x2 */ s16 value;
};

/* The file row `fn_80140DAC` copies (the 19 words of the table `fn_800D4DD8` indexes).  The unit
 * only copies the record - it reads none of its fields - so they stay `unused_0xNN`, at the offsets
 * the target's word-by-word copy names.
 * size: 0x4C */
/* The 0x4C-byte file row `fn_80140DAC` copies (the table `fn_800D4DD8` indexes).  This unit only
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

/* One 0x08-byte `{key, name}` entry of the two file tables `fn_80141050` walks (`lbl_80581E20`'s
 * entries are loaded through `fn_80140E48`, `lbl_80582348`'s through `fn_80140FB0`).
 * size: 0x08 */
struct EmFileEntry {
    /* +0x0 */ u32 key_0x00;   /* nonzero, the row is loaded */
    /* +0x4 */ char* name_0x04;
};

/* One 0x14-byte per-motion slot of `_ENEMY_WORK::slots_0x244` (`fn_80137EE0` averages the angles
 * `calcVecAngXY` derives from the slot's vector).
 * size: 0x14 */
struct EmMotionSlot {
    /* +0x000 */ u16 flags;            /* bit 0x800 admits the slot to the average */
    /* +0x002 */ u8 unused_0x002[0x004 - 0x002];
    /* +0x004 */ u32 value;            /* the slot's own angle/id, filtered when the kind is 1 */
    /* +0x008 */ nw4r::math::VEC3 vec; /* `calcVecAngXY`'s input */
};

/* size: 0xB18 */
struct _ENEMY_WORK {
    /* +0x000 */ u8 active;             /* nonzero while the record is in use */
    /* +0x001 */ u8 unused_0x001;
    /* +0x002 */ u8 group;              /* the enemy group/entry index */
    /* +0x003 */ u8 team;
    /* +0x004 */ u8 field_0x004;        /* `em_work_die_ck` treats values below 2 as still alive */
    /* +0x005 */ u8 state;              /* the per-motion step the update functions switch on */
    /* +0x006 */ u8 state_0x006;        /* fn_80177D54's weapon sub-state */
    /* +0x007 */ u8 state_0x007;        /* fn_80177D54's second sub-state */
    /* +0x008 */ u8 field_0x008;
    /* +0x009 */ u8 field_0x009;        /* `fn_80170610` gates the position seat on it */
    /* +0x00A */ u8 field_0x00A;        /* the area `fn_8012E968` matches the work records'
                                        * `area_no` (+0x1E1) and the area table's entries against */
    /* +0x00B */ u8 field_0x00B;        /* the byte `fn_8012F504` hands the effect queue as its flag */
    /* +0x00C */ u8 unused_0x00C;
    /* +0x00D */ u8 field_0x00D;        /* the byte `fn_80137604`/`fn_8013760C` latch */
    /* +0x00E */ u8 field_0x00E;        /* `fn_80137C94` hands it to `fn_8012555C` */
    /* +0x00F */ u8 unused_0x00F[0x010 - 0x00F];
    /* +0x010 */ u8 field_0x010;       /* the value `fn_80140244`'s command reports */
    /* +0x011 */ u8 field_0x011;        /* the attack timer  arms */
    /* +0x012 */ u8 field_0x012;        /* the second counter `fn_8013791C`'s states run */
    /* +0x013 */ u8 unused_0x013[1];
    /* +0x014 */ u8 field_0x014;        /* the mode  maps to a two-state mask */
    /* +0x015 */ u8 unused_0x015[1];
    /* +0x016 */ u8 field_0x016;        /* the first counter `fn_8013791C`'s states run */
    /* +0x017 */ u8 state_0x017;        /* the state `fn_8013791C`'s second switch dispatches on */
    /* +0x018 */ s16 field_0x018;       /* `fn_80126844`'s joint/motion index */
    /* +0x01A */ u16 field_0x01A;       /* passed to `fn_80141B88` */
    /* +0x01C */ u8 unused_0x01C[0x020 - 0x01C];
    /* +0x020 */ s32 timer_0x020;       /* fn_8017799C runs this down */
    /* +0x024 */ u8 char_0x024[0x34];   /* the embedded `MHchar` base `fn_800E0914` takes (0x40 bytes
                                        * in this band's view; its +0x34 byte is named below) */
    /* +0x058 */ u8 model_flag_0x058;   /* set, the tail of `fn_8013791C` refreshes the scene model */
    /* +0x059 */ u8 unused_0x059[0x13C - 0x059];
    /* +0x13C */ u32 field_0x13C;       /* the scene-model id `fn_8007F0CC` is handed */
    /* +0x140 */ u8 unused_0x140[0x188 - 0x140];
    /* +0x188 */ nw4r::math::VEC3 pos;
    /* +0x194 */ u8 unused_0x194[0x1AC - 0x194];
    /* +0x1AC */ f32 field_0x1AC;       /* the height `fn_8012F39C` compares against 0.9 * the model scale */
    /* +0x1B0 */ u8 unused_0x1B0[0x1BC - 0x1B0];
    /* +0x1BC */ u32 field_0x1BC;       /* a rotation angle (wraps at 0x10000) */
    /* +0x1C0 */ u32 field_0x1C0;       /* the second rotation angle `fn_80133DB0` steps */
    /* +0x1C4 */ u32 field_0x1C4;       /* the third rotation angle */
    /* +0x1C8 */ u32 field_0x1C8;       /* bit 0x80 suppresses `fn_8012C6F4` */
    /* +0x1CC */ u8 unused_0x1CC[0x1D0 - 0x1CC];
    /* +0x1D0 */ f32 field_0x1D0;       /* the scale `fn_80142958` returns */
    /* +0x1D4 */ u8 unused_0x1D4[0x1DE - 0x1D4];
    /* +0x1DE */ u8 field_0x1DE;        /* `fn_8013791C`'s request-to-redraw byte */
    /* +0x1DF */ u8 unused_0x1DF[1];
    /* +0x1E0 */ u8 field_0x1E0;        /* passed to `fn_802B0668` (map lookup) */
    /* +0x1E1 */ u8 area_no;
    /* +0x1E2 */ u8 field_0x1E2;
    /* +0x1E3 */ u8 field_0x1E3;
    /* +0x1E4 */ u8 unused_0x1E4;
    /* +0x1E5 */ u8 action;             /* the action id `em_act_ck` matches */
    /* +0x1E6 */ u8 state_sub;          /* fn_80177BA4's dispatch index */
    /* +0x1E7 */ u8 field_0x1E7;        /* set to 1 once the action's entry block has run */
    /* +0x1E8 */ u8 field_0x1E8;
    /* +0x1E9 */ u8 field_0x1E9;
    /* +0x1EA */ u8 unused_0x1EA[0x1F5 - 0x1EA];
    /* +0x1F5 */ u8 field_0x1F5;
    /* +0x1F6 */ u8 unused_0x1F6[0x1F8 - 0x1F6];
    /* +0x1F8 */ u8 field_0x1F8;
    /* +0x1F9 */ u8 field_0x1F9;        /* nonzero, `fn_801400BC`'s command is skipped */
    /* +0x1FA */ u8 unused_0x1FA[0x1FB - 0x1FA];
    /* +0x1FB */ u8 field_0x1FB;        /* set by `fn_80147F48`'s case 9 (added by
                                        * `enemy/fn_80147CE0.cpp`) */
    /* +0x1FC */ u8 unused_0x1FC[0x1FD - 0x1FC];
    /* +0x1FD */ u8 field_0x1FD;        /* nonzero, `fn_8013FC60`'s command is skipped */
    /* +0x1FE */ u8 unused_0x1FE[0x218 - 0x1FE];
    /* +0x218 */ u32 field_0x218;       /* nonzero, `fn_80137EE0` has slots to average */
    /* +0x21C */ u8 unused_0x21C[0x244 - 0x21C];
    /* +0x244 */ EmMotionSlot slots_0x244[10];  /* the per-motion slot set `fn_80137EE0` averages */
    /* +0x30C */ u8 unused_0x30C[0x314 - 0x30C];
    /* +0x314 */ f32 field_0x314;      /* the effect scale `fn_80148BD0` writes (added by
                                        * `enemy/fn_80147CE0.cpp`) */
    /* +0x318 */ f32 field_0x318;      /* the effect radius `fn_801493A8` writes from `fn_80135644` */
    /* +0x31C */ u8 unused_0x31C[0x324 - 0x31C];
    /* +0x324 */ f32 field_0x324;      /* the effect scale `fn_801493A8` clamps */
    /* +0x328 */ s16 field_0x328;      /* the countdown `fn_80170804` ticks */
    /* +0x32A */ u8 field_0x32A;       /* the flag `fn_80170600`/`fn_80170758` clear */
    /* +0x32B */ u8 unused_0x32B[0x354 - 0x32B];
    /* +0x354 */ s16 field_0x354;      /* the 50-frame action counter `fn_801481D8` ticks and wraps */
    /* +0x356 */ u8 field_0x356;       /* the action's "hold" flag `fn_80147F48` arms/clears */
    /* +0x357 */ u8 field_0x357;       /* the action's case-6 flag `fn_80147F48` sets */
    /* +0x358 */ u8 field_0x358;       /* the action's "run" flag `fn_80147F48` arms/clears */
    /* +0x359 */ u8 unused_0x359[0x360 - 0x359];
    /* +0x360 */ s16 field_0x360;      /* fn_8013FD98's "already in this mode" countdown */
    /* +0x362 */ u8 unused_0x362[0x36C - 0x362];
    /* +0x36C */ nw4r::math::VEC3 vec_0x36C;  /* the target position `fn_80050F80` measures against
                                        * `pos` (`fn_8013F764`'s distance test) */
    /* +0x378 */ f32 value_0x378;       /* fn_80177F30/fn_8017801C clamp this against a pool float */
    /* +0x37C */ u8 unused_0x37C[0x382 - 0x37C];
    /* +0x382 */ u8 field_0x382;        /* `fn_8013FD1C` writes it from the stream */
    /* +0x383 */ u8 field_0x383;        /* set once `fn_801262BC`'s record is latched */
    /* +0x384 */ f32 field_0x384;       /* the record's +0x04 float, copied in with it */
    /* +0x388 */ u8 unused_0x388[0x38B - 0x388];
    /* +0x38B */ u8 field_0x38B;       /* `fn_80147F48`'s per-step flag (cleared on entry) */
    /* +0x38C */ u8 field_0x38C;        /* bit 0 gates `fn_8012C870` */
    /* +0x38D */ u8 field_0x38D;        /* `fn_80137C9C`'s action-end block clears it */
    /* +0x38E */ u8 field_0x38E;        /* `fn_80137C9C`'s action-end block clears it */
    /* +0x38F */ u8 field_0x38F;        /* the per-attacker bit set `fn_8012C9AC` masks against */
    /* +0x390 */ s32 values_0x390[5];   /* index 4 is `fn_8012C204` */
    /* +0x3A4 */ s32 values_0x3A4[37];  /* the chosen attacker's damage */
    /* +0x438 */ u8 unused_0x438[0x439 - 0x438];
    /* +0x439 */ u8 field_0x439;
    /* +0x43A */ u8 field_0x43A;
    /* +0x43B */ u8 field_0x43B;        /* the per-motion kind gate `fn_8012F474` tests against 2 */
    /* +0x43C */ u8 unused_0x43C;
    /* +0x43D */ u8 field_0x43D;
    /* +0x43E */ u8 field_0x43E;
    /* +0x43F */ u8 field_0x43F;        /* `fn_8013763C` sets it, `fn_80137648` reads it back */
    /* +0x440 */ u8 unused_0x440[0x464 - 0x440];
    /* +0x464 */ f32 field_0x464;      /* the motion parameter `fn_80149C58` hands to `fn_8012F7D4` */
    /* +0x468 */ u8 unused_0x468[0x46C - 0x468];
    /* +0x46C */ u8 field_0x46C;       /* the area/entry byte `fn_80170804` latches to 0xFF */
    /* +0x46D */ u8 unused_0x46D[0x784 - 0x46D];
    /* +0x784 */ u8 field_0x784;
    /* +0x785 */ u8 unused_0x785[0x794 - 0x785];
    /* +0x794 */ u16 field_0x794;
    /* +0x796 */ s16 field_0x796;
    /* +0x798 */ u8 field_0x798;
    /* +0x799 */ u8 field_0x799;
    /* +0x79A */ u8 field_0x79A;
    /* +0x79B */ u8 field_0x79B;
    /* +0x79C */ u8 unused_0x79C[0x7A0 - 0x79C];
    /* +0x7A0 */ u32 field_0x7A0;       /* the numerator of the ratio `fn_8014001C` reports */
    /* +0x7A4 */ u32 field_0x7A4;       /* its denominator */
    /* +0x7A8 */ u8 unused_0x7A8[0x7AC - 0x7A8];
    /* +0x7AC */ u32 field_0x7AC;       /* the second frame counter `fn_8012EE80` measures against `field_0x7A0` */
    /* +0x7B0 */ f32 field_0x7B0;       /* the `team == 0xf` threshold `fn_8012EFDC` tests against 0.0 */
    /* +0x7B4 */ u8 unused_0x7B4[0x7BC - 0x7B4];
    /* +0x7BC */ f32 field_0x7BC;       /* the effect radius `fn_8012F474` scales */
    /* +0x7C0 */ f32 field_0x7C0;       /* its second factor */
    /* +0x7C4 */ f32 field_0x7C4;       /* the value `fn_8012F504` clears to 1.0f */
    /* +0x7C8 */ u8 field_0x7C8;        /* the action id `fn_8014278C` picks */
    /* +0x7C9 */ u8 unused_0x7C9[0x812 - 0x7C9];
    /* +0x812 */ u16 field_0x812;       /* the action-end block clears it */
    /* +0x814 */ u16 field_0x814;       /* the action-end block clears it */
    /* +0x816 */ u8 unused_0x816[0x818 - 0x816];
    /* +0x818 */ s16 field_0x818;       /* the motion timer `fn_8012F110` arms before its mode switch */
    /* +0x81A */ u8 unused_0x81A[0x89F - 0x81A];
    /* +0x89F */ u8 field_0x89F;        /* the mode `fn_80137720` latches */
    /* +0x8A0 */ u8 unused_0x8A0[0x8A4 - 0x8A0];
    /* +0x8A4 */ s16 field_0x8A4;       /* the timer `fn_80137720` arms from `fn_80126494` */
    /* +0x8A6 */ s16 field_0x8A6;       /* the timer `fn_801409C8` compares against the enemy data's */
    /* +0x8A8 */ u8 unused_0x8A8[0x8AA - 0x8A8];
    /* +0x8AA */ u8 mode_0x8AA;         /* `fn_80130A10` latches it (0/1); `fn_8012EC60` tests it
                                        * against 1 */
    /* +0x8AB */ u8 unused_0x8AB[0x8B3 - 0x8AB];
    /* +0x8B3 */ u8 flags_0x8B3;        /* the action bitmap `fn_801376BC`/`DC`/`04` mask */
    /* +0x8B4 */ u8 unused_0x8B4[0x8C8 - 0x8B4];
    /* +0x8C8 */ u32 field_0x8C8;       /* `fn_801260BC`'s value `fn_80137C20` latches */
    /* +0x8CC */ u32 field_0x8CC;       /* `fn_801260E0`'s value `fn_80137C20` latches */
    /* +0x8D0 */ u8 unused_0x8D0[0x8D3 - 0x8D0];
    /* +0x8D3 */ u8 field_0x8D3;        /* `fn_80137614` matches it against 1 */
    /* +0x8D4 */ u8 unused_0x8D4[0x916 - 0x8D4];
    /* +0x916 */ s16 field_0x916;
    /* +0x918 */ u8 unused_0x918[0x938 - 0x918];
    /* +0x938 */ s16 field_0x938;       /* the motion timer `fn_8012F1D8` gates its window test on */
    /* +0x93A */ u8 unused_0x93A[0x94A - 0x93A];
    /* +0x94A */ u16 field_0x94A;       /* the action-end block clears it */
    /* +0x94C */ u16 field_0x94C;       /* the action-end block clears it */
    /* +0x94E */ u8 unused_0x94E[0x954 - 0x94E];
    /* +0x954 */ s32** field_0x954;
    /* +0x958 */ s32 field_0x958;
    /* +0x95C */ u8 field_0x95C;
    /* +0x95D */ u8 field_0x95D;
    /* +0x95E */ u8 unused_0x95E[0x961 - 0x95E];
    /* +0x961 */ u8 stack_0x961[0x0F];  /* the interpreter's byte stack: `fn_8013FEF4` writes
                                        * `stack_0x961[arg[0]]`, index 1 is the depth `fn_801408B4`
                                        * pops through (`stack_0x961[2 + depth]` is the byte it
                                        * restores, `values_0x970[depth]` the word) */
    /* +0x970 */ s32 values_0x970[11];  /* the words belonging to `stack_0x961` */
    /* +0x99C */ EmCmdRec rec_0x99C;
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
    /* +0x9F0 */ u8 unused_0x9F0[0x9F7 - 0x9F0];
    /* +0x9F7 */ u8 field_0x9F7;        /* matched against `area_no` by `fn_8013FF5C` */
    /* +0x9F8 */ u8 field_0x9F8;        /* the value `fn_8013FD28`'s command reports */
    /* +0x9F9 */ u8 unused_0x9F9[0x9FC - 0x9F9];
    /* +0x9FC */ u8 field_0x9FC;        /* the program index `fn_8013F764`/`fn_8013F994` step */
    /* +0x9FD */ u8 field_0x9FD;        /* bit 0 walks the program table backwards */
    /* +0x9FE */ u8 field_0x9FE;        /* the run's state: 0 = not started, 1/2 = the distance test */
    /* +0x9FF */ u8 field_0x9FF;        /* set by every `fn_8013F764` call */
    /* +0xA00 */ EmProgTbl* prog_0xA00; /* the enemy's program table (NULL → nothing to do) */
    /* +0x0A04 */ u8 flags_0xA04;       /* bit 0 stun, bit 1 sleep, bit 2 the sleep result */
    /* +0x0A05 */ u8 field_0xA05;
    /* +0x0A06 */ u8 field_0xA06;
    /* +0x0A07 */ u8 field_0xA07;
    /* +0x0A08 */ u8 unused_0xA08[0xA0D - 0xA08];
    /* +0x0A0D */ u8 field_0xA0D;       /* fn_80177D54's per-frame gate */
    /* +0x0A0E */ u8 unused_0xA0E[0xA69 - 0xA0E];
    /* +0x0A69 */ u8 field_0xA69;       /* fn_80177D54's second gate */
    /* +0x0A6A */ u8 unused_0xA6A[0xAEE - 0xA6A];
    /* +0xAEE */ u8 field_0xAEE;        /* `fn_8013791C`'s one-shot action-setup request */
    /* +0xAEF */ u8 unused_0xAEF[0xB18 - 0xAEF];
};

#endif /* MHTRI_ENEMY_ENEMY_WORK_H */
