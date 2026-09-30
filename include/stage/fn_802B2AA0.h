/* The `stage` band unit `stage/fn_802B2AA0.cpp` - its types, its own entry points and the
 * declarations of the symbols it calls (docs/plan.md 6.5 rules 1-5).
 *
 * `.text` 0x802B2AA0-0x802B5C58.  The unit's own notes, extent, flag evidence and residuals
 * live in the source file's header comment.
 */
#ifndef MHTRI_STAGE_FN_802B2AA0_H
#define MHTRI_STAGE_FN_802B2AA0_H

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "sound/mhchar.h"
#include "sound/fn_800D7F54.h"
#include "unsplit/ef.h"
#include "unsplit/g3d.h"
#include "unsplit/Pl.h"
#include "unsplit/unknown.h"
#include "Pl/pl_master.h"
#include "fn_8004CAD8.h"
#include "ef/fn_800CDB2C.h"
#include "stage/shell_set_func_ptr.h" /* `ShellSetFuncs` and the pointer this unit defines */

/* The band's `#pragma peephole off` is NOT here: a codegen pragma in a shared header leaks into every
 * including TU, so a lane matched a file only because of the leak and another lost rows until the
 * pragma was restated.  It belongs to the `.c`/`.cpp` that measured the dependency, and every including
 * TU that depends on it now states it there itself: `stage/fn_802B2AA0.cpp`, `enemy/em019_prog.cpp`,
 * `enemy/em020_ai.cpp` and `enemy/em020_prog.cpp`.  `tools/units/stylelint.py` rule 10 fails a codegen
 * pragma in a shared header. */

/* ------------------------------------------------------------------------------------------------
 * the shared types of the band
 * ------------------------------------------------------------------------------------------------ */

/* One 0x10-byte colour record the band stages (`stage_w` area slot +0x1BC / +0x1CC, and the blend
 * table records `fn_802B4E58` interpolates). */
/* size: 0x10 */
typedef struct StageColourRec {
    /* +0x00 */ u32 colour;
    /* +0x04 */ f32 a;
    /* +0x08 */ f32 b;
    /* +0x0C */ u32 flags;
} StageColourRec;

/* One 0x1DC-byte per-area record of `stage_w`.  The record array starts at +0xC20 (the sibling
 * `stage/stg_w.cpp` names the same stride); the two staged colour records sit at +0x1BC and +0x1CC
 * inside it, which retail folds into the single `addi r4,r4,0xDDC` after `mulli r0,r0,476`. */
/* size: 0x1DC */
typedef struct StageAreaSlot {
    /* +0x000 */ u8 pad_0x000[0x1BC];
    /* +0x1BC */ StageColourRec colour_a; /* 0xDDC */
    /* +0x1CC */ StageColourRec colour_b; /* 0xDEC */
} StageAreaSlot;

/* The `stage_w` block (map symbol, .bss 0x806B87C0, 0x2FE0 B).  Only the fields this band touches are
 * named; everything else carries its offset as padding. */
/* size: 0x2FE0 */
typedef struct StageRuntime {
    /* +0x000 */ u8 pad_0x000[0x004];
    /* +0x004 */ MHchar* area_char[4]; /* the per-area actor whose joints the band seats on */
    /* +0x014 */ u8 pad_0x014[0xBB1];
    /* +0xBC5 */ u8 mapno;
    /* +0xBC6 */ u8 areano;
    /* +0xBC7 */ u8 pad_0xBC7[0x04E];
    /* +0xC15 */ u8 joint_state[0x10];
    /* +0xC25 */ u8 pad_0xC25[0x1B7];
    /* +0xDDC */ StageAreaSlot area[16];
    /* +0x2B9C */ u8 pad_0x2B9C[0x311];
    /* +0x2EAD */ u8 field_0x2EAD[8]; /* the joint ids the current map wants visible */
    /* +0x2EB5 */ u8 pad_0x2EB5[0x0DC];
    /* +0x2F91 */ u8 field_0x2F91; /* the one-shot flag byte */
    /* +0x2F92 */ u16 field_0x2F92; /* the 16-bit area mask */
    /* +0x2F94 */ u32 field_0x2F94[16]; /* one 4-second timer per area */
    /* +0x2FD4 */ u8 pad_0x2FD4[0x008];
    /* +0x2FDC */ s16 field_0x2FDC; /* the demo-placement countdown */
    /* +0x2FDE */ u8 pad_0x2FDE[0x002];
} StageRuntime;

