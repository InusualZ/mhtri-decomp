/* enemy/fn_801D80EC.cpp - one enemy's action band, `.text` 0x801D80EC..0x801DB8E0 (49 functions,
 * 0x37F4 bytes), the same family as the registered units `enemy/fn_801D428C.cpp` (the band below) and
 * `enemy/fn_801DB8E0.cpp` (the band above).
 *
 * What it is.  An enemy's per-motion step band: every function takes the shared `_ENEMY_WORK`, opens
 * on `switch (self->state)` and arms/closes a motion through the same `em_mot_set`/`em_mot_end_ck`/
 * `em_action_finish` set the neighbouring band uses.  `fn_801DB724` is the band's top-level action
 * dispatcher (it tail-branches into `fn_801D791C`/`fn_801D8078` of `enemy/fn_801D428C.cpp` for
 * actions 0/1 and into this range's own `fn_801D8950`/`fn_801D8DC8`/`fn_801DA2E0`/... for the rest);
 * `fn_801D8950`, `fn_801D8DC8`, `fn_801DA2E0`, `fn_801DA7A0`, `fn_801DABD4`, `fn_801DAC18`,
 * `fn_801DAFC8`, `fn_801DB058`, `fn_801DB6D0` are the intermediate `state_sub` dispatchers over the
 * step functions.
 *
 * Module and name (brief section 2, in evidence order).
 *   1. No `__FILE__` string covers the range: the range references no `.data`/`.rodata` string at all
 *      (its only absolute loads are the shared `.data` pools `lbl_805705xx`/`lbl_805706xx` and the
 *      enemy `jumptable_805B6A90`/`jumptable_805B6AEC`/`jumptable_805B71A8`).  The only `enemy`
 *      source name in the image is `enemy_control.cpp`, and it belongs to the registered
 *      `enemy/enemy_control.cpp` unit far below.
 *   2. `dumpmap.py lookup` answers `zz_XXXXXXX_` for every address of the range (a placeholder is not
 *      evidence).
 *   3. The code is enemy-band: it brackets `enemy/fn_801D428C.cpp`/`enemy/fn_801DB8E0.cpp`, every
 *      function takes the `_ENEMY_WORK` and the module's naming scheme is the map's own `fn_XXXXXXXX`
 *      stem.
 * The file therefore keeps the map stem (brief option 4); no name was invented.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup`: every address of the range answers `zz_XXXXXXXX_` in the
 * shared runtime dump and carries a bare `fn_XXXXXXXX = .text:0x...` entry in
 * config/RMHE08/symbols.txt; no `__FILE__` string is reachable from the range).
 *
 * Language.  C++: the range's callees are C++ manglings (`em_frame_check__FP11_ENEMY_WORKUsff`,
 * `get_em_chg_scale__FP11_ENEMY_WORK`, `calcVecAng2__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3`,
 * `rotVecY__FPQ34nw4r4math4VEC3Ul`, `setVector3__FP...`, `mulVecMat__FP...`,
 * `calcVecAngXY__FP...`, `findInterSection__FP...`, `get_joint_wpos_em__FP...`), and the `MHchar`
 * effects are reached through the `shell` callback table.  Rule 9: those are declared at C++ scope
 * with the signature their mangling encodes and called through it; every `fn_*` definition stays
 * `extern "C"`.
 *
 * Seam.  Unproven, as the brief says.  The left edge at 0x801D80EC is the discovery cut (the band
 * below, `enemy/fn_801D428C.cpp`, ends exactly there and its `fn_801D8078` dispatcher is action 1 of
 * this range's `fn_801DB724`); the right edge at 0x801DB8E0 is exact (`fn_801DB7D8` ends there and
 * `enemy/fn_801DB8E0.cpp` starts there).
 *
 * Types (rule 1).  `_ENEMY_WORK` is the shared record `include/enemy/ENEMY_WORK.h` owns.  This unit
 * added the bytes it measures to that header: the second offset vector `vec_0x31C` (0x31C, a union
 * view over the two floats `field_0x320`/`field_0x324` the neighbouring effect units name) and the
 * `u16 field_0xAEA` the shell-effect calls hand over as their flag word.
 *
 * Declarations (rule 2).  The enemy band's owner headers do not carry all the call sites'
 * signatures this range needs, and MWCC rejects two different C-linkage spellings of one name in a
 * TU, so - exactly as `enemy/fn_801D428C.cpp` does - this file declares its foreign callees itself
 * and the header corrections are recorded as a `shared-file` request in the outbox.
 *
 * Status / residuals.  All 49 rows are reconstructed and compile; 18 are byte-identical (100 %).  The
 * measured per-symbol scores are in the worker's outbox.  The rows below 100 % fall into three
 * mechanical patterns, each measured and recorded rather than re-spelled:
 *
 *   * argument-evaluation order of `em_approach_start(self, 0, <f32>)` - the target schedules the `lfs` of
 *     the float argument *before* the `li r4,0`, this compiler (same flags) emits `li` first.  Alone
 *     it is the whole gap on fn_801D832C (94.59 %, 148 B = 148 B) and part of fn_801D80EC,
 *     fn_801D85C0, fn_801D8AC4, fn_801D8B70, fn_801DB100.  A local `f32 v = lbl_...;` before the call
 *     was measured and did not change the order.
 *   * the `u8` action argument's mask - the target opens the inner switch with `clrlwi r0,arg,24`
 *     plus `cmpwi r0,0`; `switch ((u8)arg)` here folds it to the record form `clrlwi. r0,arg,24`.
 *     Present in fn_801D80EC, fn_801D8688, fn_801DA8DC.  Widening the parameter to `u32` and
 *     switching on `arg & 0xFF` was measured: fn_801D8688 fell 89.89 -> 89.19 %, so the measured
 *     best shape (the `u8` parameter) is the one kept.
 *   * `fmadds` fusion - `a + b * c` fuses to one `fmadds` where the target has a separate
 *     `fmuls`/`fadds` pair (fn_801D8688's case 1).
 *
 * The two heaviest rows are fn_801D9918 (87.31 %, 940 B vs 972 B - the joint-matrix/intersection
 * effect body, one 0x28-byte frame slot short) and fn_801D9150 (87.92 %, 752 B vs 764 B - the
 * four-window state machine); both are otherwise faithful and are the next pass's first targets.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */
#include "stage/shell_set_func_ptr.h" /* `shell_set_func_ptr` and its slots (rule 2: the owner's header) */

/* ----------------------------------------------------------------------------------------------------
 * the pool the range reads (owned elsewhere; declared, never defined - playbook 29)
 * -------------------------------------------------------------------------------------------------- */

