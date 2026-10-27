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
void ai_npc_player_watch(struct _AINPC_W* self);
void ai_npc_target_refresh(struct _AINPC_W* self);
void ai_npc_plan_decide(struct _AINPC_W* self);
void ai_npc_plan_dispatch(struct _AINPC_W* self);
void fn_802D0DCC(struct _AINPC_W* self);

/* ---- `ai/ai_npc.cpp`'s own rows above the view (0x802D44F4..) ---- */
void fn_802D6888(struct _AINPC_W* self);
void fn_802D6B2C(struct _AINPC_W* self, s16 delta);
void ai_npc_attack_update(struct _AINPC_W* self);
void ai_npc_defense_update(struct _AINPC_W* self);
void ai_npc_player_idle_count(struct _AINPC_W* self);
u32 fn_802D8550(struct _AINPC_W* self);
u32 fn_802D86D4(struct _AINPC_W* self, u16* id, s16* count);
u16 fn_802D89D4(struct _AINPC_W* self, u16 motion);
void fn_802D92F8(struct _AINPC_W* self);
void ai_npc_element_frame(struct _AINPC_W* self);
u32 ai_npc_mode1_ck(struct _AINPC_W* self);
u32 fn_802D94A0(struct _AINPC_W* self);
void fn_802D9A40(struct _AINPC_W* self);
s16 ai_npc_counter_window_get(struct _AINPC_W* self);
u32 ai_npc_talk(struct _AINPC_W* self, s32 npc, u32 msg, u32 arg, u32 kind);
u32 fn_802D7B24(struct _AINPC_W* self);

/* ---- the band below (`camera`/`light`/`stage`..) ---- */
void ai_npc_motion_run(struct _AINPC_W* self);
u8 ai_npc_in_range_ck(nw4r::math::VEC3* a, nw4r::math::VEC3* b, f32 limit);
u32 Pl_motion_input_ck(s32 a);
s32 pl_ground_check_all_layers(struct _AINPC_W* self, nw4r::math::VEC3* pos, void* angle, f32* out,
               s32 mode);
s32 coll_wall_slide(nw4r::math::VEC3* to, nw4r::math::VEC3* from, f32 radius, nw4r::math::VEC3* pos, u32 kind,
               u16 layers, u8 mode, u8 area);
void hit_attack_list_push(struct _HIT_W* hit);
void hit_data_apply(struct _HIT_W* hit, struct _HIT_DATA* data);
void hit_flags_clear(struct _HIT_W* hit);
void eft_rot_vec_copy(struct _CP_VECTOR* dst, struct _CP_VECTOR* src);
void eft004_key_spawn(u8 id, nw4r::math::VEC3* pos, s32 a, u8 area, s32 b, f32 scale);
void eft013_npc_place(struct _AINPC_W* self, nw4r::math::VEC3* pos, u32 a, f32 scale, u32 b);
void em_land_data_init(::LandData* land);
void vec3_move_damp(nw4r::math::VEC3* from, nw4r::math::VEC3* to, nw4r::math::VEC3* dir, f32 k,
                  nw4r::math::VEC3* out);