/* One 0x4F8-byte per-area runtime object (two of them at `lbl_806BB7E0`). */
/* size: 0x4F8 */
typedef struct StageAreaObj {
    /* +0x000 */ nw4r::math::VEC3 seat_pos;
    /* +0x00C */ nw4r::math::VEC3 seat_pos2;
    /* +0x018 */ u8 pad_0x018[0x018];
    /* +0x030 */ f32 pitch;
    /* +0x034 */ f32 span;
    /* +0x038 */ u8 pad_0x038[0x02C];
    /* +0x064 */ u32 ang_x; /* calcVecAngXY's first output */
    /* +0x068 */ u32 ang_y; /* calcVecAngXY's second output */
    /* +0x06C */ s32 field_0x06C;
    /* +0x070 */ s32 field_0x070;
    /* +0x074 */ s32 field_0x074;
    /* +0x078 */ s32 field_0x078;
    /* +0x07C */ u8 field_0x07C; /* the camera mode `get_cfg` selects */
    /* +0x07D */ u8 pad_0x07D[3];
    /* +0x080 */ s32 field_0x080;
    /* +0x084 */ u8 field_0x084;
    /* +0x085 */ u8 pad_0x085[0x00D];
    /* +0x092 */ u8 field_0x092;
    /* +0x093 */ u8 pad_0x093[0x00B];
    /* +0x09E */ u8 field_0x09E;
    /* +0x09F */ u8 field_0x09F;
    /* +0x0A0 */ u8 field_0x0A0;
    /* +0x0A1 */ u8 field_0x0A1;
    /* +0x0A2 */ u8 field_0x0A2;
    /* +0x0A3 */ u8 field_0x0A3;
    /* +0x0A4 */ u8 field_0x0A4;
    /* +0x0A5 */ u8 field_0x0A5;
    /* +0x0A6 */ u8 field_0x0A6;
    /* +0x0A7 */ u8 pad_0x0A7;
    /* +0x0A8 */ u16 field_0x0A8;
    /* +0x0AA */ u8 field_0x0AA;
    /* +0x0AB */ u8 field_0x0AB;
    /* +0x0AC */ u8 field_0x0AC;
    /* +0x0AD */ u8 field_0x0AD;
    /* +0x0AE */ u16 field_0x0AE;
    /* +0x0B0 */ u8 field_0x0B0;
    /* +0x0B1 */ u8 pad_0x0B1;
    /* +0x0B2 */ u16 field_0x0B2;
    /* +0x0B4 */ u8 field_0x0B4;
    /* +0x0B5 */ u8 field_0x0B5;
    /* +0x0B6 */ u8 field_0x0B6;
    /* +0x0B7 */ u8 field_0x0B7;
    /* +0x0B8 */ nw4r::math::VEC3 field_0x0B8;
    /* +0x0C4 */ nw4r::math::VEC3 field_0x0C4;
    /* +0x0D0 */ nw4r::math::VEC3 field_0x0D0;
    /* +0x0DC */ nw4r::math::VEC3 field_0x0DC;
    /* +0x0E8 */ nw4r::math::VEC3 field_0x0E8;
    /* +0x0F4 */ nw4r::math::VEC3 field_0x0F4;
    /* +0x100 */ u8 field_0x100[4];
    /* +0x104 */ u8 pad_0x104[8];
    /* +0x10C */ u8 field_0x10C;
    /* +0x10D */ u8 field_0x10D;
    /* +0x10E */ u8 field_0x10E;
    /* +0x10F */ u8 field_0x10F;
    /* +0x110 */ u16 field_0x110;
    /* +0x112 */ u8 pad_0x112[2];
    /* +0x114 */ u32 field_0x114;
    /* +0x118 */ u8 field_0x118;
    /* +0x119 */ u8 pad_0x119;
    /* +0x11A */ u16 field_0x11A;
    /* +0x11C */ u16 field_0x11C;
    /* +0x11E */ u8 pad_0x11E[0x26];
    /* +0x144 */ u8 field_0x144;
    /* +0x145 */ u8 field_0x145;
    /* +0x146 */ u8 pad_0x146[0x042];
    /* +0x188 */ u8 field_0x188; /* this area's stage-state byte */
    /* +0x189 */ u8 pad_0x189[0x0A7];
    /* +0x230 */ u8 field_0x230;
    /* +0x231 */ u8 pad_0x231[0x053];
    /* +0x284 */ u8 field_0x284;
    /* +0x285 */ u8 pad_0x285[0x103];
    /* +0x388 */ u8 field_0x388;
    /* +0x389 */ u8 pad_0x389[0x0B3];
    /* +0x43C */ u8 field_0x43C;
    /* +0x43D */ u8 pad_0x43D[0x057];
    /* +0x494 */ u32 field_0x494; /* the area's 0xB20-byte move work */
    /* +0x498 */ u8 field_0x498; /* kind index into `get_cfg`'s table */
    /* +0x499 */ u8 field_0x499;
    /* +0x49A */ u8 field_0x49A;
    /* +0x49B */ u8 field_0x49B; /* the resolved kind, 0xFF while unresolved */
    /* +0x49C */ u8 pad_0x49C[0x18];
    /* +0x4B4 */ u8 field_0x4B4;
    /* +0x4B5 */ u8 pad_0x4B5[0x13];
    /* +0x4C8 */ u8 field_0x4C8;
    /* +0x4C9 */ u8 pad_0x4C9[0x13];
    /* +0x4DC */ u8 field_0x4DC;
    /* +0x4DD */ u8 pad_0x4DD[7];
    /* +0x4E4 */ u32 field_0x4E4;
    /* +0x4E8 */ u8 field_0x4E8;
    /* +0x4E9 */ u8 pad_0x4E9[7];
    /* +0x4F0 */ u32 field_0x4F0;
    /* +0x4F4 */ u16 field_0x4F4;
    /* +0x4F6 */ u8 pad_0x4F6;
    /* +0x4F7 */ u8 field_0x4F7;
} StageAreaObj;