extern f32 lbl_807994F8;
extern f32 lbl_807994FC;
extern f32 lbl_80799500;
extern f32 lbl_80799504;
extern f32 lbl_8079950C;
extern f32 lbl_80799510;
extern f32 lbl_8079951C;
extern f32 lbl_80799520;
extern f32 lbl_80799524;
extern f32 lbl_80799528;
extern f32 lbl_8079952C;
extern f32 lbl_80799530;
extern f32 lbl_80799534;
extern f32 lbl_80799538;
extern f32 lbl_8079953C;
extern f32 lbl_80799540;
extern f32 lbl_80799544;
extern f32 lbl_80799548;
extern f32 lbl_8079954C;
extern f32 lbl_80799550;
extern f32 lbl_80799554;
extern f32 lbl_80799558;
extern f32 lbl_8079955C;
extern f32 lbl_80799560;
extern f32 lbl_80799564;
extern f32 lbl_80799568;
extern f32 lbl_8079956C;
extern f32 lbl_80799570;
extern f32 lbl_80799574;
extern f32 lbl_80799578;
extern f32 lbl_8079957C;
extern f32 lbl_80799580;
extern f32 lbl_80799584;
extern f32 lbl_80799588;
extern f32 lbl_8079958C;
extern f32 lbl_80799590;
extern f32 lbl_80799594;
extern f32 lbl_80799598;
extern f32 lbl_807995A8;
extern f32 lbl_807995AC;
extern f32 lbl_807995B0;
extern f32 lbl_807995B4;
extern f32 lbl_807995B8;
extern f32 lbl_807995BC;
extern f32 lbl_807995C0;
extern f32 lbl_807995C4;
extern f32 lbl_807995C8;
extern f32 lbl_807995CC;
extern f32 lbl_807995D0;
extern f32 lbl_807995D4;
extern f32 lbl_807995D8;
extern f32 lbl_807995DC;
extern f32 lbl_807995E0;
extern f32 lbl_807995E4;
extern f32 lbl_807995E8;
extern f32 lbl_807995EC;
extern f32 lbl_807995F0;
extern f32 lbl_807995F4;
extern f32 lbl_807995F8;
extern f32 lbl_807995FC;
extern f32 lbl_80799600;
extern f32 lbl_80799604;
extern f32 lbl_80799608;
extern f32 lbl_8079960C;
extern f32 lbl_80799610;
extern f32 lbl_80799614;
extern f32 lbl_80799618;
extern f32 lbl_8079961C;
extern f32 lbl_80799620;
extern f32 lbl_80799624;
extern f32 lbl_80799628;
extern f32 lbl_8079962C;
extern f32 lbl_80799630;
extern f32 lbl_80799634;
extern f32 lbl_80799638;
extern f32 lbl_80799648;

/* the range's own `.data` tables (no `.data` range is registered for this unit, so they stay the
 * shared pool's bytes; only the ones the code loads explicitly are declared). */
extern u8 lbl_80570500[];
extern u8 lbl_80570540[];
extern u8 lbl_80570580[];
extern u8 lbl_805705C0[];
extern u8 lbl_80570600[];
extern u8 lbl_80570640[];
extern u8 lbl_80570680[];
extern u8 lbl_805B6A90[];
extern u8 lbl_805B6AEC[];
extern u8 lbl_805B71A8[];
extern u8 lbl_805B6B88[];
extern u8 lbl_805B6BB0[];
extern u8 lbl_805B6BD8[];
extern u8 lbl_805B6C10[];
extern u8 lbl_805B6C50[];
extern u8 lbl_805B6C80[];
extern u8 lbl_805B6CD8[];
extern u8 lbl_805B6D48[];
extern u8 lbl_805B6D88[];
extern u8 lbl_805B6DB0[];
extern u8 lbl_805B6DD8[];
extern u8 lbl_805B6E00[];
extern u8 lbl_805B6E28[];
extern u8 lbl_805B6E50[];
extern u8 lbl_805B6E78[];
extern u8 lbl_805B6EB8[];
extern u8 lbl_805B6EE8[];
extern u8 lbl_805B6F10[];
extern u8 lbl_805B6F38[];
extern u8 lbl_805B6F60[];
extern u8 lbl_805B6F88[];
extern u8 lbl_805B6FB0[];
extern u8 lbl_805B6FD8[];
extern u8 lbl_805B7000[];
extern u8 lbl_805B7040[];
extern u8 lbl_805B70B0[];
extern u8 lbl_805B70F8[];
extern u8 lbl_805B7128[];
extern u8 lbl_805B7150[];
extern u8 lbl_805B7190[];

/* ----------------------------------------------------------------------------------------------------
 * the C++-mangled callees (rule 9: declared with the signature the mangling encodes, called through it)
 * -------------------------------------------------------------------------------------------------- */

s32 calcVecAng2(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
                                                /* calcVecAng2__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3 */
void rotVecY(nw4r::math::VEC3* v, u32 angle);   /* rotVecY__FPQ34nw4r4math4VEC3Ul */
void calcVecAngXY(nw4r::math::VEC3* v, u32* out1, s32* out2);
                                                /* calcVecAngXY__FPQ34nw4r4math4VEC3PUlPUl */
s32 findInterSection(nw4r::math::VEC3* a, nw4r::math::VEC3* b, nw4r::math::VEC3* c, u8 d, u32 e,
                     u8 f, u16 g, u8* h);
            /* findInterSection__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3PQ34nw4r4math4VEC3UcUlUcUsPUc */
void setVector3(nw4r::math::VEC3* v, f32 x, f32 y, f32 z);
void mulVecMat(nw4r::math::VEC3* v, nw4r::math::MTX34* m);
u32 em_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
                                                /* em_frame_check__FP11_ENEMY_WORKUsff */
f32 get_em_chg_scale(struct _ENEMY_WORK* self); /* get_em_chg_scale__FP11_ENEMY_WORK */
void get_joint_wpos_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::VEC3* out);
                                                /* get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3 */
void get_joint_wmat_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::MTX34* out);
                                                /* get_joint_wmat_em__FP11_ENEMY_WORKUlPQ34nw4r4math5MTX34 */
