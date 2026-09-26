/*
 * The cross-unit declarations `src/ai/fn_802D0F34.cpp` needs, plus the two `.bss`/`.data` blocks it
 * reads.
 *
 * Rule 2 (docs/plan.md 6.5): a declaration belongs in the owner's header.  None of the symbols
 * below has an owner header yet - their owning units are the unregistered bands above and below
 * this one (`0x802D44F4..`, `0x802C474C..0x802CC794`) or registered units whose headers do not
 * declare them - so this header carries them for the one unit that needs them now.  When an owner
 * unit registers, the declaration moves there and this file includes it.
 *
 * The signatures are the call sites' registers/stack in `build/RMHE08/obj/ai/fn_802D0F34.o`, spelled
 * with the record types (`_AINPC_W`, `_HIT_W`, `MHchar`, `nw4r::math::VEC3`) the callee's own
 * mangling or body shows.  C-linkage names are inside `extern "C"`; the mangled ones are declared at
 * C++ scope with their real signature so the front-end reproduces the map's mangling (rule 9).
 */
#ifndef MHTRI_AI_FN_802D0F34_H
#define MHTRI_AI_FN_802D0F34_H

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"
#include "ai/ai_npc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- the `ai` band below (0x802CC794..0x802D0DCC and its neighbours) ---- */
void fn_802CD770(struct _AINPC_W* self);
void fn_802CDA1C(struct _AINPC_W* self);
void fn_802CDB10(struct _AINPC_W* self);
void fn_802D0C9C(struct _AINPC_W* self);
void fn_802D0DCC(struct _AINPC_W* self);

/* ---- the `ai` band above (0x802D44F4..) ---- */
void fn_802D6888(struct _AINPC_W* self);
void fn_802D6B2C(struct _AINPC_W* self, s32 a);
void fn_802D6D4C(struct _AINPC_W* self);
void fn_802D6DF4(struct _AINPC_W* self);
void fn_802D8478(struct _AINPC_W* self);
u32 fn_802D8550(struct _AINPC_W* self);
s32 fn_802D86D4(u16* a, s16* b);
u16 fn_802D89D4(struct _AINPC_W* self);
void fn_802D92F8(struct _AINPC_W* self);
void fn_802D93B8(struct _AINPC_W* self);
u32 fn_802D948C(struct _AINPC_W* self);
u32 fn_802D94A0(struct _AINPC_W* self);
void fn_802D9A40(struct _AINPC_W* self);
s16 fn_802D9D14(struct _AINPC_W* self);
void fn_802D9D30(struct _AINPC_W* self, s32 a, u32 b, u32 c, u32 d);
u32 fn_802D7B24(struct _AINPC_W* self);

/* ---- the band below (`camera`/`light`/`stage`..) ---- */
void fn_802CB858(struct _AINPC_W* self);
s32 fn_802CB900(nw4r::math::VEC3* out, nw4r::math::VEC3* in, f32 limit);
u32 fn_8027BC48(s32 a);
s32 fn_80291B08(struct _AINPC_W* self, nw4r::math::VEC3* pos, void* angle, f32* out,
               s32 mode);
s32 fn_8029208C(nw4r::math::VEC3* probe, nw4r::math::VEC3* current, nw4r::math::VEC3* pos,
               u16 mask, u16 flags, s32 a, u8 area, f32 offset);
void fn_8029EFDC(struct _HIT_W* hit);
void fn_8029F204(struct _HIT_W* hit, void* owner, f32 value);
void fn_8029F538(struct _HIT_W* hit);
void fn_800FC0D4(struct _CP_VECTOR* dst, struct _CP_VECTOR* src);
void fn_801006A0(u8 id, nw4r::math::VEC3* pos, s32 a, u8 area, s32 b, f32 scale);
void fn_801075AC(struct _AINPC_W* self, nw4r::math::VEC3* pos, s32 a, s32 b, f32 c);
void fn_8012A624(nw4r::math::VEC3* out);
void fn_800524C0(nw4r::math::VEC3* out, nw4r::math::VEC3* a, nw4r::math::VEC3* b,
                  nw4r::math::VEC3* c, f32 d);
