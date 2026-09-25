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
    /* +0x00A */ u8 field_0x00A;
    /* +0x00B */ u8 unused_0x00B[2];
    /* +0x00D */ u8 field_0x00D;        /* the byte `fn_80137604`/`fn_8013760C` latch */
    /* +0x00E */ u8 field_0x00E;        /* `fn_80137C94` hands it to `fn_8012555C` */
    /* +0x00F */ u8 unused_0x00F[0x011 - 0x00F];
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
    /* +0x194 */ u8 unused_0x194[0x1BC - 0x194];
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
    /* +0x1F9 */ u8 unused_0x1F9[0x218 - 0x1F9];
    /* +0x218 */ u32 field_0x218;       /* nonzero, `fn_80137EE0` has slots to average */
    /* +0x21C */ u8 unused_0x21C[0x244 - 0x21C];
    /* +0x244 */ EmMotionSlot slots_0x244[10];  /* the per-motion slot set `fn_80137EE0` averages */
    /* +0x30C */ u8 unused_0x30C[0x328 - 0x30C];
    /* +0x328 */ s16 field_0x328;      /* the countdown `fn_80170804` ticks */
    /* +0x32A */ u8 field_0x32A;       /* the flag `fn_80170600`/`fn_80170758` clear */
    /* +0x32B */ u8 unused_0x32B[0x378 - 0x32B];
    /* +0x378 */ f32 value_0x378;       /* fn_80177F30/fn_8017801C clamp this against a pool float */
    /* +0x37C */ u8 unused_0x37C[0x38C - 0x37C];
    /* +0x38C */ u8 field_0x38C;        /* bit 0 gates `fn_8012C870` */
    /* +0x38D */ u8 field_0x38D;        /* `fn_80137C9C`'s action-end block clears it */
    /* +0x38E */ u8 field_0x38E;        /* `fn_80137C9C`'s action-end block clears it */
    /* +0x38F */ u8 field_0x38F;        /* the per-attacker bit set `fn_8012C9AC` masks against */
    /* +0x390 */ s32 values_0x390[5];   /* index 4 is `fn_8012C204` */
    /* +0x3A4 */ s32 values_0x3A4[37];  /* the chosen attacker's damage */
    /* +0x438 */ u8 unused_0x438[0x439 - 0x438];
    /* +0x439 */ u8 field_0x439;
    /* +0x43A */ u8 field_0x43A;
    /* +0x43B */ u8 unused_0x43B[2];
    /* +0x43D */ u8 field_0x43D;
    /* +0x43E */ u8 field_0x43E;
    /* +0x43F */ u8 field_0x43F;        /* `fn_8013763C` sets it, `fn_80137648` reads it back */
    /* +0x440 */ u8 unused_0x440[0x46C - 0x440];
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
    /* +0x79C */ u8 unused_0x79C[0x7C8 - 0x79C];
    /* +0x7C8 */ u8 field_0x7C8;        /* the action id `fn_8014278C` picks */
    /* +0x7C9 */ u8 unused_0x7C9[0x812 - 0x7C9];
    /* +0x812 */ u16 field_0x812;       /* the action-end block clears it */
    /* +0x814 */ u16 field_0x814;       /* the action-end block clears it */
    /* +0x816 */ u8 unused_0x816[0x89F - 0x816];
    /* +0x89F */ u8 field_0x89F;        /* the mode `fn_80137720` latches */
    /* +0x8A0 */ u8 unused_0x8A0[0x8A4 - 0x8A0];
    /* +0x8A4 */ s16 field_0x8A4;       /* the timer `fn_80137720` arms from `fn_80126494` */
    /* +0x8A6 */ u8 unused_0x8A6[0x8B3 - 0x8A6];
    /* +0x8B3 */ u8 flags_0x8B3;        /* the action bitmap `fn_801376BC`/`DC`/`04` mask */
    /* +0x8B4 */ u8 unused_0x8B4[0x8C8 - 0x8B4];
    /* +0x8C8 */ u32 field_0x8C8;       /* `fn_801260BC`'s value `fn_80137C20` latches */
    /* +0x8CC */ u32 field_0x8CC;       /* `fn_801260E0`'s value `fn_80137C20` latches */
    /* +0x8D0 */ u8 unused_0x8D0[0x8D3 - 0x8D0];
    /* +0x8D3 */ u8 field_0x8D3;        /* `fn_80137614` matches it against 1 */
    /* +0x8D4 */ u8 unused_0x8D4[0x916 - 0x8D4];
    /* +0x916 */ s16 field_0x916;
    /* +0x918 */ u8 unused_0x918[0x94A - 0x918];
    /* +0x94A */ u16 field_0x94A;       /* the action-end block clears it */
    /* +0x94C */ u16 field_0x94C;       /* the action-end block clears it */
    /* +0x94E */ u8 unused_0x94E[0x954 - 0x94E];
    /* +0x954 */ s32** field_0x954;
    /* +0x958 */ s32 field_0x958;
    /* +0x95C */ u8 field_0x95C;
    /* +0x95D */ u8 field_0x95D;
    /* +0x95E */ u8 unused_0x95E[0x9DC - 0x95E];
    /* +0x9DC */ u8 field_0x9DC;
    /* +0x9DD */ u8 field_0x9DD;
    /* +0x9DE */ u8 unused_0x9DE[2];
    /* +0x9E0 */ s32 field_0x9E0;
    /* +0x9E4 */ u8 field_0x9E4;
    /* +0x9E5 */ u8 unused_0x9E5[0xA04 - 0x9E5];
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