void* get_move_work_adrs(u8 index);             /* get_move_work_adrs__FUc */
u16 get_move_work_max(u8 index);                /* get_move_work_max__FUc */
void CancelFade(struct _ENEMY_WORK* self);      /* CancelFade */

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------------------------------------------
 * the enemy-band callees (the signatures are the call sites')
 * -------------------------------------------------------------------------------------------------- */

/* enemy/fn_801251D0.cpp (0x801251D0..0x8012BA00) */
void em_se_tbl_play(void* tbl, u32 a, u32 b);
void em_se_tbl_play_alt(void* tbl, u32 a, u32 b);
void em_part_rec_reset(struct _ENEMY_WORK* self, u32 a);
void em_part_rec_alt_set(struct _ENEMY_WORK* self, u32 a, u32 b);
u32 em_hit_mask_get(struct _ENEMY_WORK* self, f32 a);
void em_action_finish(struct _ENEMY_WORK* self);
void fn_801280F4(struct _ENEMY_WORK* self);
void em_state_set(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_target_pos_set(struct _ENEMY_WORK* self, void* p);
void em_hit_window_set(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void em_hit_window_set_default(struct _ENEMY_WORK* self, u32 a, u32 b);

/* enemy/fn_8012BDF4.cpp (0x8012BDF4..0x8012E968) */
void fn_8012B380(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_busy_set(struct _ENEMY_WORK* self);
u32 fn_8012D1A0(struct _ENEMY_WORK* self);
u32 fn_8012D23C(struct _ENEMY_WORK* self, u32 a, u8 b);
u8 fn_8012D3E0(struct _ENEMY_WORK* self, u32 a);

/* enemy/fn_8012E968.cpp (0x8012E968..0x8012EC74) */
u32 fn_8012EC3C(struct _ENEMY_WORK* self);
u32 em_alt_mode_ck(struct _ENEMY_WORK* self);

/* enemy/fn_8012EC74.cpp (0x8012EC74..0x80137604) */
void em_mot_set(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void em_mot_set_ck(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
f32 get_em_base_scale(struct _ENEMY_WORK* self);
f32 fn_8012F8F4(struct _ENEMY_WORK* self);
void em_mot_speed_set(struct _ENEMY_WORK* self, f32 a);
u32 em_mot_end_ck(struct _ENEMY_WORK* self);
void fn_8013032C(struct _ENEMY_WORK* self);
void fn_801303EC(struct _ENEMY_WORK* self);
void em_move_mode_set(struct _ENEMY_WORK* self, u32 a);
void fn_80130CDC(struct _ENEMY_WORK* self, u32 a);
void fn_80130CE8(struct _ENEMY_WORK* self, u32 a, u32 b, f32 c);
void em_busy_timer_reset(struct _ENEMY_WORK* self);
void fn_80131EC0(struct _ENEMY_WORK* self);
void em_turn_to_target(struct _ENEMY_WORK* self, u32 a);
void em_turn_in_window(struct _ENEMY_WORK* self, f32 lo, f32 hi, s32 angle);
void fn_80133F4C(struct _ENEMY_WORK* self, u32 a, f32 b, f32 c);
void em_approach_start(struct _ENEMY_WORK* self, f32 speed, u32 flags);
u32 em_approach_step(struct _ENEMY_WORK* self, u32 a, u32 b);
void em_turn_seq_start(struct _ENEMY_WORK* self, void* tbl, u32 a, u32 b, u32 c);
u32 em_turn_seq_step(struct _ENEMY_WORK* self, void* tbl);
void em_move_vec_clr(struct _ENEMY_WORK* self);
void em_move_vec2_clr(struct _ENEMY_WORK* self);
void em_move_offset_apply(struct _ENEMY_WORK* self);
void em_move_offset_rot_apply(struct _ENEMY_WORK* self, void* p);
void em_camera_req(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80136D14(struct _ENEMY_WORK* self);

/* enemy/fn_8011D448.cpp (0x8011D448..) - the damage/knockback helper */
void fn_8011E6EC(struct _ENEMY_WORK* self, s32 a, u32 b, f32 c, f32 d);

/* the neighbouring registered units' dispatchers */
void fn_801D791C(struct _ENEMY_WORK* self); /* enemy/fn_801D428C.cpp */
void fn_801D8078(struct _ENEMY_WORK* self); /* enemy/fn_801D428C.cpp */
void fn_801DFD3C(struct _ENEMY_WORK* self, void* p, u16* out, f32 a); /* enemy/fn_801DB8E0.cpp */
u32 fn_801E0058(struct _ENEMY_WORK* self, u32 a);                     /* enemy/fn_801DB8E0.cpp */

/* ef / ai / stage module helpers */
void subVec3(void* out, void* a, void* b);
f32 fn_80050EF4(void* a, void* b);
f32 calcVecDistXZ(void* a, void* b);
f32 fn_80050EAC(void* a, void* b);
void addVec3(void* out, void* a, void* b);
void fn_80051EE0(void* out, void* a, f32 b);
void addVec3To(void* a, void* b);
void eft007_part_spawn(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void em_spawn_rec_init(void* out);
void fn_802B43A8(void* pos, u8 a, u16 b);
void fn_802B954C(void);
void fn_802B9574(u32 a);
void eft_em_spawn(struct _ENEMY_WORK* self, u32 a, u32 b, void* pos, f32 c);
void fn_803B9BA0(struct _ENEMY_WORK* self, void* pos, u32 a);

#ifdef __cplusplus
}
#endif

/* ====================================================================================================
 * bodies
 * ================================================================================================== */

extern "C" void fn_801D80EC(struct _ENEMY_WORK* self, u32 arg1, u32 arg2) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xA, 4, 0);
        em_mot_speed_set(self, lbl_80799520);
        em_hit_window_set(self, 0, 0xC, 2);
        fn_80130CDC(self, (u32)-0x14);
        switch ((u8)arg1) {
        case 0: em_approach_start(self, lbl_80799524, 0); return;
        case 1: em_approach_start(self, lbl_80799528, 0); return;
        case 2: em_approach_start(self, lbl_807994FC, 0); return;
        case 3: em_approach_start(self, lbl_8079952C, 0); return;
        case 4: em_approach_start(self, lbl_80799530, 0); return;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            switch ((u8)arg2) {
            case 0:
                self->state = self->state + 1;
                em_mot_set(self, 0xB, 6, 0);
                return;
            case 1:
                em_action_finish(self);
                return;
            }
        } else {
            return;
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_801D82A0(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_80570500, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_80570500) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D832C(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x10, 0xA, 0);
        em_approach_start(self, lbl_807994FC, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D83C0(struct _ENEMY_WORK* self) {
    f32 temp_f31;
    s32 temp_r3;
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_80570580, 0, 1, 0);
        em_move_vec2_clr(self);
        temp_f31 = get_em_chg_scale(self);
        self->field_0x318 = lbl_80799534 * get_em_base_scale(self) * temp_f31;
        self->field_0x324 = lbl_807994FC;
        temp_r3 = calcVecAng2(&self->pos, &self->vec_0x36C);
        rotVecY(&self->offset_0x30C.vec_0x310, temp_r3);
        rotVecY(&self->vec_0x31C, temp_r3);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_807994FC, lbl_80799510) == 1) {
            CancelFade(self);
        }
        if (em_turn_seq_step(self, lbl_80570580) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D84E8(struct _ENEMY_WORK* self) {
    f32 temp_f1;
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x22, 2, 0);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_807994FC, lbl_80799538) == 1) {
            temp_f1 = (lbl_80799540 * (lbl_80799544 * get_em_base_scale(self))) / lbl_80799548;
            em_turn_to_target(self, (u16)(s32)(lbl_8079953C + temp_f1));
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D85C0(struct _ENEMY_WORK* self, u8 arg1) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, (u16)((((s32)~((arg1 - 1) | (1 - arg1)) >> 0x1F) + 0x25)), 2, 0);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_8079954C, lbl_80799550) == 1) {
            em_turn_to_target(self, 0x100);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D8688(struct _ENEMY_WORK* self, u8 arg1) {
    f32 var_f1;
    f32 temp_f31;
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        var_f1 = 0.0f;
        em_mot_set_ck(self, 0xA, 4, 0);
        switch (arg1) {
        case 0:
            var_f1 = lbl_80799554 * fn_80050EF4(&self->pos, &self->vec_0x36C);
            break;
        case 1:
            var_f1 = lbl_80799558 + (lbl_8079955C * get_em_chg_scale(self));
            break;
        }
        em_approach_start(self, var_f1, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0xB, 6, 0);
            em_approach_start(self, lbl_807994FC, 0x10);
            return;
        }
        return;
    case 2:
        em_approach_step(self, 0, 0x80);
        if ((em_mot_end_ck(self) == 1) ||
            (temp_f31 = lbl_80799560 * get_em_chg_scale(self),
             calcVecDistXZ(&self->pos, &self->vec_0x36C) < temp_f31)) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D87FC(struct _ENEMY_WORK* self) {
    f32 var_f1;
    f32 var_f2;
    s32 temp_r0;
    s32 var_r31;
    u8 temp_r4;

    var_r31 = 0;
    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        if ((s16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0) < 0) {
            var_f1 = lbl_80799500;
            var_f2 = lbl_80799564;
        } else {
            var_f1 = lbl_80799564;
            var_f2 = lbl_80799500;
        }
        temp_r0 = self->bits_0x1EC & 3;
        switch (temp_r0) {
        case 0:
            var_r31 = (s32)(u16)(s32)(lbl_8079953C + ((lbl_80799540 * var_f1) / lbl_80799548));
            break;
        case 1:
            var_r31 = -(s32)(u16)(s32)(lbl_8079953C + ((lbl_80799540 * var_f2) / lbl_80799548));
            break;
        }
        em_turn_seq_start(self, lbl_80570640, 0, 1, (u16)var_r31);
        return;
    case 1:
        em_turn_seq_step(self, lbl_80570640);
        if (em_frame_check(self, 1, lbl_80799568, lbl_807994FC) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D8950(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: fn_801D80EC(self, 0, 0); break;
    case 1: fn_801D80EC(self, 0, 1); break;
    case 2: fn_801D80EC(self, 1, 1); break;
    case 3: fn_801D80EC(self, 2, 1); break;
    case 4: fn_801D80EC(self, 3, 1); break;
    case 5: fn_801D80EC(self, 4, 1); break;
    case 6: fn_801D82A0(self); break;
    case 9: fn_801D832C(self); break;
    case 10: fn_801D83C0(self); break;
    case 11: fn_801D84E8(self); break;
    case 14: fn_801D85C0(self, 0); break;
    case 15: fn_801D85C0(self, 1); break;
    case 20: fn_801D8688(self, 0); break;
    case 21: fn_801D87FC(self); break;
    case 22: fn_801D8688(self, 1); break;
    }
}

extern "C" void fn_801D89F4(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x26, 4, 0);
        em_busy_set(self);
        em_busy_timer_reset(self);
        return;
    case 1:
        em_busy_set(self);
        em_busy_timer_reset(self);
        if (em_frame_check(self, 1, lbl_8079956C, lbl_807994FC) == 1) {
            fn_80136D14(self);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_8013032C(self);
            fn_801303EC(self);
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_801D8AC4(struct _ENEMY_WORK* self) {
    s32 temp_r0;
    u8 temp_r3;

    em_busy_set(self);
    em_busy_timer_reset(self);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        fn_8013032C(self);
        fn_801303EC(self);
        self->timer_0x020 = 0x5A;
        return;
    case 1:
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 == 0) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_801D8B70(struct _ENEMY_WORK* self) {
    u8 temp_r3;

    em_busy_set(self);
    em_busy_timer_reset(self);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xA, 4, 0);
        em_mot_speed_set(self, lbl_80799520);
        em_approach_start(self, lbl_807994FC, 0);
        fn_8013032C(self);
        fn_801303EC(self);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_801D8C34(struct _ENEMY_WORK* self) {
    u8 temp_r3;

    em_busy_set(self);
    em_busy_timer_reset(self);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 4);
        em_turn_seq_start(self, lbl_80570500, 0, 0, 0);
        fn_8013032C(self);
        fn_801303EC(self);
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_80570500) == 1) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_801D8CE4(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0x27, 0, 0);
        fn_801303EC(self);
        em_hit_window_set_default(self, 0, 0xA);
        em_busy_set(self);
        em_busy_timer_reset(self);
        fn_80136D14(self);
        return;
    case 1:
        em_busy_set(self);
        em_busy_timer_reset(self);
        if (em_frame_check(self, 2, lbl_80799570, lbl_807994FC) == 1) {
            fn_80136D14(self);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D8DC8(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: fn_801D89F4(self); return;
    case 1: fn_801D8AC4(self); return;
    case 3: fn_801D8B70(self); return;
    case 4: fn_801D8C34(self); return;
    case 5: fn_801D8CE4(self); return;
    }
}

extern "C" void fn_801D8E10(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_80570540, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_80570540) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D8E9C(struct _ENEMY_WORK* self) {
    f32 temp_f31;
    u8 temp_r3;
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x15, 4, 0);
        em_hit_window_set(self, 0, 2, 0x88);
        em_hit_window_set(self, 1, 0x34, 0x18);
        fn_80130CDC(self, (u32)-0x14);
        em_move_vec2_clr(self);
        temp_f31 = get_em_chg_scale(self);
        self->field_0x318 = lbl_80799574 * get_em_base_scale(self) * temp_f31;
        self->field_0x324 = lbl_807994FC;
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        rotVecY(&self->vec_0x31C, self->field_0x1C0);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_80799578, lbl_80799504) == 1) {
            CancelFade(self);
        }
        fn_80133F4C(self, 0x8000, lbl_80799578, lbl_80799504);
        temp_r3 = self->state_0x006;
        if ((temp_r3 == 0) && (self->field_0xA69 == 0)) {
            self->state_0x006 = temp_r3 + 1;
            em_hit_window_set(self, 1, 0x35, 0x10);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D9024(struct _ENEMY_WORK* self, u8 arg1) {
    u32 var_r5;
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 6, 0);
        var_r5 = 3;
        if (arg1 == 1) {
            var_r5 = 0x24;
        }
        em_hit_window_set(self, 0, var_r5, 0x88);
        em_hit_window_set(self, 1, 0x37, 0x18);
        fn_80130CDC(self, (u32)-0x14);
        return;
    case 1:
        if (em_frame_check(self, 0, lbl_80799578, lbl_807994FC) == 1) {
            em_hit_window_set(self, 1, 0x38, 0x18);
        }
        if (em_frame_check(self, 0, lbl_8079957C, lbl_807994FC) == 1) {
            em_hit_window_set(self, 1, 0x23, 0x10);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D9150(struct _ENEMY_WORK* self, u8 arg1) {
    u32 var_r5;
    u32 var_r5_2;
    u32 var_r5_3;
    s32 temp_r3_2;
    u32 temp_r30;
    u8 temp_r0;
    u8 temp_r3;
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x16, 2, 0);
        var_r5 = 4;
        if (arg1 == 1) {
            var_r5 = 0x25;
        }
        em_hit_window_set(self, 0, var_r5, 0x88);
        em_hit_window_set(self, 1, 0x39, 0x18);
        fn_80130CDC(self, (u32)-0x32);
        self->timer_0x020 = 0x3E8;
        em_approach_start(self, lbl_807994FC, 0);
        return;
    case 1:
        if (em_frame_check(self, 0, lbl_80799580, lbl_807994FC) == 1) {
            em_hit_window_set(self, 1, 0x20, 0x90);
        }
        if (em_frame_check(self, 0, lbl_80799584, lbl_807994FC) == 1) {
            var_r5_2 = 5;
            if (arg1 == 1) {
                var_r5_2 = 0x26;
            }
            em_hit_window_set(self, 0, var_r5_2, 0x88);
            em_hit_window_set(self, 1, 0x3A, 0x18);
        }
        if (em_frame_check(self, 0, lbl_80799588, lbl_807994FC) == 1) {
            em_hit_window_set(self, 1, 0x21, 0x90);
        }
        if (em_frame_check(self, 0, lbl_8079958C, lbl_807994FC) == 1) {
            var_r5_3 = 6;
            if (arg1 == 1) {
                var_r5_3 = 0x27;
            }
            em_hit_window_set(self, 0, var_r5_3, 0x88);
            em_hit_window_set(self, 1, 0x3B, 0x18);
        }
        if ((em_frame_check(self, 0, lbl_80799590, lbl_807994FC) == 1) ||
            (em_frame_check(self, 0, lbl_80799594, lbl_807994FC) == 1)) {
            temp_r3 = self->state_0x006;
            if (temp_r3 < 5) {
                temp_r0 = temp_r3 + 1;
                self->state_0x006 = temp_r0;
                em_mot_speed_set(self, lbl_807994F8 + (lbl_80799598 * (f32)temp_r0));
            }
        }
        temp_r3_2 = self->timer_0x020;
        if (temp_r3_2 > 0) {
            self->timer_0x020 = temp_r3_2 - 1;
        }
        temp_r30 = em_approach_step(self, 0, 0x100);
        if ((em_mot_end_ck(self) == 1) && ((temp_r30 == 1) || (self->timer_0x020 <= 0))) {
            self->state = self->state + 1;
            em_mot_set(self, 0x30, 2, 0);
            em_hit_window_set(self, 1, 0x22, 0x90);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_801D944C(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp1C;
    nw4r::math::VEC3 sp10;
    u32 spC;
    s32 sp8;
    u16 var_r3;
    u8 temp_r3;

    VEC3_ctor(&sp1C);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x17, 6, 0);
        em_hit_window_set(self, 0, 7, 0x80);
        fn_80130CDC(self, (u32)-0x14);
        subVec3(&sp10, &self->vec_0x36C, &self->pos);
        copyVec3(&sp1C, &sp10);
        calcVecAngXY(&sp1C, &spC, &sp8);
        var_r3 = sp8 - (u16)(self->field_0x1C0 + 0xD334);
        if ((u32)(var_r3 - 1) <= 0x7FFE) {
            if (var_r3 > 0xE39) {
                var_r3 = 0xE39;
            }
        } else {
            var_r3 = 0;
        }
        self->state_0x006 = (u8)((s32)var_r3 >> 8);
        return;
    case 1:
        em_turn_in_window(self, lbl_807995A8, lbl_807995AC, self->state_0x006 << 8);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D9584(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 sp1C;
    nw4r::math::VEC3 sp10;
    u32 spC;
    s32 sp8;
    u16 temp_r0;
    u32 var_r0;
    u8 temp_r3;

    VEC3_ctor(&sp1C);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x18, 6, 0);
        em_hit_window_set(self, 0, 8, 0x80);
        fn_80130CDC(self, (u32)-0x14);
        subVec3(&sp10, &self->vec_0x36C, &self->pos);
        copyVec3(&sp1C, &sp10);
        calcVecAngXY(&sp1C, &spC, &sp8);
        temp_r0 = sp8 - (u16)(self->field_0x1C0 + 0x2CCD);
        if ((s32)temp_r0 > 0x8000) {
            var_r0 = 0x10000 - temp_r0;
            if (var_r0 > 0xE39) {
                var_r0 = 0xE39;
            }
        } else {
            var_r0 = 0;
        }
        self->state_0x006 = (u8)((s32)var_r0 >> 8);
        return;
    case 1:
        em_turn_in_window(self, lbl_807995A8, lbl_807995AC, -(self->state_0x006 << 8));
        if ((arg1 == 1) && (em_frame_check(self, 0, lbl_80799504, lbl_807994FC) == 1)) {
            fn_803B9BA0(self, &self->pos, 0x64);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D9708(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 sp8;
    u8 temp_r3;

    VEC3_ctor(&sp8);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, (u16)(((arg1 - 1) == 0) + 0x2A), 6, 0);
        fn_80130CDC(self, (u32)-0x32);
        return;
    case 1:
        if ((em_frame_check(self, 3, lbl_807995B0, lbl_80799504) == 1) && ((system_w.field_0x0c & 1) == 0)) {
            eft007_part_spawn(self, 0x1B, 0xE39, 0);
        }
        if (em_frame_check(self, 0, lbl_807995B4, lbl_807994FC) == 1) {
            setVector3(&sp8, lbl_807994FC, lbl_807995B8, lbl_807994FC);
            shell_set_func_ptr->method_0x54(self, 2, 0x16, &sp8, shell_set_func_ptr);
        }
        if (em_frame_check(self, 0, lbl_807995BC, lbl_807994FC) == 1) {
            eft007_part_spawn(self, 0x1C, 0xE39, 0);
        }
        if ((em_frame_check(self, 3, lbl_807995C0, lbl_807995C4) == 1) && ((system_w.field_0x0c & 1) == 0)) {
            eft007_part_spawn(self, 0x1B, 0xE39, 0);
        }
        if ((em_frame_check(self, 3, lbl_807995C8, lbl_807995CC) == 1) && ((system_w.field_0x0c & 1) == 0)) {
            eft007_part_spawn(self, 0x1B, 0xE39, 0);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D9918(struct _ENEMY_WORK* self, u8 arg1, u8 arg2) {
    nw4r::math::MTX34 sp60;
    nw4r::math::VEC3 sp4C;
    nw4r::math::VEC3 sp38;
    nw4r::math::VEC3 sp2C;
    nw4r::math::VEC3 sp20;
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;
    s32 sp48;
    u16 sp5A;
    u8 sp58;
    u32 var_r4;
    u32 var_r28;
    u32 var_r29;
    u32 var_r27;
    u32 var_r31;
    s32 temp_r27;
    s32 temp_r29;
    f32 temp_f1;
    f32 temp_f1_2;
    u8 temp_r3;

    var_r31 = arg2;
    VEC3_ctor(&sp38);
    VEC3_ctor(&sp2C);
    VEC3_ctor(&sp20);
    VEC3_ctor(&sp14);
    MTX34_ctor(&sp60);
    em_spawn_rec_init(&sp48);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 0);
        switch (arg1) {
        default:
            var_r4 = 0x34;
            var_r28 = 0x1B;
            var_r29 = 0x33;
            break;
        case 1:
            var_r4 = 0x29;
            var_r28 = 0x1A;
            var_r29 = 0x32;
            break;
        case 2:
            var_r4 = 0x34;
            var_r28 = 0x31;
            var_r29 = 0x33;
            break;
        case 3:
            var_r4 = 0x29;
            var_r28 = 0x30;
            var_r29 = 0x32;
            break;
        }
        em_mot_set(self, var_r4, 2, 0);
        em_hit_window_set(self, 0, var_r28, 0x88);
        em_hit_window_set(self, 1, var_r29, 0x90);
        fn_80130CDC(self, (u32)-0x14);
        return;
    case 1:
        if (em_frame_check(self, 2, lbl_8079950C, lbl_807994FC) == 1) {
            em_turn_to_target(self, 0xC0);
        }
        if ((em_frame_check(self, 3, lbl_807995D0, lbl_807995D4) == 1) && ((system_w.field_0x0c & 3) == 0)) {
            setVector3(&sp38, lbl_807994FC, lbl_807994FC, lbl_807994FC);
            eft_em_spawn(self, 0x47, 0x16, &sp38, lbl_807994F8);
        }
        if (em_frame_check(self, 0, lbl_807995D8, lbl_807994FC) == 1) {
            sp48 = 0x17;
            copyVec3(&sp4C, setVec3(&sp8, lbl_807994FC, lbl_8079956C, lbl_80799568));
            copyVec3(&sp38, &sp4C);
            get_joint_wmat_em(self, sp48, &sp60);
            setVector3(&sp2C, sp60.m[0][3], sp60.m[1][3], sp60.m[2][3]);
            mulVecMat(&sp38, &sp60);
            addVec3To(&sp2C, &sp38);
            copyVec3(&sp14, &self->pos);
            temp_f1 = sp14.y;
            sp14.y = temp_f1 + lbl_8079950C;
            temp_r29 = findInterSection(&sp14, &sp2C, &sp20, 1, 0xFFFF, self->area_no,
                                        (u16)(em_hit_mask_get(self, temp_f1) | 0xC0), 0);
            temp_f1_2 = (lbl_807995DC + (self->vec_0x36C.y - sp2C.y)) / lbl_80799500;
            temp_r27 = -(s32)(lbl_80799538 * temp_f1_2);
            var_r27 = temp_r27 - (s32)(lbl_807995E4 *
                     ((calcVecDistXZ(&self->vec_0x36C, &sp2C) - lbl_807995E0) / lbl_8079950C));
            if ((s32)var_r27 < 0x8000) {
                if ((s32)var_r27 > 0x1000) {
                    var_r27 = 0x1000;
                }
            } else if ((s32)var_r27 < 0xF800) {
                var_r27 = 0xF800;
            }
            sp58 = 0;
            sp5A = (u16)var_r27;
            eft007_part_spawn(self, 0x24, var_r27, 0);
            switch (var_r31) {
            case 0:
                var_r31 = 0xD;
                break;
            case 1:
                var_r31 = 0x1D;
                break;
            case 2:
                var_r31 = 0x1E;
                break;
            }
            if (temp_r29 == 0) {
                shell_set_func_ptr->method_0x3C(self, &sp48, var_r31, shell_set_func_ptr);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801D9CE4(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;
    f32 var_f31;
    u8 temp_r3;

    VEC3_ctor(&sp14);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x19, 4, 0);
        em_mot_speed_set(self, lbl_80799520);
        em_hit_window_set(self, 0, 9, 0x88);
        fn_80130CDC(self, (u32)-0x14);
        em_target_pos_set(self, 0);
        var_f31 = calcVecDistXZ(&self->pos, &self->vec_0x36C) - lbl_807995E8;
        if (var_f31 > lbl_807995EC) {
            var_f31 = lbl_807995EC;
        } else if (var_f31 < lbl_807994FC) {
            var_f31 = lbl_807994FC;
        }
        em_move_vec_clr(self);
        self->field_0x318 = var_f31 / lbl_80799500;
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_807995A8, lbl_807995AC) == 1) {
            em_turn_to_target(self, 0x3A0);
        }
        if (em_frame_check(self, 3, lbl_807995F0, lbl_807995F4) == 1) {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_frame_check(self, 0, lbl_807995F8, lbl_807994FC) == 1) {
            em_camera_req(self, (u32)-1, 1);
            copyVec3(&sp14, setVec3(&sp8, lbl_807994FC, lbl_807994FC, lbl_807994FC));
            shell_set_func_ptr->method_0x2C(self, 5, 0x21, &sp14, lbl_807994F8, self->field_0xAEA,
                                           shell_set_func_ptr);
        }
        if ((em_frame_check(self, 1, lbl_807995FC, lbl_807994FC) == 1) && (fn_8012D1A0(self) == 1)) {
            if (fn_8012D3E0(self, 0) != 0xFF) {
                fn_8012B380(self, 1, 2);
                em_state_set(self, 8, 0);
                return;
            }
            em_state_set(self, 1, 6);
            return;
        }
        return;
    }
}

extern "C" void fn_801D9F2C(struct _ENEMY_WORK* self, u8 arg1) {
    u32 var_r5;
    f32 temp_f31;
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1C, 4, 0);
        var_r5 = 0x11;
        if (arg1 == 1) {
            var_r5 = 0x2A;
        }
        em_hit_window_set(self, 0, var_r5, 0x80);
        em_move_vec_clr(self);
        temp_f31 = get_em_chg_scale(self);
        self->field_0x318 = lbl_80799600 * get_em_base_scale(self) * temp_f31;
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_807994FC, lbl_80799604) == 1) {
            em_turn_to_target(self, 0x280);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801DA04C(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_805705C0, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_805705C0) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801DA0D8(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;
    u8 temp_r3;

    VEC3_ctor(&sp14);
    VEC3_ctor(&sp8);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1D, 4, 0);
        em_hit_window_set(self, 0, 1, 0x88);
        fn_80130CDC(self, (u32)-0x14);
        return;
    case 1:
        if (em_frame_check(self, 0, lbl_80799608, lbl_807994FC) == 1) {
            get_joint_wpos_em(self, 0x29, &sp14);
            shell_set_func_ptr->method_0x14(self, &sp14, 0x18, self->area_no, self->field_0xAEA,
                                           shell_set_func_ptr);
        }
        if (em_frame_check(self, 0, lbl_8079960C, lbl_807994FC) == 1) {
            em_camera_req(self, 0x29, 5);
            setVector3(&sp8, lbl_80799610, lbl_807994FC, lbl_807994FC);
            shell_set_func_ptr->method_0x2C(self, 9, 0x29, &sp8, lbl_80799614, self->field_0xAEA,
                                           shell_set_func_ptr);
            fn_802B43A8(&self->pos, self->area_no, self->bits_0x1EC);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801DA254(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_80570600, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_80570600) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801DA2E0(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: fn_801D8E10(self); break;
    case 1: fn_801D8E9C(self); break;
    case 2: fn_801D9024(self, 0); break;
    case 3: fn_801D9150(self, 0); break;
    case 4: fn_801D944C(self); break;
    case 5: fn_801D9584(self, 0); break;
    case 6: fn_801D9708(self, 0); break;
    case 7: fn_801D9708(self, 1); break;
    case 8: fn_801D9918(self, 0, 0); break;
    case 9: fn_801D9918(self, 1, 0); break;
    case 10: fn_801D9CE4(self); break;
    case 11: fn_801D9F2C(self, 0); break;
    case 12: fn_801DA04C(self); break;
    case 13: fn_801DA0D8(self); break;
    case 14: fn_801DA254(self); break;
    case 15: fn_801D8E10(self); break;
    case 16: fn_801D9F2C(self, 1); break;
    case 17: fn_801D9024(self, 1); break;
    case 18: fn_801D9150(self, 1); break;
    case 19: fn_801D9918(self, 2, 0); break;
    case 20: fn_801D9918(self, 3, 0); break;
    case 21: fn_801D9918(self, 0, 1); break;
    case 22: fn_801D9918(self, 1, 1); break;
    case 23: fn_801D9918(self, 2, 1); break;
    case 24: fn_801D9918(self, 3, 1); break;
    case 25: fn_801D9918(self, 0, 2); break;
    case 26: fn_801D9918(self, 1, 2); break;
    case 27: fn_801D9918(self, 2, 2); break;
    case 28: fn_801D9918(self, 3, 2); break;
    case 29: fn_801DA04C(self); break;
    case 30: fn_801DA254(self); break;
    case 31: fn_801D9584(self, 1); break;
    }
}

extern "C" s32 fn_801DA410(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 sp2C;
    nw4r::math::VEC3 sp20;
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;
    s32 temp_r31;
    u8* temp_r31_2;

    VEC3_ctor(&sp2C);
    VEC3_ctor(&sp20);
    temp_r31 = (s32)get_move_work_adrs(2);
    if ((s32)arg1 < (s32)get_move_work_max(2)) {
        temp_r31_2 = (u8*)(temp_r31 + (arg1 * 0xB20));
        if (*temp_r31_2 != 0) {
            setVector3(&sp2C, lbl_807994FC, lbl_807994FC, lbl_807994F8);
            rotVecY(&sp2C, self->field_0x1C0);
            fn_80051EE0(&sp8, &sp2C, lbl_80799618 * get_em_chg_scale(self));
            addVec3(&sp14, &self->pos, &sp8);
            copyVec3(&sp20, &sp14);
            if (fn_80050EAC(temp_r31_2 + 0x3C, &sp20) == lbl_8079961C) {
                return 1;
            }
        }
    }
    return 0;
}

extern "C" void fn_801DA514(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp3C;
    nw4r::math::VEC3 sp30;
    nw4r::math::VEC3 sp24;
    nw4r::math::VEC3 sp18;
    nw4r::math::VEC3 spC;
    u16 sp8;
    f32 temp_f31;
    s32 temp_r0;
    u8 temp_r3;

    VEC3_ctor(&sp3C);
    VEC3_ctor(&sp30);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        fn_801DFD3C(self, &sp3C, &sp8, lbl_80799620);
        subVec3(&sp24, &sp3C, &self->pos);
        copyVec3(&sp30, &sp24);
        em_target_pos_set(self, &sp3C);
        em_turn_seq_start(self, lbl_80570680, 2, 1, sp8);
        em_move_vec_clr(self);
        temp_f31 = get_em_base_scale(self);
        fn_80051EE0(&spC, &sp30, lbl_80799624);
        fn_80051EE0(&sp18, &spC, temp_f31);
        copyVec3(&self->offset_0x30C.vec_0x310, &sp18);
        self->field_0x314 = lbl_807994FC;
        return;
    case 1:
        fn_80131EC0(self);
        if (em_frame_check(self, 3, lbl_80799628, lbl_8079962C) == 1) {
            em_move_offset_apply(self);
            em_turn_seq_step(self, lbl_80570680);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            self->timer_0x020 = 0x5A;
            em_mot_set(self, 0x35, 2, 0);
            return;
        }
        return;
    case 2:
        fn_80131EC0(self);
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 < 0) {
            self->timer_0x020 = 0;
        }
        if (fn_8012D1A0(self) == 1) {
            if (fn_801DA410(self, self->field_0x382) == 1) {
                if (fn_8012D23C(self, 1, self->field_0x382) == 1) {
                    self->field_0x1E7 = 5;
                    if (fn_8012EC3C(self) == 1) {
                        em_state_set(self, 9, 3);
                        return;
                    }
                    em_state_set(self, 9, 1);
                    return;
                }
                if ((self->timer_0x020 <= 0) || (fn_8012D23C(self, 0, self->field_0x382) == 0)) {
                    em_state_set(self, 1, 7);
                    return;
                }
                return;
            }
            em_state_set(self, 1, 7);
            return;
        }
        return;
    }
}

extern "C" void fn_801DA514(struct _ENEMY_WORK* self);

extern "C" void fn_801DA7A0(struct _ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        fn_801DA514(self);
    }
}

extern "C" void fn_801DA7B4(struct _ENEMY_WORK* self) {
    u8 temp_r0;
    u8 temp_r3;

    fn_80131EC0(self);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x35, 4, 0);
        return;
    case 1:
        if (fn_8012D1A0(self) == 1) {
            if ((fn_8012D23C(self, 1, self->field_0x382) == 1) &&
                (fn_801DA410(self, self->field_0x382) == 1)) {
                temp_r0 = self->field_0x1E7 - 1;
                self->field_0x1E7 = temp_r0;
                if (temp_r0 == 0) {
                    em_state_set(self, 9, 2);
                    return;
                }
                if (fn_8012EC3C(self) == 1) {
                    em_state_set(self, 9, 3);
                    return;
                }
                em_state_set(self, 9, 1);
                return;
            }
            em_state_set(self, 1, 7);
        } else {
            return;
        }
        break;
    }
}