/* One 0x10-byte record of the area blend table `fn_802B4CD0` picks and `fn_802B4D24` reads. */
/* size: 0x10 */
/* The two joint-id lists one map/area entry holds: the ids to show and the ids to hide. */
/* size: 0x08 */
typedef struct StageJointLists {
    /* +0x00 */ u8* show;
    /* +0x04 */ u8* hide;
} StageJointLists;

/* One 8-byte entry of the map's demo-marker table: the map it belongs to, how many offsets it has and
 * the offset table itself. */
/* size: 0x08 */
typedef struct StageDemoEntry {
    /* +0x00 */ u8 mapno;
    /* +0x01 */ u8 count;
    /* +0x02 */ u8 pad_0x02[2];
    /* +0x04 */ u8* offsets;
} StageDemoEntry;

/* size: 0x10 */
typedef struct StageBlendEntry {
    /* +0x00 */ u32 kind;
    /* +0x04 */ u32 colour_a;
    /* +0x08 */ u32 colour_b;
    /* +0x0C */ u32 colour_c;
} StageBlendEntry;

/* The `MHchar` actor as this band's joint loop sees it: the joint count sits inside the
 * `sound/mhchar.h` view's padding at +0x15C, so the loop keeps a local view instead of editing a
 * shared header. */
/* size: 0x160 */
typedef struct StageActorJoints {
    /* +0x000 */ u8 pad_0x000[0x15C];
    /* +0x15C */ u32 joint_count;
} StageActorJoints;

