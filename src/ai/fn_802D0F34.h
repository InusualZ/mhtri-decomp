/*
 * ai/fn_802D0F34.h - the callees, `.bss` and `.data` that `ai/ai_npc.cpp`'s `namespace view_fn_802D0F34` bodies use,
 *   spelled with the view's record types (`_AINPC_W`, `_HIT_W`, `MHchar`); C-linkage names inside `extern "C"`, mangled
 *   ones at C++ scope (rule 9).  `lobby/fn_8030121C.cpp` includes it too.
 */
#ifndef MHTRI_AI_FN_802D0F34_H
#define MHTRI_AI_FN_802D0F34_H

#include "types.h"
#include "nw4r/math.h"
#include "ef/pRoot.h"
#include "pl.h"
#include "ai/ai_npc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- `ai/ai_npc.cpp`'s own rows below the view (0x802CC794..0x802D0F34) ---- */
void fn_802CD770(struct _AINPC_W* self);
void fn_802CDA1C(struct _AINPC_W* self);
void fn_802CDB10(struct _AINPC_W* self);
void fn_802D0C9C(struct _AINPC_W* self);
void fn_802D0DCC(struct _AINPC_W* self);

/* ---- `ai/ai_npc.cpp`'s own rows above the view (0x802D44F4..) ---- */
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
u32 Pl_motion_input_ck(s32 a);
s32 fn_80291B08(struct _AINPC_W* self, nw4r::math::VEC3* pos, void* angle, f32* out,
               s32 mode);
s32 fn_8029208C(nw4r::math::VEC3* probe, nw4r::math::VEC3* current, nw4r::math::VEC3* pos,
               u16 mask, u16 flags, s32 a, u8 area, f32 offset);
void hit_attack_list_push(struct _HIT_W* hit);
void hit_data_apply(struct _HIT_W* hit, void* owner, f32 value);
void hit_flags_clear(struct _HIT_W* hit);
void eft_rot_vec_copy(struct _CP_VECTOR* dst, struct _CP_VECTOR* src);
void fn_801006A0(u8 id, nw4r::math::VEC3* pos, s32 a, u8 area, s32 b, f32 scale);
void fn_801075AC(struct _AINPC_W* self, nw4r::math::VEC3* pos, s32 a, s32 b, f32 c);
void fn_8012A624(nw4r::math::VEC3* out);
void fn_800524C0(nw4r::math::VEC3* out, nw4r::math::VEC3* a, nw4r::math::VEC3* b,
                  nw4r::math::VEC3* c, f32 d);
void addVec3To(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
void g3d_root_model_bind(s32 a, u32 b);
void addVec3To(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
void g3d_root_model_bind(s32 a, u32 b);
void vec3_scale(nw4r::math::VEC3* out, nw4r::math::VEC3* in, f32 scale);
f32 fn_80050EF4(void* a, void* b);
f32 calcVecDistXZ(const void* a, const void* b);
void addVec3(nw4r::math::VEC3* out, nw4r::math::VEC3* a, nw4r::math::VEC3* b);
f32 calcDistanceSqXZ(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
s32 calcVecAng2(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
 /* owner `src/mh3_pad.cpp`; its body returns `dst` */
s32 abs(s16 value);

/* ---- the model (`sound/mhchar.cpp`'s range) ---- */
void fn_800E0914(struct MHchar* chr);
void mhchar_joint_mtx_get(struct MHchar* chr);
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

/* ---- the pooled constants (`.sdata2`, this unit's own pool, declared never defined - playbook 29) ---- */
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
extern void* lbl_805D3FB8[8];              /* the two-level motion table `fn_802D282C` walks */
extern u8 lbl_805D3AD8[];                  /* the motion-entry records `fn_802D2F7C` walks (stride 0x1A) */
extern u32 lbl_80792508[];                 /* the motion ids the entries' +0x0F byte indexes */
extern void* lbl_805D4150[16];             /* the dispatcher table `fn_802D0DCC` also uses */

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
s32 ai_npc_motion_step_ck(struct _AINPC_W* self, s32 motion, s32 step);
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