extern "C" void fn_801DA8DC(struct _ENEMY_WORK* self, u8 arg1) {
    u32 var_r5;
    f32 temp_f2;
    u8 temp_r0;
    u8 temp_r3;

    fn_80131EC0(self);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x32, 4, 0);
        var_r5 = 0x14;
        if (arg1 == 1) {
            var_r5 = 0x2D;
        }
        em_hit_window_set_default(self, 0, var_r5);
        return;
    case 1:
        temp_r0 = self->state_0x006;
        switch (temp_r0) {
        case 0:
            if (em_frame_check(self, 0, lbl_80799630, lbl_807994FC) == 1) {
                temp_f2 = (f32)self->field_0x7A4;
                fn_8011E6EC(self, (s32)(lbl_80799634 * temp_f2), 1, lbl_807994F8, temp_f2);
                fn_80130CE8(self, 0x32, 1, lbl_807994F8);
            }
            if (em_frame_check(self, 0, lbl_80799638, lbl_807994FC) == 1) {
                em_camera_req(self, 0x16, 7);
            }
            if (em_mot_end_ck(self) == 1) {
                self->state_0x006 = self->state_0x006 + 1;
                em_mot_set(self, 0x35, 4, 0);
            case 1:
                if (fn_8012D1A0(self) == 1) {
                    if ((fn_8012D23C(self, 1, self->field_0x382) == 1) &&
                        (fn_801DA410(self, self->field_0x382) == 1)) {
                        em_state_set(self, 9, 0);
                        return;
                    }
                    em_state_set(self, 1, 7);
                }
            } else {
                return;
            }
            break;
        }
        break;
    }
}

