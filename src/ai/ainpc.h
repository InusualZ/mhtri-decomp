/*
 * ai/ainpc.h - the AI companion record `_AINPC_W` (spelled by the map's `ai_skill_ck__FP8_AINPC_WUc` and
 *   `ai_torch_ck__FP8_AINPC_W`), shared by `ai/ai_npc.cpp`, `hud/cockpit.cpp`, `hud/cockpit_quest.cpp` and the enemy
 *   programs; the layout is the union of the offsets their bodies access, untouched offsets kept as padding.
 * size: 0x4A8 (the map's size of the `ainpc_w` object; evidenced to +0x498)
 */
#ifndef MHTRI_AI_AINPC_H
#define MHTRI_AI_AINPC_H

#include "types.h"
#include "nw4r/math.h"

/* The player work the record targets; only pointed at and forwarded by the band's own layout - a unit
 * that reads its fields includes `pl.h`. */
struct _PLW;

/* The sound work `+0x498` holds, forwarded to `fn_800DCC24` (the SE request layer's entry). */
struct _se_w;

/* The 0x10-byte record `+0x41C` points at; only its float at +0xC (the frame-budget factor) is
 * evidenced.  size: 0x10 (approximate, evidenced to +0xC) */
struct AINPCFormation {
    /* +0x0 */ u8 unused_0x00[0xC];
    /* +0xC */ f32 budget_0x0C;
};