void fn_80073F68(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
void fn_8007F0CC(s32 a, u32 b);
void fn_80051EE0(nw4r::math::VEC3* out, nw4r::math::VEC3* in, f32 scale);
f32 fn_80050EF4(void* a, void* b);
f32 fn_80050F80(const void* a, const void* b);
void fn_80051378(nw4r::math::VEC3* out, nw4r::math::VEC3* a, nw4r::math::VEC3* b);
f32 calcDistanceSqXZ(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
s32 calcVecAng2(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
void fn_80041E40(nw4r::math::VEC3* dst, const nw4r::math::VEC3* src);
void fn_80043EA8(nw4r::math::VEC3* out);
s32 fn_8045AB38(s16 value);

/* ---- the model (`sound/fn_800DD1F0.cpp`'s range) ---- */
void fn_800E0914(struct MHchar* chr);
void fn_800E0A14(struct MHchar* chr);
void fn_800E11C0(struct MHchar* chr, u32 a, u32 b, u16 motion, s32 c, s32 d, s32 e, f32 f,
                 f32 g);
void fn_800E1640(struct MHchar* chr, f32 value);
u32 fn_800E16DC(struct MHchar* chr, u32 motion, s32 flag);
u32 fn_800E2198(struct MHchar* chr, s32 flag);

/* ---- the sound bridge ---- */
void fn_800DCB74(u32 handle, s32 index, nw4r::math::VEC3* pos);
void fn_800DCC24(u32 handle, s32 a, s32 b);
void fn_800DCCF8(u32 handle, nw4r::math::VEC3* pos, s32 a);

#ifdef __cplusplus
}
#endif

/* ---- the pooled constants (`.sdata2`, shared and unclaimed: declared, never defined - playbook 29) ---- */
extern const f32 lbl_8079A670;
extern const f32 lbl_8079A674;
extern const f32 lbl_8079A678;
extern const f32 lbl_8079A688;
extern const f32 lbl_8079A68C;
extern const f32 lbl_8079A6E0;
extern const f32 lbl_8079A6E4;
extern const f32 lbl_8079A6F0;
extern const f32 lbl_8079A6F8;
extern const f32 lbl_8079A730;
extern const f32 lbl_8079A73C;
extern const f32 lbl_8079A748;
extern const f32 lbl_8079A774;
extern const f32 lbl_8079A7B4;
extern const f32 lbl_8079A7B8;
extern const f64 lbl_8079A6C8;
extern const f32 lbl_8079A6A8;
extern const f32 lbl_8079A6B0;
extern const f32 lbl_8079A754;
extern const f32 lbl_8079A7A8;
extern const f32 lbl_8079A7AC;
extern const f32 lbl_8079A7B0;

/* ---- the tables the band reads ---- */
extern u8 lbl_806BD360[];                  /* the shared `.bss` work block `fn_802D27E0` returns
                                            * (an open array: the target addresses it
                                            * absolutely, not through `r13`) */
extern void* lbl_805D3FB8[8];              /* the two-level motion table `fn_802D282C` walks */
extern u8 lbl_805D3AD8[];                  /* the motion-entry records `fn_802D2F7C` walks (stride 0x1A) */
extern u32 lbl_80792508[];                 /* the motion ids the entries' +0x0F byte indexes */
extern void* lbl_805D4150[16];             /* the dispatcher table `ai/fn_802D0DCC.c` also uses */
extern s32 pRoot;                          /* g3d's root scene node (`.sbss` 0x80794974) */

/* ---- C++-linkage callees (their map names are manglings; rule 9) ---- */
u16 ran_suu(s32 index);
u8* get_move_work_adrs(u8 index);
u32 ai_skill_ck(struct _AINPC_W* self, u8 skill);
void hit_flag_set(struct _HIT_W* hit, u32 flags);
u8 get_now_areano(void);

/* ---- this unit's own symbols, in address order (the definitions come in the same order, so the
 * object's `.text` matches the target's layout: the unit is one TU and its functions are packed in
 * source order) ---- */
s32 ai_area_ck(struct _AINPC_W* self);
u16 ai_get_motion_no(struct _AINPC_W* self);
void get_joint_wpos_ai(struct _AINPC_W* self, u32 joint, nw4r::math::VEC3* out);

#ifdef __cplusplus
extern "C" {
#endif

u8* fn_802D27E0(void);
s32 fn_802D282C(s32 index);
void fn_802D287C(struct _AINPC_W* self, s32 motion, s32 a, s32 b, s32 c);
void fn_802D2904(struct _AINPC_W* self, s32 motion, s32 a, s32 b);
void fn_802D2910(struct _AINPC_W* self, s32 motion, s32 a, s32 b);
void fn_802D2984(struct _AINPC_W* self);
void fn_802D2990(struct _AINPC_W* self, s32 motion);
void fn_802D29A0(struct _AINPC_W* self, s32 motion);
void fn_802D29B8(struct _AINPC_W* self, s8 value);
void fn_802D29C0(struct _AINPC_W* self);
void fn_802D29CC(struct _AINPC_W* self, f32 scale);
void fn_802D29E4(struct _AINPC_W* self, s8 value);
void fn_802D29EC(struct _AINPC_W* self);
void fn_802D29F8(struct _AINPC_W* self, s8 variant);
void fn_802D2A00(struct _AINPC_W* self, u8 motion, u16 step, s16 gauge);
void fn_802D2ABC(struct _AINPC_W* self, s32 motion, s32 step, s32 gauge);
void fn_802D2AD4(struct _AINPC_W* self, u8 variant);
s32 fn_802D2B38(struct _AINPC_W* self, s32 motion, s32 step);
void fn_802D2B68(struct _AINPC_W* self);
s32 fn_802D2B78(struct _AINPC_W* self, s32 mask);
void fn_802D2B88(struct _AINPC_W* self, u16 flags);
void fn_802D2B98(struct _AINPC_W* self, u16 flags);
void fn_802D2BB0(struct _AINPC_W* self, struct _HIT_W* hit);
void fn_802D2F7C(struct _AINPC_W* self, u8 index, s32 row);
u16 fn_802D30F8(u16 angle, s32 target, u16 limit);
s32 fn_802D3184(struct _AINPC_W* self, u32 limit);
void fn_802D31FC(struct _AINPC_W* self);
void fn_802D3210(struct _AINPC_W* self, u32* angles);
void fn_802D327C(struct _AINPC_W* self, u32* angles);
s32 fn_802D32B4(struct _AINPC_W* self, u16* table);
s32 fn_802D3398(struct _AINPC_W* self, u16* table);
void fn_802D3474(struct _AINPC_W* self, s32 motion, s32 step);
void fn_802D348C(struct _AINPC_W* self);
void fn_802D35B4(struct _AINPC_W* self, s32 delta);
u32 fn_802D35EC(struct _AINPC_W* self, s32 delta);
void fn_802D3684(struct _AINPC_W* self);
void fn_802D3694(struct _AINPC_W* self, s32 delta);
void fn_802D36B8(struct _AINPC_W* self);
s16 fn_802D3984(struct _AINPC_W* self, s32 high, s32 low, u32 reverse);
void fn_802D39DC(struct _AINPC_W* self);
void fn_802D3A20(struct _AINPC_W* self);
void fn_802D3A2C(struct _AINPC_W* self, s16 value);
void fn_802D3A34(struct _AINPC_W* self, s16 delta);
void fn_802D3AD8(struct _AINPC_W* self);
void fn_802D3AF8(struct _AINPC_W* self);
void fn_802D3B10(struct _AINPC_W* self);
void fn_802D3B24(struct _AINPC_W* self);
s32 fn_802D3B34(struct _AINPC_W* self, u16 limit, u8 copy);
void fn_802D3CB8(struct _AINPC_W* self);
void fn_802D3CCC(struct _AINPC_W* self, s16 value);
void fn_802D3CD4(struct _AINPC_W* self, s16 value);
void fn_802D3CDC(struct _AINPC_W* self);
void fn_802D3CEC(struct _AINPC_W* self);
void fn_802D3CFC(struct _AINPC_W* self, u8 full);
void fn_802D3D84(struct _AINPC_W* self);
s32 fn_802D3DE8(struct _AINPC_W* self);
s32 fn_802D3E4C(struct _AINPC_W* self);
u32 fn_802D3F08(struct _AINPC_W* self);
u8 fn_802D3F1C(struct _AINPC_W* self);
s32 fn_802D3F70(struct _AINPC_W* self, u8 high, u8 low);
s32 fn_802D4020(struct _AINPC_W* self);
void fn_802D40A4(struct _AINPC_W* self, s16 id);
void fn_802D40EC(struct _AINPC_W* self, s32 id, s32 delta);
void fn_802D41C8(struct _AINPC_W* self);
void fn_802D41D4(struct _AINPC_W* self);
void fn_802D41E0(struct _AINPC_W* self, s8 selector, s32 reset);
void fn_802D4200(struct _AINPC_W* self);
void fn_802D4218(struct _AINPC_W* self);
void fn_802D4224(struct _AINPC_W* self);
void fn_802D4230(struct _AINPC_W* self, f32 value);
void fn_802D4238(struct _AINPC_W* self);
void fn_802D0F34(struct _AINPC_W* self);
void fn_802D1D90(struct _AINPC_W* self);
void fn_802D1FFC(struct _AINPC_W* self);
u32 fn_802D2024(struct _AINPC_W* self);
void fn_802D214C(struct _AINPC_W* self);
s32 fn_802D21F8(u8 value);
void fn_802D2238(struct _AINPC_W* self);
s32 fn_802D2264(struct _AINPC_W* self);
void fn_802D4248(struct _AINPC_W* self, u8 a1, u8 kind, u32 joint, s32 effect, f32 scale);

#ifdef __cplusplus
}
#endif

#endif