extern "C" void fn_801DAAC8(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x33, 2, 0);
        em_hit_window_set_default(self, 0, 0x19);
        fn_80131EC0(self);
        fn_802B954C();
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_80799648, lbl_807994FC) == 1) {
            fn_802B9574(0);
        }
        if (em_frame_check(self, 3, lbl_8079951C, lbl_807994FC) == 1) {
            fn_80131EC0(self);
        }
        if (em_frame_check(self, 0, lbl_8079951C, lbl_807994FC) == 1) {
            fn_803B9BA0(self, &self->pos, 0x64);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801DABD4(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: fn_801DA7B4(self); return;
    case 1: fn_801DA8DC(self, 0); return;
    case 2: fn_801DAAC8(self); return;
    case 3: fn_801DA8DC(self, 1); return;
    }
}

extern "C" void fn_801DAC18(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x17: em_se_tbl_play(lbl_805B6B88, 0, 0x17); return;
    case 0x18: em_se_tbl_play(lbl_805B6BD8, 0, 0x18); return;
    case 0x19: em_se_tbl_play(lbl_805B6BB0, 0, 0x19); return;
    case 0x1A: em_se_tbl_play(lbl_805B6BB0, 0, 0x1A); return;
    case 0x1B: em_se_tbl_play(lbl_805B6C10, 0, 0x1B); return;
    case 0x1C: em_se_tbl_play(lbl_805B6C50, 0, 0x1C); return;
    case 0x1D: em_se_tbl_play(lbl_805B6D88, 0, 0x1D); return;
    case 0x58: em_se_tbl_play(lbl_805B6C80, 0, 0x58); return;
    case 0x5C: em_se_tbl_play(lbl_805B6CD8, 0, 0x5C); return;
    case 0x5D: em_se_tbl_play(lbl_805B6D48, 0, 0x5D); return;
    case 0x7A: em_se_tbl_play(lbl_805B6DB0, 0, 0x7A); return;
    case 0x7B: em_se_tbl_play(lbl_805B6DD8, 0, 0x7B); return;
    case 0x9F: em_se_tbl_play(lbl_805B6E00, 0, 0x9F); return;
    case 0xA0: em_se_tbl_play(lbl_805B6E00, 0, 0xA0); return;
    case 0x7C: em_se_tbl_play(lbl_805B6E28, 0, 0x7C); return;
    case 0x8D: em_se_tbl_play(lbl_805B6E50, 0, 0x8D); return;
    case 0x7D: em_se_tbl_play(lbl_805B6EB8, 0, 0x7D); return;
    case 0x8E: em_se_tbl_play(lbl_805B6EE8, 0, 0x8E); return;
    case 0x78: em_se_tbl_play(lbl_805B6E78, 0, 0x78); return;
    case 0xA8: em_se_tbl_play(lbl_805B6F10, 0, 0xA8); return;
    case 0xB6: em_se_tbl_play(lbl_805B6F38, 0, 0xB6); return;
    case 0xB7: em_se_tbl_play(lbl_805B6F60, 0, 0xB7); return;
    case 0xB8: em_se_tbl_play(lbl_805B6F88, 0, 0xB8); return;
    case 0xB9: em_se_tbl_play(lbl_805B6FB0, 0, 0xB9); return;
    case 0xBA: em_se_tbl_play(lbl_805B6FD8, 0, 0xBA); return;
    case 0xBB: em_se_tbl_play(lbl_805B7000, 0, 0xBB); return;
    case 0xBC: em_se_tbl_play(lbl_805B7000, 0, 0xBC); return;
    case 0xBF: em_se_tbl_play(lbl_805B7040, 0, 0xBF); return;
    case 0xC1: em_se_tbl_play(lbl_805B6E50, 0, 0xC1); return;
    case 0xC9: em_se_tbl_play(lbl_805B70B0, 0, 0xC9); return;
    case 0xF0: em_se_tbl_play(lbl_805B6BD8, 0, 0xF0); return;
    default: em_action_finish(self); return;
    }
}