/* The colour blend source `fn_802B2AA0` reads: three 4-word groups, each a base colour, an axis
 * colour and the two targets the base blends towards. */
/* size: 0x10 */
typedef struct StageBlendGroup {
    /* +0x00 */ u32 base;
    /* +0x04 */ u32 axis;
    /* +0x08 */ u32 target_a;
    /* +0x0C */ u32 target_b;
} StageBlendGroup;

/* size: 0x38 */
typedef struct StageBlendRec {
    /* +0x00 */ s32 enabled;
    /* +0x04 */ f32 se_frame;
    /* +0x08 */ StageBlendGroup group[3];
} StageBlendRec;

#include "g3d/g3d_scnroot.h" /* `nw4r::g3d::ScnRoot`, the scene root the two camera calls go through */

/* ------------------------------------------------------------------------------------------------
 * the block symbols (.bss: no registered unit owns them, so the declarations live here)
 * ------------------------------------------------------------------------------------------------ */

extern "C" u8 stage_w[];
extern "C" u8 lbl_806BB7B8[];
extern "C" u8 lbl_806BB7C4[];
extern "C" u8 lbl_806BB7D0[];
extern "C" u8 lbl_806BB7E0[];

/* ------------------------------------------------------------------------------------------------
 * the pooled `.data` / `.sdata2` constants (declared, never defined: the pool belongs to the data pass)
 * ------------------------------------------------------------------------------------------------ */

/* .sdata2 floats */
extern f32 lbl_8079A448; /* 0.0f     */
extern f32 lbl_8079A468; /* 1.0f     */
extern f32 lbl_8079A484; /* 0.5f     */
extern f32 lbl_8079A49C; /* 16384.0f */
extern f32 lbl_8079A4A0; /* 23700.0f */
extern f32 lbl_8079A4A4; /* -25000.0f */
extern f32 lbl_8079A4A8; /* 16900.0f */
extern f32 lbl_8079A4AC; /* 38400.0f */
extern f32 lbl_8079A4B0; /* -32900.0f */
extern f32 lbl_8079A4B4; /* -44000.0f */
extern f32 lbl_8079A4B8; /* 3028.0f  */
extern f32 lbl_8079A4BC; /* 200.0f   */
extern f32 lbl_8079A4C0; /* 30.0f    */
extern f32 lbl_8079A4C4; /* 0.3f     */
extern f32 lbl_8079A4C8; /* 52.0f    */
extern f32 lbl_8079A4CC; /* -573.0f  */
extern f32 lbl_8079A4D0; /* 1465.0f  */
extern f32 lbl_8079A4D8; /* 0.0f     */
extern f32 lbl_8079A4DC; /* 0.5f     */
extern f32 lbl_8079A4E0; /* -1.0f    */
extern f32 lbl_8079A4E4; /* 100.0f   */
extern f32 lbl_8079A4E8; /* 55.0f    */

/* .data - the band's own tables (0x805CF60C-0x805CFBE8; only the jump table is claimed, see the
 * file header) */
extern StageJointLists* lbl_805CF60C[];
extern StageJointLists* lbl_805CF644[];
extern StageJointLists* lbl_805CF6A0[]; /* the mapno 1/12 per-area list pair */
extern u8 lbl_805CF6D4[];
extern u8 lbl_805CF6E8[];
extern u8 lbl_805CF6F4[];
extern u8 lbl_805CF700[];
extern f32 lbl_805CF784[];
extern u8 lbl_805CF7C0[]; /* the 0xC-byte Vec records the strips seat on */
extern f32 lbl_805CF7F0[];
extern u8 lbl_805CF7FC[];
extern u8* lbl_805CF8E0[];
extern u8* lbl_805CF8F0[];
extern StageDemoEntry lbl_805CFA38[];
extern f32 lbl_805CFA58[];
extern StageBlendEntry lbl_805CFA68[];
extern StageBlendEntry lbl_805CFAE8[];
extern StageBlendEntry lbl_805CFB68[];

