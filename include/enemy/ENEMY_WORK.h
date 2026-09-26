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

/* The sound-work record (`include/sound/se.h`) whose handle the `enemy` band's action steps hand
 * `se_req_pos_ps`/`shell_se_req`; forward-declared so this header stays light (rule 4's 0xB14). */
struct _se_w;

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
    /* +0x001 */ u8 unused_0x001[0x008 - 0x001];
    /* +0x008 */ u8 field_0x008;      /* the kind `fn_8012D1A8` classifies (added by
                                      * `enemy/fn_801B4458.cpp`'s `fn_801B47A4`) */
    /* +0x009 */ u8 unused_0x009[0x016 - 0x009];
    /* +0x016 */ u8 area_no;
    /* +0x017 */ u8 unused_0x017[0x03C - 0x017];
    /* +0x03C */ nw4r::math::VEC3 vec_0x3C; /* the position `fn_801B45B0` measures the seat
                                      * records against (added by `enemy/fn_801B4458.cpp`) */
    /* +0x048 */ u8 unused_0x048[0xB20 - 0x048];
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

struct EmPartState {
    /* +0x000 (+0x740) */ u8 field_0x740;  /* the part flag `fn_801823A0` writes from its argument
                                          * and `fn_801823C0` sets */
    /* +0x001 (+0x741) */ u8 pad_0x741;
    /* +0x002 (+0x742) */ s16 field_0x742; /* the six per-part counters `fn_801823C0` clears */
    /* +0x004 (+0x744) */ s16 field_0x744;
    /* +0x006 (+0x746) */ s16 field_0x746;
    /* +0x008 (+0x748) */ s16 field_0x748;
    /* +0x00A (+0x74A) */ s16 field_0x74A;
    /* +0x00C (+0x74C) */ s16 field_0x74C;
};

/* The 0xC-byte colour/step block at +0x328 this range's MHchar material steppers read as one float
 * followed by the K-colour bytes (`fn_80181CC0` writes them, `fn_80181E24` reads them back).  It is
 * the same bytes the header's other +0x328 views spell as the aim vector / handle set / action block.
 * size: 0x0C */
struct EmColorBlock {
    /* +0x000 (+0x328) */ f32 field_0x328; /* the scalar `fn_80181CC0` steps toward the mode target */
    /* +0x004 (+0x32C) */ u8 field_0x32C;  /* GX K-colour red   (`fn_80181CC0`/`fn_80181E24`) */
    /* +0x005 (+0x32D) */ u8 field_0x32D;  /* its green */
    /* +0x006 (+0x32E) */ u8 field_0x32E;  /* its blue */
    /* +0x007 (+0x32F) */ u8 field_0x32F;  /* the aim/approach byte `fn_801825A4` kind 2 returns
                                          * and `fn_801820DC` gates on (== 0) */
    /* +0x008 (+0x330) */ u8 field_0x330;  /* the "special part armed" byte `fn_801820DC` tests and
                                          * `fn_801825A4` kind 6 returns as != 0 */
    /* +0x009 (+0x331) */ u8 pad_0x331;
    /* +0x00A (+0x332) */ s16 field_0x332; /* the part-3 timer `fn_801825A4` kind 4 compares
                                          * against 900 */
};