extern "C" void fn_801DAFC8(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: em_se_tbl_play_alt(lbl_805B70F8, 0, 0); return;
    case 15: em_se_tbl_play_alt(lbl_805B7128, 0, 0xF); return;
    case 26: em_se_tbl_play_alt(lbl_805B7190, 0, 0x1A); return;
    case 28: em_se_tbl_play_alt(lbl_805B7150, 0, 0x1C); return;
    default: em_se_tbl_play_alt(lbl_805B70F8, 0, 0); return;
    }
}

extern "C" void fn_801DB054(struct _ENEMY_WORK* self) {
    fn_801D8CE4(self);
}

extern "C" void fn_801DB058(struct _ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        fn_801DB054(self);
    }
}

extern "C" void fn_801DB06C(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_80570500, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_80570500) == 1) {
            em_state_set(self, 0xD, 1);
        }
        return;
    }
}

extern "C" void fn_801DB100(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xA, 4, 0);
        em_mot_speed_set(self, lbl_80799520);
        em_hit_window_set(self, 0, 0xC, 2);
        fn_80130CDC(self, (u32)-0x14);
        em_approach_start(self, lbl_80799528, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_state_set(self, 0xD, 2);
        }
        return;
    }
}

extern "C" void fn_801DB1C8(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;
    f32 var_f31;
    u8 temp_r3;

    VEC3_ctor(&sp14);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x19, 4, 0);
        em_hit_window_set(self, 0, 0x1D, 8);
        em_hit_window_set(self, 1, 0x36, 0x110);
        em_target_pos_set(self, 0);
        var_f31 = calcVecDistXZ(&self->pos, &self->vec_0x36C) - lbl_807995E8;
        if (var_f31 > lbl_807995EC) {
            var_f31 = lbl_807995EC;
        } else if (var_f31 < lbl_807994FC) {
            var_f31 = lbl_807994FC;
        }
        em_move_vec_clr(self);
        self->field_0x318 = var_f31 / lbl_80799628;
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_807995A8, lbl_807995AC) == 1) {
            em_turn_to_target(self, 0x300);
        }
        if (em_frame_check(self, 3, lbl_807995F0, lbl_807995F4) == 1) {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_frame_check(self, 0, lbl_807995F8, lbl_807994FC) == 1) {
            em_camera_req(self, (u32)-1, 1);
            copyVec3(&sp14, setVec3(&sp8, lbl_807994FC, lbl_807994FC, lbl_807994FC));
            shell_set_func_ptr->method_0x2C(self, 5, 0x21, &sp14, lbl_807994F8, self->field_0xAEA,
                                           shell_set_func_ptr);
        }
        if ((em_frame_check(self, 1, lbl_807995FC, lbl_807994FC) == 1) && (fn_8012D1A0(self) == 1)) {
            if (fn_801E0058(self, 0) == 1) {
                em_state_set(self, 0xD, 3);
                return;
            }
            em_state_set(self, 1, 6);
            return;
        }
        return;
    }
}