/* .sdata - the band's joint-id lists */
extern StageJointLists* lbl_80792310;
extern StageJointLists* lbl_80792330;
extern StageJointLists* lbl_807923C8[];
extern StageJointLists lbl_80792410;
extern StageJointLists lbl_80792440;
extern StageJointLists lbl_80792450;
extern StageJointLists lbl_80792470;
extern u8 lbl_80792418[0x4];
extern u8 lbl_8079241C[0x4];
extern u8 lbl_80792420[0x8];
extern u8 lbl_80792428[0x8];
extern u8 lbl_80792430[0x8];
extern u8 lbl_80792458[0x8];
extern u8 lbl_80792460[0x4];
extern u8 lbl_80792464[0x4];
extern u8 lbl_80792498[];
extern u8 lbl_8079249C[];
extern u8 lbl_807924A0[];
extern u8 lbl_807924A4[];
extern u8 lbl_807924A8[];

/* ------------------------------------------------------------------------------------------------
 * the callees (C linkage: the map spells every one of these plainly)
 * ------------------------------------------------------------------------------------------------ */

extern "C" s32 screen_split_mode_ck(void);
/* 0x8004723C is `mh3_pad.cpp`'s; spelled as its owner's header does (this call site passes `s32*`,
 * `camera/fn_802B5C58.cpp` a `void**`, hence the erased types). */
extern "C" void* word_copy_return_dst(void* out, const void* src);
extern "C" void fn_800473F4(u8 value);
extern "C" void fn_80057DE0(s32 a, s32 count, u32* colours, f32 param);
extern "C" void fn_80057EF4(s32 a, s32 count, u32* colours, f32 param);
extern "C" void camera_posture_info_ctor(s32* out);
extern "C" s32 fn_8007A1B8(const void* a, s32 b, void* c, void* d, s32 e, s32 f, void* g);
extern "C" s32 fn_80082C80(s32 root, s32 mode);
extern "C" void my_player_no_set(s8 value);  /* s8 is the owner's spelling (the argument
 * narrows with `extsb` in retail); `include/ef/fn_800CDB2C.h` declares it the same way. */