/* The 0x2C-byte block this band's `fn_801D71C4` (`enemy/fn_801D428C.cpp`) clears at +0x328: two
 * vectors, the float after them and the two flags its spawn code sets.  The bytes are the ones
 * `enemy/fn_80170600.cpp` names `field_0x328`/`field_0x32A` on the other side of the union - the
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
    /* +0x00C */ u8 field_0x00C;        /* the mode `fn_80166DF8` gates its case-0/case-3 blocks on
                                        * (3 = the "entry seated" state) */
    /* +0x00D */ u8 field_0x00D;        /* the byte `fn_80137604`/`fn_8013760C` latch */
    /* +0x00E */ u8 field_0x00E;        /* `fn_80137C94` hands it to `fn_8012555C` */
    /* +0x00F */ u8 field_0x00F;        /* `enemy/fn_801B4458.cpp`'s `fn_801B4D14` gates its
                                        * seat re-test on it being zero (added by that unit) */
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
    /* +0x1B0 */ nw4r::math::VEC3 aim;   /* the aim point `fn_801D75D0` copies the target
                                         * record's position into (added by `enemy/fn_801D428C.cpp`) */
    /* +0x1BC */ u32 field_0x1BC;       /* a rotation angle (wraps at 0x10000) */
    /* +0x1C0 */ u32 field_0x1C0;       /* the second rotation angle `fn_80133DB0` steps */
    /* +0x1C4 */ u32 field_0x1C4;       /* the third rotation angle */
    /* +0x1C8 */ u32 field_0x1C8;       /* bit 0x80 suppresses `fn_8012C6F4` */
    /* +0x1CC */ f32 field_0x1CC;       /* the seat-preference weight `enemy/fn_801B4458.cpp`'s
                                        * `fn_801B4D14` scales by 0.85/1.3/1.1 for the seat mode
                                        * at +0x0A (added by that unit) */
    /* +0x1D0 */ f32 field_0x1D0;       /* the scale `fn_80142958` returns */
    /* +0x1D4 */ f32 field_0x1D4;        /* the alpha ratio `fn_801A9210` scales by 255 */
    /* +0x1D8 */ u8 unused_0x1D8[0x1DE - 0x1D8];
    /* +0x1DE */ u8 field_0x1DE;        /* `fn_8013791C`'s request-to-redraw byte */
    /* +0x1DF */ u8 unused_0x1DF[1];
    /* +0x1E0 */ u8 field_0x1E0;        /* passed to `fn_802B0668` (map lookup) */
    /* +0x1E1 */ u8 area_no;
    /* +0x1E2 */ u8 field_0x1E2;
    /* +0x1E3 */ u8 field_0x1E3;
    /* +0x1E4 */ u8 field_0x1E4;  /* `enemy/fn_801993E0.cpp`'s `fn_8019E908`: bit 0 is the * "armed" flag and the byte counts down while it is set * (`lbz`/`clrlwi ...,31` guard, then `addi -1`).  Both views agree on bit 0; bit 0 is the `fn_801A9384` kind-0 flag */
    /* +0x1E5 */ u8 action;             /* the action id `em_act_ck` matches */
    /* +0x1E6 */ u8 state_sub;          /* fn_80177BA4's dispatch index */
    /* +0x1E7 */ u8 field_0x1E7;        /* set to 1 once the action's entry block has run */
    /* +0x1E8 */ u8 field_0x1E8;
    /* +0x1E9 */ u8 field_0x1E9;
    /* +0x1EA */ u8 unused_0x1EA[0x1EC - 0x1EA];
    /* +0x1EC */ u16 bits_0x1EC;      /* the 5-bit value `enemy/fn_80182D5C.cpp`'s `fn_8018493C`
                                        * reads as `lhz` + `clrlwi ...,27` to derive its 0x96/0x5A timer
                                        * (added by that unit) */
    /* +0x1EE */ u8 unused_0x1EE[0x1F5 - 0x1EE];
    /* +0x1F5 */ u8 field_0x1F5;
    /* +0x1F6 */ u8 unused_0x1F6[0x1F8 - 0x1F6];
    /* +0x1F8 */ u8 field_0x1F8;
    /* +0x1F9 */ u8 field_0x1F9;        /* nonzero, `fn_801400BC`'s command is skipped */
    /* +0x1FA */ u8 unused_0x1FA[0x1FB - 0x1FA];
    /* +0x1FB */ u8 field_0x1FB;        /* set by `fn_80147F48`'s case 9 (added by
                                        * `enemy/fn_80147CE0.cpp`) */
    /* +0x1FC */ u8 field_0x1FC;        /* the action-record / "special part" request flag: both
                                        * `enemy/fn_801502C8.cpp`'s `fn_801542D0` and
                                        * `enemy/fn_801DB8E0.cpp`'s `fn_801DF8EC` arm it with
                                        * +0x1FE/+0x1FF */
    /* +0x1FD */ u8 field_0x1FD;        /* nonzero, `fn_8013FC60`'s command is skipped */
    /* +0x1FE */ u8 field_0x1FE;        /* the mode/timer byte `fn_801542D0` arms (0x0A/0x1E) and
                                        * `fn_801DF8EC` writes (0x0A), beside +0x1FC/+0x1FF */
    /* +0x1FF */ u8 field_0x1FF;        /* the id `fn_801542D0`/`fn_801DF8EC` arm beside
                                        * +0x1FC/+0x1FE */
    /* +0x200 */ u8 unused_0x200[0x20C - 0x200];
    /* +0x20C */ f32 field_0x20C;        /* the effect spawn height `fn_801D4DD8`/`fn_801D428C`
                                         * add to the work record's y, and the height
                                         * `fn_801A4504` compares its joint samples against */
    /* +0x210 */ f32 field_0x210;       /* the second effect spawn height `enemy/fn_801DB8E0.cpp`'s
                                        * `fn_801DB978` adds to the work record's y (the +0x20C
                                        * sibling is the other branch's) */
    /* +0x214 */ u8 unused_0x214[0x218 - 0x214];
    /* +0x218 */ u32 field_0x218;       /* nonzero, `fn_80137EE0` has slots to average */
    /* +0x21C */ u8 unused_0x21C[0x228 - 0x21C];
    /* +0x228 */ u16 field_0x228;      /* `enemy/fn_801993E0.cpp`'s `fn_8019D8B8` case 4: the low
                                        * two bits are the "blocked" gate (`lhz` + `clrlwi ...,30`). */
    /* +0x22A */ u8 unused_0x22A[0x23C - 0x22A];
    /* +0x23C */ u8 field_0x23C;       /* the action-ready byte `enemy/fn_801CA004.cpp`'s
                                        * `fn_801CA40C` case 0 reports (added by that unit) */
    /* +0x23D */ u8 unused_0x23D[0x244 - 0x23D];
    /* +0x244 */ EmMotionSlot slots_0x244[10];  /* the per-motion slot set `fn_80137EE0` averages */
    union {
        struct {
            /* +0x30C */ u8 unused_0x30C[0x314 - 0x30C];
            /* +0x314 */ f32 field_0x314;  /* the effect scale `fn_80148BD0` writes (added by
                                            * `enemy/fn_80147CE0.cpp`) */
            /* +0x318 */ f32 field_0x318;  /* the effect radius `fn_801493A8` writes from `fn_80135644` */
        };
        /* the same bytes as the offset vector `enemy/fn_801CA004.cpp`'s `fn_801CC5DC` rotates and
         * hands the position helpers (added by that unit; `vec_0x310.y`/`.z` are the two floats
         * above). */
        struct {
            /* +0x30C */ u8 unused_0x30Cb[0x310 - 0x30C];
            /* +0x310 */ nw4r::math::VEC3 vec_0x310;
        } offset_0x30C;
    };
    union {
        struct {
            /* +0x31C */ u8 unused_0x31C[0x320 - 0x31C];
            /* +0x320 */ f32 field_0x320;  /* the third effect scale `fn_80167770` seeds (added by
                                            * `enemy/fn_80165FC8.cpp`; the byte range is the +0x31C
                                            * VEC3 clash that unit's header records) */
            /* +0x324 */ f32 field_0x324;  /* the effect scale `fn_801493A8` clamps */
        };
        /* the second offset vector `enemy/fn_801D80EC.cpp`'s `fn_801D83C0` rotates (`rotVecY` on
         * +0x31C; `.y`/`.z` are the two floats above, so the two views share the bytes). */
        /* +0x31C */ nw4r::math::VEC3 vec_0x31C;
    };
    /* +0x328 */ union {
        /* the countdown/flag view `fn_80170600`/`fn_80170804` use. */
        struct {
            /* +0x328 */ s16 field_0x328;  /* the countdown `fn_80170804` ticks */
            /* +0x32A */ u8 field_0x32A;   /* the flag `fn_80170600`/`fn_80170758` clear */
            /* +0x32B */ u8 unused_0x32B[0x338 - 0x32B];
            /* +0x338 */ u8 field_0x338[4]; /* `fn_801A4504`: per-slot data-table index, 0xFF = none */
        };
        /* +0x328 */ s32 handles_0x328[4]; /* `fn_801A4504`: per-slot joint-effect handles, -1 = empty */
        /* `enemy/fn_801993E0.cpp` walks the same bytes as a four-slot handle/state set: `fn_8019ECD4`
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
        /* the signed-short TEV view `enemy/fn_8018B3B8.cpp`'s `fn_80191038` steps: the same
         * +0x350/+0x352 bytes as the flat view above, read as `lha`/`sth` s16 colour words (the
         * +0x350 halfword is stepped by 2 and clamped, the +0x352 one set from it).  Added by that
         * unit; a union member because the byte view above already owns those bytes. */
        struct {
            /* +0x328 */ u8 unused_0x328e[0x350 - 0x328];
            /* +0x350 */ s16 tev_0x350;
            /* +0x352 */ s16 tev_0x352;
        };
        /* the float/rotation view `enemy/fn_801502C8.cpp`'s `fn_80154988` steps (the same bytes
         * as the flat s32 view above, read as the animation angles the effect rotates by). */
        struct {
            /* +0x328 */ u8 unused_0x328d[0x08];
            /* +0x330 */ s32 field_0x330;
            /* +0x334 */ u8 unused_0x334d[0x08];
            /* +0x33C */ f32 field_0x33C;
            /* +0x340 */ f32 field_0x340;
            /* +0x344 */ u8 unused_0x344d[0x04];
            /* +0x348 */ f32 field_0x348;
            /* +0x34C */ f32 field_0x34C;
        } rot_0x328;
        /* the byte view `enemy/fn_801502C8.cpp`'s `fn_80154E40` resets (the free-slot states of
         * the effect slot set). */
        struct {
            /* +0x328 */ u8 slots_0x328[0x10];
            /* +0x338 */ s16 field_0x338;
            /* +0x33A */ u8 unused_0x33Ad[0x02];
            /* +0x33C */ s16 field_0x33C;
        } init_0x328;
        /* the action block `enemy/fn_801D428C.cpp` clears (`fn_801D71C4`). */
        struct EmActionBlock action_0x328;
        /* the signed-short countdown view the action band `enemy/fn_8015D860.cpp` uses: the same
         * 0x32A bytes as one halfword, where the first view above reads a `u8` flag there
         * (`fn_8015DB68` stores 0x384 into it and `fn_8015DD6C` `lha`s it down while positive).
         * Two views of the same bytes, so a union member and not a re-typing. */
        struct {
            /* +0x328 */ u8 unused_0x328d[0x2];
            /* +0x32A */ s16 timer_0x32A;  /* armed to 0x384 at action 1's sub-state 6 */
            /* +0x32C */ u8 unused_0x32Cc[0x338 - 0x32C];
        };
        /* the float/word/short-timer view `enemy/fn_801CA004.cpp`'s action band writes and reads
         * (`fn_801CAA8C` clears 0x328/0x32C, `fn_801CA40C` reads the three 0x330/0x332/0x334
         * halfwords; the 0x330 byte flag `armed_0x328.field_0x330` above is the same byte). */
        struct {
            /* +0x328 */ f32 field_0x328;
            /* +0x32C */ s32 field_0x32C;
            /* +0x330 */ s16 field_0x330;
            /* +0x332 */ s16 field_0x332;
            /* +0x334 */ s16 field_0x334;
            /* +0x336 */ u8 unused_0x336j[0x354 - 0x336];
        } timer_0x328;
        /* the effect-seat view `enemy/fn_801B4458.cpp` keeps: the selected seat record index at
         * +0x328 (0xFF = none) and its armed flag at +0x329 (added by that unit). */
        struct {
            /* +0x328 */ u8 slot_0x328;
            /* +0x329 */ u8 flag_0x329;
            /* +0x32A */ u8 unused_0x32Af[0x354 - 0x32A];
        };
        /* this range's colour/step view (`fn_80181CC0`/`fn_80181E24`/`fn_801820DC`/`fn_801825A4`). */
        struct EmColorBlock color_0x328;
    };
    /* +0x354 */ union {
        /* the s16 view `fn_801481D8` counts and the byte view `fn_80147F48` flags - the same four
         * bytes, in the two spellings the reconstructed units use (rule 1: one home). */
        struct {
            /* +0x0 */ s16 field_0x354;   /* the 50-frame action counter `fn_801481D8` ticks and wraps */
            /* +0x2 */ u8 field_0x356;    /* the action's "hold" flag `fn_80147F48` arms/clears */
            /* +0x3 */ u8 field_0x357;    /* the action's case-6 flag `fn_80147F48` sets */
        };
        /* the 32-bit action bit set `enemy/fn_801993E0.cpp` clears (`fn_8019E948`), sets
         * (`fn_8019E960`) and tests (`fn_8019E9AC`): `slot = id / 8` clamped to 3,
         * `mask_0x354[slot] |= 1 << (id % 8)`. */
        u8 mask_0x354[4];
        /* the s16 TEV pair view `enemy/fn_8018B3B8.cpp`'s `fn_80191038` steps (`lha`/`sth` at
         * +0x354/+0x356, the same bytes as `field_0x354`/`field_0x356` above).  Added by that unit. */
        struct {
            /* +0x0 */ s16 tev_0x354;
            /* +0x2 */ s16 tev_0x356;
        };
    };
    /* +0x358 */ union {
        struct {
            /* +0x358 */ u8 field_0x358;       /* the action's "run" flag `fn_80147F48` arms/clears */
            /* +0x359 */ u8 field_0x359;       /* `enemy/fn_801993E0.cpp`'s `fn_801994F4` sets it with
                                                * +0x35B at the two state-0/1 action starts. */
            /* +0x35A */ u8 unused_0x35A;
            /* +0x35B */ u8 field_0x35B;       /* the same action-start pair as +0x359. */
            /* +0x35C */ u8 field_0x35C;       /* the two action-completion flags `fn_8019E398`
                                                * sets/clears (bit 0) and bit 1, and `fn_8019D8B8`
                                                * cases 1/2 test. */
            /* +0x35D */ u8 unused_0x35D[0x360 - 0x35D];
        };
        /* the signed-short TEV view `enemy/fn_8018B3B8.cpp`'s `fn_80191038`/`fn_801913FC` step
         * (`lha`/`sth` at +0x358/+0x35A/+0x35C, the same bytes as the byte view above; +0x35E
         * is the shadow flag the same unit arms).  Added by that unit. */
        struct {
            /* +0x358 */ s16 tev_0x358;
            /* +0x35A */ s16 tev_0x35A;
            /* +0x35C */ s16 tev_0x35C;
            /* +0x35E */ u8 tev_0x35E;
            /* +0x35F */ u8 unused_0x35F;
        };
    };
    /* +0x360 */ s16 field_0x360;      /* fn_8013FD98's "already in this mode" countdown */
    /* +0x362 */ s16 field_0x362;      /* `enemy/fn_801993E0.cpp`'s `fn_8019D8B8` case 7: `<= 0`
                                        * answers 1 (`lha` + `cmpwi 0` + `bgt`). */
    /* +0x364 */ u8 unused_0x364[0x36C - 0x364];
    /* +0x36C */ nw4r::math::VEC3 vec_0x36C;  /* the target position `fn_80050F80` measures against
                                        * `pos` (`fn_8013F764`'s distance test) */
    /* +0x378 */ f32 value_0x378;       /* fn_80177F30/fn_8017801C clamp this against a pool float */
    /* +0x37C */ u32 field_0x37C;       /* the rotation word `fn_801CEF44` latches from
                                        * `field_0x1C0` and hands `fn_801354F4` (added by
                                        * `enemy/fn_801CCBC4.cpp`) */
    /* +0x380 */ u8 field_0x380;        /* the "special part armed" selector `fn_801D6758` matches
                                         * against 1 (added by `enemy/fn_801D428C.cpp`) */
    /* +0x381 */ u8 state_0x381;        /* its record index, handed to `fn_801377D0` */
    /* +0x382 */ u8 field_0x382;        /* `fn_8013FD1C` writes it from the stream; 0xFF means
                                         * "no special part", which `fn_801D6758` rejects */
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
    /* +0x440 */ s16 field_0x440;    /* the action frame counter `enemy/fn_801B4458.cpp`'s
                                        * `fn_801B47A4` tests against 0x1C2 (added by that unit) */
    /* +0x442 */ u8 unused_0x442[0x450 - 0x442];
    /* +0x450 */ s16 field_0x450;       /* the first of the two `lha` gates `enemy/fn_801DB8E0.cpp`'s
                                        * `fn_801DF8EC` compares (+0x452 is its sibling) */
    /* +0x452 */ s16 value_0x452;       /* the 0x384-frame gate `fn_801D6DA4` tests (added by
                                         * `enemy/fn_801D428C.cpp`) */
    /* +0x454 */ u8 unused_0x454[0x464 - 0x454];
    /* +0x464 */ f32 field_0x464;      /* the motion parameter `fn_80149C58` hands to `fn_8012F7D4` */
    /* +0x468 */ u8 unused_0x468[0x46C - 0x468];
    /* +0x46C */ u8 field_0x46C;       /* the area/entry byte `fn_80170804` latches to 0xFF */
    /* +0x46D */ u8 unused_0x46D[0x482 - 0x46D];
    /* +0x482 */ u8 field_0x482;       /* nonzero picks the second approach float in
                                        * `enemy/fn_80182D5C.cpp`'s `fn_80184CE0` (added by that unit;
                                        * the pre-header `include/enemy.h` view of this byte) */
    /* +0x483 */ u8 unused_0x483[0x48E - 0x483];
    /* +0x48E */ u8 field_0x48E;        /* the part-kind byte `enemy/fn_801DB8E0.cpp`'s `fn_801DF2F8`
                                        * compares against 4 and 6 */
    /* +0x48F */ u8 field_0x48F;      /* the part-damage special-case byte `fn_80181E24` tests
                                        * against 1 before painting the 0x1E4 colour white (added by
                                        * `enemy/fn_80181C88.cpp`, which re-types the base filler) */
    /* +0x490 */ u8 unused_0x490[0x491 - 0x490];
    /* +0x491 */ u8 field_0x491;      /* `enemy/fn_8015D860.cpp`'s `fn_8015DB68` sets it at action
                                      * 0xA's sub-state 0xC8 */
    /* +0x492 */ u8 unused_0x492[0x608 - 0x492];
    /* +0x608 */ u32 field_0x608;      /* the X rotation word `fn_801661FC` hands `rotMatrixX`
                                        * (added by `enemy/fn_80165FC8.cpp`) */
    /* +0x60C */ u8 unused_0x60C[0x610 - 0x60C];
    /* +0x610 */ u32 field_0x610;      /* the Z rotation word the same function hands `rotMatrixZ` */
    /* +0x614 */ u8 unused_0x614[0x740 - 0x614];
    /* +0x740 */ struct EmPartState part_0x740; /* the per-part flags/counters `fn_801823A0`/
                                        * `fn_801823C0` write (added by `enemy/fn_80181C88.cpp`) */
    /* +0x74E */ u8 unused_0x74E[0x761 - 0x74E];
    /* +0x761 */ u8 field_0x761;       /* one byte, two bands: `enemy/fn_801993E0.cpp`'s slot bit
                                        * map (`fn_8019EA04` clears it, `fn_8019EA80` scans its low
                                        * 8 bits for free slot indices) and `enemy/fn_801D428C.cpp`'s
                                        * `fn_801D4F78`, whose bit 0 is the "aim target live" flag it
                                        * mirrors from `fn_8012EC60`/the area test and bit 1 its
                                        * one-shot latch. */
    /* +0x762 */ u8 field_0x762;       /* the latch `fn_801D4F78` checks before setting bit 1
                                        * (added by `enemy/fn_801D428C.cpp`) */
    /* +0x763 */ u8 unused_0x763[0x76C - 0x763];
    /* +0x76C */ nw4r::math::VEC3 vec_0x76C; /* the position `calcVecAngX` is handed in
                                         * `fn_801D4CE0` (added by `enemy/fn_801D428C.cpp`) */
    /* +0x778 */ u8 unused_0x778[0x784 - 0x778];
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
    /* +0x81A */ s16 field_0x81A;     /* the motion timer `fn_80182320` gates its arming on
                                        * (<= 0); added by `enemy/fn_80181C88.cpp`, which re-types the
                                        * base filler's first two bytes */
    /* +0x81C */ u8 unused_0x81C[0x833 - 0x81C];
    /* +0x833 */ u8 field_0x833;       /* the "approach armed" byte `enemy/fn_801B4458.cpp`'s
                                        * `fn_801B47A4` branches on (added by that unit) */
    /* +0x834 */ u8 field_0x834;      /* the "already seated" byte `fn_801671AC` tests against 1
                                        * (added by `enemy/fn_80165FC8.cpp`) */
    /* +0x835 */ u8 field_0x835;      /* the seat flag `fn_80166DF8` sets */
    /* +0x836 */ u16 flags_0x836;       /* bit 0x8000 is the "aim target found" flag `fn_801D75D0`
                                         * mirrors `self->action_0x328.field_0x345` into (added by
                                         * `enemy/fn_801D428C.cpp`) */
    /* +0x838 */ u8 unused_0x838[0x888 - 0x838];
    /* +0x888 */ s32 field_0x888;       /* the effect handle `enemy/fn_801B4458.cpp`'s `fn_801B4E3C`
                                        * releases through `fn_803B9994` and clears to -1; `fn_801B6C38`
                                        * tests it against -1 (added by that unit) */
    /* +0x88C */ u8 unused_0x88C[0x89F - 0x88C];
    /* +0x89F */ u8 field_0x89F;        /* the mode `fn_80137720` latches */
    /* +0x8A0 */ u8 unused_0x8A0[0x8A2 - 0x8A0];
    /* +0x8A2 */ u16 field_0x8A2;       /* the counter `fn_801CD400` gates its target search on
                                        * (`lhz` + `cmplwi 0xFA`); added by `enemy/fn_801CCBC4.cpp` */
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
    /* +0x9F0 */ u8 unused_0x9F0[0x9F6 - 0x9F0];
    /* +0x9F6 */ u8 field_0x9F6;       /* one state byte, two readers: `enemy/fn_801993E0.cpp`'s
                                        * `fn_8019DAC0` (area 3 arms the done state only when it
                                        * reads 2) and `enemy/fn_801D428C.cpp`'s `fn_801D6D24`
                                        * (matches it against 7 before arming the +0x330 flag). */
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
    /* +0x0A6A */ u8 unused_0xA6A[0xAEA - 0xA6A];
    /* +0x0AEA */ u16 field_0xAEA;      /* the eighteen-bit flag word `enemy/fn_801D80EC.cpp`'s
                                        * effect calls hand the shell callback (added by that
                                        * unit) */
    /* +0xAEC */ u8 unused_0xAEC[0xAEE - 0xAEC];
    /* +0xAEE */ u8 field_0xAEE;        /* `fn_8013791C`'s one-shot action-setup request */
    /* +0xAEF */ u8 unused_0xAEF[0xB14 - 0xAEF];
    /* +0xB14 */ struct _se_w* se_0xB14;  /* the sound-work handle `enemy/fn_801BD6C0.cpp`'s
                                           * `se_req_pos_ps`/`shell_se_req` calls take */
};

#endif /* MHTRI_ENEMY_ENEMY_WORK_H */