extern "C" void fn_801DB3F8(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp30;
    nw4r::math::VEC3 sp24;
    nw4r::math::VEC3 sp18;
    nw4r::math::VEC3 spC;
    u16 sp8;
    u8 temp_r3;

    VEC3_ctor(&sp30);
    VEC3_ctor(&sp24);
    temp_r3 = self->state;
    switch (temp_r3) {
    case 0:
        self->state = temp_r3 + 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        fn_801DFD3C(self, &sp30, &sp8, lbl_80799618);
        subVec3(&sp18, &sp30, &self->pos);
        copyVec3(&sp24, &sp18);
        em_target_pos_set(self, &sp30);
        em_turn_seq_start(self, lbl_80570680, 2, 1, sp8);
        em_move_vec_clr(self);
        fn_80051EE0(&spC, &sp24, lbl_807994F8 / fn_8012F8F4(self));
        copyVec3(&self->offset_0x30C.vec_0x310, &spC);
        self->field_0x314 = lbl_807994FC;
        return;
    case 1:
        em_move_offset_apply(self);
        em_turn_seq_step(self, lbl_80570680);
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x35, 2, 0);
            return;
        }
        return;
    case 2:
        if (fn_8012D1A0(self) == 1) {
            if (fn_801E0058(self, 1) == 1) {
                em_state_set(self, 0xD, 4);
                return;
            }
            em_state_set(self, 1, 7);
        }
        break;
    }
}