extern "C" void fn_800DD38C(void);
extern "C" u32 fn_800E16DC(void* chr, s32 a, s32 b, f32 c, f32 d);
extern "C" void fn_800FA3B8(nw4r::math::VEC3* out);
extern "C" void fn_800FA420(nw4r::math::VEC3* out);
extern "C" void fn_801FF984(void* out);
extern "C" u32 fn_8021F218(void);
extern "C" f32 fn_8021F228(u32 index);
extern "C" s32 fn_8021F238(void);
extern "C" u32 Pl_motion_input_ck(void* work);
extern "C" s32 fn_80291BBC(void* a, u8 area, s32 handle, void* out, s32 limit);
extern "C" void fn_802AE1DC(StageColourRec* rec);
extern "C" void fn_802AE250(void* out, const void* in);
extern "C" s32 fn_802AFF38(void);
extern "C" u8* fn_802B04A0(u8 kind);
extern "C" void fn_802B0B7C(StageRuntime* st, u8 index);
extern "C" void fn_802B2E2C(void);
extern "C" void fn_802B2F3C(StageColourRec* dst, const StageColourRec* src);
extern "C" void fn_802B2F60(StageRuntime* st, u8 kind);
extern "C" void fn_802B3270(StageRuntime* st, MHchar* chr, u8 mode);
extern "C" u8 fn_802FB8C4(void);
extern "C" u8 fn_802FB8EC(s32 index);
extern "C" u8 fn_802FB900(void);
extern "C" u8 fn_802FB97C(void);
extern "C" u32 fn_802FB9F8(void);
extern "C" u8 quest_id_head_ck(void);
extern "C" u8 quest_id_tail_ck(void);
extern "C" s32 fn_802B2978(u32 from, u32 to, f32 t);
extern "C" s32 fn_802B46DC(u8 index);
extern "C" s32 camera_angle_y_get(void);
extern "C" void fn_802B4680(void* plw, u8 index);
extern "C" f32 fn_802B4D24(s16 index, f32 span);
extern "C" u32 fn_802B53CC(u32 base);
extern "C" u32 fn_802B5488(u32 block);
extern "C" u32 fn_802B54C4(u32 block);
extern "C" u32 fn_802B5538(u32 rec);
extern "C" u32 fn_802B5590(u32 slot);
extern "C" u32 fn_802B55E8(u32 block);
extern "C" StageBlendEntry* fn_802B4CD0(void);
extern "C" void fn_802B5640(StageAreaObj* area, u8 index);
extern "C" void fn_802B592C(u8 index, s32 camera);
extern "C" void fn_802B5980(StageAreaObj* area);
extern "C" void fn_802B59F8(StageAreaObj* area);
extern "C" void fn_802B5C58(StageAreaObj* area);
extern "C" s32 fn_802B700C(StageAreaObj* area);
extern "C" s32 fn_802B7034(StageAreaObj* area);
extern "C" s32 fn_802B95E8(StageAreaObj* area);
extern "C" s32 fn_802B9828(StageAreaObj* area);
extern "C" s32 fn_802B9C04(StageAreaObj* area);
extern "C" s32 fn_802BA25C(StageAreaObj* area);
extern "C" s32 fn_802BA39C(StageAreaObj* area);
extern "C" s32 fn_802BB118(StageAreaObj* area);
extern "C" s32 fn_802BB7FC(StageAreaObj* area);
extern "C" s32 fn_802BBEE0(StageAreaObj* area);
extern "C" s32 fn_802BC1E4(StageAreaObj* area);
extern "C" void fn_802BD658(void);
extern "C" void fn_802BDDB0(nw4r::math::VEC3* out);
extern "C" u32 fn_802BE088(void);
extern "C" void fn_802BE0F4(s32 a, StageAreaObj* area, void* vec, f32 pitch, f32 span);
extern "C" void fn_802BE1EC(StageAreaObj* area);
extern "C" void fn_802BE4FC(s32 kind);
extern "C" void fn_802BEAAC(StageAreaObj* area, u8 kind);
extern "C" void fn_802C20A4(nw4r::math::VEC3* vec);
extern "C" void fn_802FBA94(void);
extern "C" void lb_sub12_send(u8 index);
extern "C" s32 quest_time_elapsed_get(void);
extern "C" s32 quest_time_limit_get(void);
extern "C" u8 fn_803A8F60(s32 value);
extern "C" u8 get_cfg(u8 kind, u8 mode);

/* C++ free functions whose map names are manglings (rule 9). */
u32 LbCheckKujiraEvent(void);
nw4r::math::VEC3 get_camera_pos(void);
void eft028_set_koware(u8 kind, nw4r::math::VEC3* pos, u8 area, long param);

/* ------------------------------------------------------------------------------------------------
 * the unit's own entry points (rule 2: other units include this header, not a local copy)
 * ------------------------------------------------------------------------------------------------ */

extern "C" s32 fn_802B45D4(void);
extern "C" void fn_802B45F4(u8 index);
extern "C" u32 fn_802B53CC(u32 base);
extern "C" u32 fn_802B5488(u32 block);
extern "C" u32 fn_802B54C4(u32 block);
extern "C" u32 fn_802B5538(u32 rec);
extern "C" u32 fn_802B5590(u32 slot);
extern "C" u32 fn_802B55E8(u32 block);

#endif /* MHTRI_STAGE_FN_802B2AA0_H */