void addVec3To(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
void g3d_root_model_bind(s32 a, u32 b);
void addVec3To(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
void g3d_root_model_bind(s32 a, u32 b);
void vec3_scale(nw4r::math::VEC3* out, nw4r::math::VEC3* in, f32 scale);
f32 vec3_distance(void* a, void* b);
f32 calcVecDistXZ(const void* a, const void* b);
void addVec3(nw4r::math::VEC3* out, nw4r::math::VEC3* a, nw4r::math::VEC3* b);
f32 calcDistanceSqXZ(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
s32 calcVecAng2(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
 /* owner `src/mh3_pad.cpp`; its body returns `dst` */
s32 abs(s16 value);

/* ---- the model (`sound/mhchar.cpp`'s range) ---- */
void mhchar_model_mtx_update(struct MHchar* chr);
void mhchar_joint_mtx_get(struct MHchar* chr, u32 joint, nw4r::math::MTX34* out);
void mhchar_motion_set(struct MHchar* chr, u32 kind, s32 index, u16 motion, s32 blend, f32 start, s32 loop,
                 u32 flags, f32 rate);
void mhchar_motion_speed_set(struct MHchar* chr, f32 value);
u32 mhchar_frame_check(struct MHchar* chr, u16 layer, s32 flag, f32 low, f32 high);
u32 mhchar_motion_end_ck(struct MHchar* chr, s32 flag);

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
extern u32 lbl_80792508[2];                 /* the motion ids the entries' +0x0F byte indexes */
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

u8* ainpc_work_get(void);
s32 fn_802D282C(s32 index);
void ai_npc_motion_play(struct _AINPC_W* self, s32 motion, s32 a, s32 b, s32 c);
void ai_npc_motion_set(struct _AINPC_W* self, u32 motion, s32 a, s32 b);
void ai_npc_motion_change(struct _AINPC_W* self, s32 motion, s32 a, s32 b);
u32 ai_npc_motion_end_ck(struct _AINPC_W* self);
u32 ai_npc_frame_ck(struct _AINPC_W* self, u32 flag, f32 low, f32 high);
u32 fn_802D29A0(struct _AINPC_W* self, u32 layer, f32 low, f32 high);
void fn_802D29B8(struct _AINPC_W* self, s8 value);
void fn_802D29C0(struct _AINPC_W* self);
void fn_802D29CC(struct _AINPC_W* self, f32 scale);
void fn_802D29E4(struct _AINPC_W* self, s8 value);
void fn_802D29EC(struct _AINPC_W* self);
void ai_npc_variant_set(struct _AINPC_W* self, u8 variant);
void fn_802D2A00(struct _AINPC_W* self, u8 motion, u16 step, u16 gauge);
void fn_802D2ABC(struct _AINPC_W* self, s32 motion, s32 step, s32 gauge);
void ai_npc_base_state_set(struct _AINPC_W* self, u32 variant);
u32 ai_npc_motion_step_ck(struct _AINPC_W* self, s32 motion, s32 step);
void ai_npc_joint_mtx_get(struct _AINPC_W* self, u32 joint, nw4r::math::MTX34* out);
s32 ai_npc_flag_ck(struct _AINPC_W* self, s32 mask);
void fn_802D2B88(struct _AINPC_W* self, u16 flags);
void fn_802D2B98(struct _AINPC_W* self, u16 flags);
void fn_802D2BB0(struct _AINPC_W* self, struct _HIT_W* hit);
void fn_802D2F7C(struct _AINPC_W* self, u8 index, s32 row);
u16 fn_802D30F8(u16 angle, u16 target, u32 limit);
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
void fn_802D3A34(struct _AINPC_W* self, s32 delta);
void fn_802D3AD8(struct _AINPC_W* self);
void fn_802D3AF8(struct _AINPC_W* self);
void fn_802D3B10(struct _AINPC_W* self);
void fn_802D3B24(struct _AINPC_W* self);
u32 fn_802D3B34(struct _AINPC_W* self, u16 limit, u8 copy);
void fn_802D3CB8(struct _AINPC_W* self);
void fn_802D3CCC(struct _AINPC_W* self, s16 value);
void fn_802D3CD4(struct _AINPC_W* self, s16 value);
void fn_802D3CDC(struct _AINPC_W* self);
void fn_802D3CEC(struct _AINPC_W* self);
void fn_802D3CFC(struct _AINPC_W* self, u8 full);
void fn_802D3D84(struct _AINPC_W* self);
u32 fn_802D3DE8(struct _AINPC_W* self);
s32 ai_npc_counter_roll(struct _AINPC_W* self);
u32 fn_802D3F08(struct _AINPC_W* self);
u8 fn_802D3F1C(struct _AINPC_W* self);
s32 fn_802D3F70(struct _AINPC_W* self, u8 high, u8 low);
u32 fn_802D4020(struct _AINPC_W* self);
void fn_802D40A4(struct _AINPC_W* self, u16 id);
void fn_802D40EC(struct _AINPC_W* self, u16 id, s16 delta);
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
u8 fn_802D21F8(u8 value);
void fn_802D2238(struct _AINPC_W* self);
s32 fn_802D2264(struct _AINPC_W* self);
void fn_802D4248(struct _AINPC_W* self, u8 a1, u8 kind, u32 joint, s32 effect, f32 scale);

#ifdef __cplusplus
}
#endif

#endif