extern "C" void fn_801DB594(struct _ENEMY_WORK* self) {
    u8 temp_r0;
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x32, 4, 0);
        fn_803B9BA0(self, &self->pos, 0x64);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            temp_r0 = self->state_0x006 + 1;
            self->state_0x006 = temp_r0;
            if (temp_r0 >= 3) {
                em_state_set(self, 0xD, 5);
            }
        }
        return;
    }
}

extern "C" void fn_801DB648(struct _ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch (temp_r4) {
    case 0:
        self->state = temp_r4 + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x33, 2, 0);
        fn_80130CDC(self, 0x3E8);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801DB6D0(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: fn_801DB06C(self); return;
    case 1: fn_801DB100(self); return;
    case 2: fn_801DB1C8(self); return;
    case 3: fn_801DB3F8(self); return;
    case 4: fn_801DB594(self); return;
    case 5: fn_801DB648(self); return;
    }
}

extern "C" void fn_801DB724(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0: fn_801D791C(self); break;
    case 1: fn_801D8078(self); break;
    case 2: fn_801D8950(self); break;
    case 6: fn_801D8DC8(self); break;
    case 7: fn_801DA2E0(self); break;
    case 8: fn_801DA7A0(self); break;
    case 9: fn_801DABD4(self); break;
    case 10: fn_801DAC18(self); break;
    case 11: fn_801DAFC8(self); break;
    case 12: fn_801DB058(self); break;
    case 13: fn_801DB6D0(self); break;
    }
    if (self->field_0x1E2 == 4) {
        em_busy_timer_reset(self);
    }
}

extern "C" void fn_801DB7D8(struct _ENEMY_WORK* self) {
    if (em_alt_mode_ck(self) == 1) {
        if (self->action_0x328.field_0x344 == 0) {
            em_part_rec_alt_set(self, 0, 0);
            em_part_rec_alt_set(self, 1, 1);
            em_part_rec_alt_set(self, 2, 2);
            em_part_rec_alt_set(self, 3, 3);
            em_part_rec_alt_set(self, 4, 4);
            em_part_rec_alt_set(self, 5, 5);
            self->action_0x328.field_0x344 = 1;
        }
    } else if (self->action_0x328.field_0x344 == 1) {
        em_part_rec_reset(self, 0);
        em_part_rec_reset(self, 1);
        em_part_rec_reset(self, 2);
        em_part_rec_reset(self, 3);
        em_part_rec_reset(self, 4);
        em_part_rec_reset(self, 5);
        self->action_0x328.field_0x344 = 0;
    }
}