struct _AINPC_W {
    /* +0x000 */ u8 active;            /* non-zero while the record is in use: the 0x802D44F4 band's
    /* +0x001 */ u8 unused_0x001[0x002 - 0x001];
    /* +0x002 */ u8 field_0x002;
    /* +0x003 */ u8 team;              /* read by `get_enemy_data` as its first argument */
    /* +0x004 */ u8 state;             /* the division's own sub-state every dispatcher switches on */
    /* +0x005 */ u8 sub_step;          /* the sub-state's own counter: the tables' entry index when
                                        * it is 0/1, the remaining holds otherwise */
    /* +0x006 */ u8 unused_0x006[0x00A - 0x006];
    /* +0x00A */ u8 area;              /* read by `get_enemy_data` as its second argument */
    /* +0x00B */ u8 unused_0x00B[0x024 - 0x00B];
    /* +0x024 */ nw4r::math::VEC3 vec_0x024;
    /* +0x030 */ u8 unused_0x030[0x054 - 0x030];
    /* +0x054 */ u32 field_0x054;
    /* +0x058 */ u32 field_0x058;
    /* +0x05C */ u8 unused_0x05C[0x16C - 0x05C];
    /* +0x16C */ struct _PLW* plw_0x16C;  /* the player work it targets (its +0x3C is a VEC3) */
    /* +0x170 */ u8 variant;            /* 1 or 2 - selects the odd/even arm of the dispatchers */
    /* +0x171 */ u8 field_0x171;
    /* +0x172 */ u16 field_0x172;      /* the reaction sub-state (`field_0x171 == 4`, and the
    /* +0x174 */ u8 unused_0x174[0x178 - 0x174];
    /* +0x178 */ nw4r::math::VEC3 vec_0x178;
    /* +0x184 */ u8 unused_0x184[0x190 - 0x184];
    /* +0x190 */ u32 field_0x190;       /* the effect/sound handle: only its low 16 bits are used */
    /* +0x194 */ u32 field_0x194;       /* the yaw the effect pass advances (`rotVecY`'s angle) */
    /* +0x198 */ u8 unused_0x198[0x19C - 0x198];
    /* +0x19C */ f32 field_0x19C;       /* the aim's travelled distance */
    /* +0x1A0 */ u8 unused_0x1A0[0x1A4 - 0x1A0];
    /* +0x1A4 */ u8 field_0x1A4;
    /* +0x1A5 */ u8 unused_0x1A5[0x1AC - 0x1A5];
    /* +0x1AC */ s32 field_0x1AC;
    /* +0x1B0 */ nw4r::math::VEC3 vec_0x1B0;
    /* +0x1BC */ f32 field_0x1BC;
    /* +0x1C0 */ f32 field_0x1C0;
    /* +0x1C4 */ u8 enemy_index;        /* scaled by 0xB18 into the `_ENEMY_WORK` array */
    /* +0x1C5 */ u8 unused_0x1C5[0x1C8 - 0x1C5];
    /* +0x1C8 */ u32 field_0x1C8;
    /* +0x1CC */ u8 field_0x1CC;
    /* +0x1CD */ u8 pad_0x1CD;
    /* +0x1CE */ u8 pad_0x1CE;
    /* +0x1CF */ u8 field_0x1CF;
    /* +0x1D0 */ u8 step;               /* the per-motion step every dispatcher switches on */
    /* +0x1D1 */ u8 unused_0x1D1[0x1D4 - 0x1D1];
    /* +0x1D4 */ f32 field_0x1D4;
    /* +0x1D8 */ u8 unused_0x1D8[0x1DC - 0x1D8];
    /* +0x1DC */ f32 field_0x1DC;       /* the effect's hold timer `fn_802D2904` arms */
    /* +0x1E0 */ u8 unused_0x1E0[0x1E1 - 0x1E0];
    /* +0x1E1 */ u8 field_0x1E1;
    /* +0x1E2 */ u8 unused_0x1E2[0x1E5 - 0x1E2];
    /* +0x1E5 */ u8 field_0x1E5;
    /* +0x1E6 */ u8 unused_0x1E6[0x1EC - 0x1E6];
    /* +0x1EC */ u16 field_0x1EC;       /* the SE id `fn_802D2ABC` is armed with */
    /* +0x1EE */ u8 unused_0x1EE[0x1F0 - 0x1EE];
    /* +0x1F0 */ u32 field_0x1F0;
    /* +0x1F4 */ u32 field_0x1F4;
    /* +0x1F8 */ u8 unused_0x1F8[0x20C - 0x1F8];
    /* +0x20C */ u32 field_0x20C;       /* the behaviour flag word `fn_802D6888` assembles */
    /* +0x210 */ u8 unused_0x210[0x21C - 0x210];
    /* +0x21C */ s16 field_0x21C;       /* non-zero sets the first behaviour bit */
    /* +0x21E */ u8 unused_0x21E[0x234 - 0x21E];
    /* +0x234 */ s8 field_0x234;        /* the two halves of the first held-item counter */
    /* +0x235 */ s8 field_0x235;
    /* +0x236 */ u8 unused_0x236[0x238 - 0x236];
    /* +0x238 */ s8 field_0x238;        /* the two halves of the second held-item counter */
    /* +0x239 */ s8 field_0x239;
    /* +0x23A */ u8 unused_0x23A[0x23C - 0x23A];
    /* +0x23C */ u8 field_0x23C;
    /* +0x23D */ u8 pad_0x23D;
    /* +0x23E */ s16 field_0x23E;
    /* +0x240 */ u8 unused_0x240[0x30E - 0x240];
    /* +0x30E */ u8 field_0x30E;
    /* +0x30F */ u8 unused_0x30F[0x326 - 0x30F];
    /* +0x326 */ u8 field_0x326;
    /* +0x327 */ u8 unused_0x327[0x33A - 0x327];
    /* +0x33A */ u8 field_0x33A;        /* the per-record index into the 0x805D5354/5360/5378/5390
    /* +0x33B */ u8 unused_0x33B[0x349 - 0x33B];
    /* +0x349 */ u8 field_0x349;
    /* +0x34A */ u8 field_0x34A;
    /* +0x34B */ u8 field_0x34B;
    /* +0x34C */ u8 field_0x34C;
    /* +0x34D */ u8 field_0x34D;
    /* +0x34E */ u8 field_0x34E;
    /* +0x34F */ u8 field_0x34F;
    /* +0x350 */ s16 field_0x350;
    /* +0x352 */ u16 field_0x352;
    /* +0x354 */ u8 field_0x354;
    /* +0x355 */ u8 unused_0x355[0x358 - 0x355];
    /* +0x358 */ u8 field_0x358;
    /* +0x359 */ u8 unused_0x359[0x35C - 0x359];
    /* +0x35C */ u8 field_0x35C;
    /* +0x35D */ u8 unused_0x35D;
    /* +0x35E */ s16 field_0x35E;
    /* +0x360 */ u8 unused_0x360[0x364 - 0x360];
    /* +0x364 */ u32 field_0x364;
    /* +0x368 */ u8 unused_0x368[0x374 - 0x368];
    /* +0x374 */ s16 field_0x374;       /* the hold gauge the 0x802D7464 percentage scales */
    /* +0x376 */ u8 field_0x376;
    /* +0x377 */ u8 field_0x377;
    /* +0x378 */ u8 field_0x378;
    /* +0x379 */ u8 unused_0x379[0x380 - 0x379];
    /* +0x380 */ u16 field_0x380;
    /* +0x382 */ u8 field_0x382;
    /* +0x383 */ u8 unused_0x383;
    /* +0x384 */ u8 field_0x384;
    /* +0x385 */ u8 unused_0x385[0x389 - 0x385];
    /* +0x389 */ u8 field_0x389;
    /* +0x38A */ u8 unused_0x38A[0x38E - 0x38A];
    /* +0x38E */ u8 field_0x38E;
    /* +0x38F */ u8 unused_0x38F[0x3B0 - 0x38F];
    /* +0x3B0 */ u16 hold_id_0x3B0;    /* the four held-item slot ids (0x1D = the live-chain item
    /* +0x3B2 */ u8 unused_0x3B2[0x3B4 - 0x3B2];
    /* +0x3B4 */ u16 hold_id_0x3B4;
    /* +0x3B6 */ u8 unused_0x3B6[0x3B8 - 0x3B6];
    /* +0x3B8 */ u16 hold_id_0x3B8;
    /* +0x3BA */ u8 unused_0x3BA[0x3BC - 0x3BA];
    /* +0x3BC */ u16 hold_id_0x3BC;
    /* +0x3BE */ u8 unused_0x3BE[0x3C2 - 0x3BE];
    /* +0x3C2 */ u16 field_0x3C2;       /* the motion the finish-out branch leaves the target in */
    /* +0x3C4 */ u8 unused_0x3C4[0x3C6 - 0x3C4];
    /* +0x3C6 */ u16 field_0x3C6;       /* cleared when the aim's second stage starts */
    /* +0x3C8 */ u8 unused_0x3C8[0x3D2 - 0x3C8];
    /* +0x3D2 */ u8 field_0x3D2;
    /* +0x3D3 */ u8 unused_0x3D3[0x3D4 - 0x3D3];
    /* +0x3D4 */ s16 field_0x3D4;       /* the item-page gauge, clamped into [0, +0x3D6] */
    /* +0x3D6 */ s16 field_0x3D6;       /* its cap, raised by `fn_802D6B6C` up to 0x96 */
    /* +0x3D8 */ s16 field_0x3D8;
    /* +0x3DA */ s16 field_0x3DA;
    /* +0x3DC */ s16 field_0x3DC;
    /* +0x3DE */ u8 field_0x3DE;
    /* +0x3DF */ u8 unused_0x3DF[0x3E0 - 0x3DF];
    /* +0x3E0 */ nw4r::math::VEC3 vec_0x3E0;  /* the effect's own offset vector */
    /* +0x3EC */ u16 field_0x3EC;       /* the aim's stage timer the vector pass advances */
    /* +0x3EE */ u16 field_0x3EE;       /* the stage timer's per-frame delta added to field_0x194 */
    /* +0x3F0 */ u8 field_0x3F0;        /* the tables' variant id, zero for the odd/even variants */
    /* +0x3F1 */ u8 unused_0x3F1[0x3F4 - 0x3F1];
    /* +0x3F4 */ u16 field_0x3F4;       /* the OR of the four slot levels at +0x400 */
    /* +0x3F6 */ u8 field_0x3F6;        /* the two 0/1 rolls that pick the four slot levels */
    /* +0x3F7 */ u8 field_0x3F7;
    /* +0x3F8 */ u8 field_0x3F8[4];
    /* +0x3FC */ u8 field_0x3FC[4];     /* their ids, into the 0x805D41A0 table */
    /* +0x400 */ u16 field_0x400[4];    /* their tuning values, into the 0x805D41B0 table */
    /* +0x408 */ u8 unused_0x408[0x414 - 0x408];
    /* +0x414 */ s16 field_0x414;       /* the hold countdown the best slot level arms */
    /* +0x416 */ u8 unused_0x416[0x41C - 0x416];
    /* +0x41C */ struct AINPCFormation* formation_0x41C;
    /* +0x420 */ u8 field_0x420;
    /* +0x421 */ u8 unused_0x421;
    /* +0x422 */ s16 field_0x422;
    /* +0x424 */ s16 field_0x424;       /* the hold-item frame budget */
    /* +0x426 */ u8 unused_0x426[0x427 - 0x426];
    /* +0x427 */ u8 field_0x427;        /* the five per-skill counters the finish-out clears */
    /* +0x428 */ u8 field_0x428;
    /* +0x429 */ u8 field_0x429;
    /* +0x42A */ u8 field_0x42A;
    /* +0x42B */ u8 field_0x42B;
    /* +0x42C */ s16 field_0x42C;       /* the motion the finish-out arms the target with */
    /* +0x42E */ u8 skill_0x42E[3];     /* the three skill slots `ai_skill_ck` compares against */
    /* +0x431 */ u8 field_0x431;        /* set once any of the mobility skills is unlocked */
    /* +0x432 */ u8 unused_0x432[0x434 - 0x432];
    /* +0x434 */ s32 field_0x434;       /* the two skill counters `fn_802D7464` bumps */
    /* +0x438 */ s32 field_0x438;
    /* +0x43C */ u8 unused_0x43C[0x43D - 0x43C];
    /* +0x43D */ u8 field_0x43D;
    /* +0x43E */ s16 field_0x43E;
    /* +0x440 */ u8 field_0x440;
    /* +0x441 */ u8 unused_0x441[0x442 - 0x441];
    /* +0x442 */ s16 field_0x442;
    /* +0x444 */ u8 field_0x444;
    /* +0x445 */ u8 field_0x445;
    /* +0x446 */ u8 field_0x446;
    /* +0x447 */ u8 field_0x447;
    /* +0x448 */ u8 field_0x448;
    /* +0x449 */ u8 unused_0x449[0x450 - 0x449];
    /* +0x450 */ s8 field_0x450;        /* the barrel state `ai_taru_move_ck` hands out */
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
    /* +0x46C */ u8 field_0x46C;        /* set when the item page has been acknowledged */
    /* +0x46D */ u8 unused_0x46D[0x47C - 0x46D];
    /* +0x47C */ s16 field_0x47C;       /* the torch gauge `fn_802D84D0` sets */
    /* +0x47E */ u8 unused_0x47E[0x482 - 0x47E];
    /* +0x482 */ u8 field_0x482;        /* read sign-extended at every `fn_802D9D30` call site */
    /* +0x483 */ u8 field_0x483;        /* the item page the AI NPC is on (1..4) */
    /* +0x484 */ u8 field_0x484;
    /* +0x485 */ u8 field_0x485;
    /* +0x486 */ s16 field_0x486;
    /* +0x488 */ u8 unused_0x488[0x492 - 0x488];
    /* +0x492 */ u8 field_0x492;
    /* +0x493 */ u8 unused_0x493[0x498 - 0x493];
    /* +0x498 */ struct _se_w* sound_0x498;  /* the SE work `fn_800DCC24` is handed */
    /* +0x49C */ u8 unused_0x49C[0x4A8 - 0x49C];
};

#endif /* MHTRI_AI_AINPC_H */
