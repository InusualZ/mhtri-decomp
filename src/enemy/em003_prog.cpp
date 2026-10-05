/* enemy/em003_prog.cpp - enemy 003 program
 *
 * `.text` 0x80154E40..0x8015D860, 68 functions written (the rest of the range is not decompiled yet).
 * Renamed from `fn_801550FC`: the unit's `.data` holds `em003_prog_tbl` (0x805A44A8) and its `.text` starts at 0x80154E40.
 * Each function keeps the `#pragma` state it had in its retired source. The retired header's notes follow below.
 */

/* Retired header of `enemy/fn_801550FC.cpp` (kept for its notes and residuals): */
/* enemy/fn_801550FC.cpp - the em003 enemy's action/state unit, 0x80154E40..0x8015D860 (108 functions).
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with `dumpmap.py
 * lookup`: every function of the range reports `zz_<addr>_` in the shared runtime dump and no
 * `__FILE__`/class string names it, and `config/RMHE08/symbols.txt` carries only the bare
 * `fn_XXXXXXXX` entries for the range - the three exceptions are the C++ mangled definitions listed
 * below, which are declared and defined through their real signatures).
 *
 * What it is.  The per-action work functions of one `enemy`-module actor, the `em003` enemy: three of
 * the range's own definitions are C++ manglings (`em003_call_act_type_ck__FP11_ENEMY_WORK`,
 * `em003_atama_koware_act_ck__FP11_ENEMY_WORK`, `em003_yobi_boss_em_ck__FUc`), and the range drives
 * the shared enemy work record (`_ENEMY_WORK`) through the enemy-band helpers
 * (`em_frame_check`, `em_act_ck`, `get_joint_wpos_em`, `get_em_chg_scale`, ...).  The bodies are
 * per-action state steps: frame/timer gates on the work record, distance and angle tests against the
 * player, `em_act_ck`/`em_frame_check` gates, motion requests and effect/part requests.
 *
 * The range's left edge is settled from evidence (2026-09-30 recut): `fn_80154D44` is the static-initializer
 * of the unit below (its `.ctors` word points at it), the 0.0 pool entry is duplicated at
 * `lbl_807970C0` from `fn_80154E90` on (one TU pools a value once), and the em003 data chunk
 * (`lbl_805A4518` .. the `jumptable_805A4848` of `fn_80154FAC` and the `lbl_805A5D18` vtable `fn_80154F70`
 * installs) is read from 0x80154E90 upwards - so the unit starts at 0x80154E40, the four functions
 * 0x80154E40..0x801550FC came from `enemy/fn_801502C8.cpp`.  The upper end is bounded by the em008 chunk
 * taking over (the next queue entry starts 0x8015E854).
 *
 * Language: C++ (the range defines three C++-mangled symbols; `langcheck.py` agrees).  The flat
 * `fn_*` symbols are `extern "C"` so objdiff pairs them by name, and the mangled callees are declared
 * and called through their real signatures (rule 9).
 *
 * Object: `_ENEMY_WORK`, included from `enemy.h` (the union of every consumer's copy).  Name
 * evidence: the mangled callee `em_frame_check__FP11_ENEMY_WORKUsff` carries the type name.  The
 * fields this unit walks were added to that header with their offsets (+0x331, the +0x332 union, the
 * +0x334..+0x336 bytes, +0x338, +0x33C, +0xA0D); the +0x332 union keeps the signed-short reading
 * `enemy/fn_80176C58.cpp` uses, because `fn_8015D194` clears the byte at +0x333 on its own.
 *
 * Flags: this unit needs no deviation - the `enemy` lib's `cflags_main` (see the block's comment in
 * `configure.py`) measured every body below.  No `#pragma` is used.
 *
 * State of the reconstruction.  The 62 functions the bodies below cover are the range's state steps and
 * their dispatchers, in address order; the large per-action functions (`fn_801550FC`,
 * `fn_80155BA8`, `fn_8015B0CC` and the rest of the range's biggest bodies) are NOT written yet - they
 * are recorded as the residual, with the two functions that the m2c draft could not round-trip
 * (`fn_8015C890`, `fn_8015D140`) and the four the compiler rejected outright.
 *
 * Residuals, per shape (the measurement lives in the outbox, not here; playbook rows in
 * `docs/matching.md`):
 *   * `fn_80156920`/`fn_80156B28` are the two-step motion start-up of the range: retail places the
 *     `(a1 & 0xFF) == 1` and `== 2` bodies out of line and the `else` inline, this source emits the
 *     inverted nesting MWCC orders inline-first.  A plain `switch` measures worse, not better; the
 *     difference is instruction order inside the same 240-byte body.
 *   * the biggest bodies (`fn_801550FC` and up) need the per-action record fields this unit reaches
 *     through the helpers (motion ids, effect/part requests) and are the next pass's work.
 *   * `fn_80154E90` (90.98 %, recut 2026-09-30): retail keeps `clrlwi r0,r31,24`+`cmpwi` for the `arg1 != 0` test
 *     (this unit's peephole folds it) and loads the two `em_motion_param_set` float arguments one call later; the
 *     instruction multiset is otherwise the same.  `fn_80154F70` (99.33 %) picks `r3` where retail picks `r0`
 *     for the table address.
 *   * nothing here is a paired-single (`psq_l`/`psq_st`) residual: the range has no such instruction
 *     (checked with the disassembly), so the stopping rule of `docs/matching.md` does not apply.
 */

#include "enemy/em_mot_finished_ck.h" /* em_mot_finished_ck (rule 2: the owner's header) */
#include "enemy/em_motion_param_set.h" /* em_motion_param_set (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "enemy.h"
#include "unsplit/enemy.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "ef/fn_80105314.h"
#include "sys_mem.h"
#include "fn_8004CAD8.h"
#include "mh3_pad.h"
/* the call sites use the argument-less view: a cast call is the same direct call. */
#define em_mot_finished_ck_c1 ((u32 (*)(void))em_mot_finished_ck)

/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */

/* the C++-mangled callees (rule 9: real signature, outside `extern "C"`) */
extern s32 calcVecAng2(VEC3*, VEC3*);
extern u32 em003_atama_koware_act_ck(_ENEMY_WORK*);

extern u32 em_frame_check(_ENEMY_WORK*, u16, f32, f32);
extern void rotVecY(VEC3*, u32);

extern "C" {
extern void fn_803B9BA0(_ENEMY_WORK*, u32, u32);
extern void fn_8043B410(_ENEMY_WORK*);
extern void fn_8043B424(_ENEMY_WORK*, u32, u32);
extern "C" void fn_80154E40(_ENEMY_WORK* self);
extern "C" void fn_80154E90(_ENEMY_WORK* self, u8 arg1);
/* untyped: opaque handle - the 0x0C-byte effect helper (the constructor returns its `this`) */
extern "C" void** fn_80154F70(void** self);
extern "C" void fn_80154FAC(_ENEMY_WORK* self, u8* arg1, u8* arg2);

extern "C" void fn_80155664(_ENEMY_WORK* self);
extern "C" void fn_8015569C(_ENEMY_WORK* self);
extern "C" void fn_801556E8(_ENEMY_WORK* self);
extern "C" void fn_80155770(_ENEMY_WORK* self);
extern "C" void fn_801557EC(_ENEMY_WORK* self);
extern "C" void fn_80155868(_ENEMY_WORK* self);
extern "C" void fn_801558E8(_ENEMY_WORK* self);
extern "C" void fn_80155964(_ENEMY_WORK* self);
extern "C" void fn_801559B8(_ENEMY_WORK* self);
extern "C" void fn_80155A34(_ENEMY_WORK* self);
extern "C" void fn_80155AB0(_ENEMY_WORK* self);
extern "C" void fn_80155B2C(_ENEMY_WORK* self);
extern "C" void fn_80155BA8(_ENEMY_WORK* self, u8 arg1);
extern "C" void fn_80155DD8(_ENEMY_WORK* self);
extern "C" void fn_80155E58(_ENEMY_WORK* self);
extern "C" void fn_80155F74(_ENEMY_WORK* self);
extern "C" void fn_801560B8(_ENEMY_WORK* self, u8 arg1);
extern "C" void fn_80156258(_ENEMY_WORK* self, u8 arg1);
extern "C" void fn_801564DC(_ENEMY_WORK* self, u8 a1);
extern "C" void fn_801565B4(_ENEMY_WORK* self);
extern "C" void fn_8015668C(_ENEMY_WORK* self);
extern "C" void fn_80156708(_ENEMY_WORK* self);
extern "C" void fn_80156784(_ENEMY_WORK* self);
extern "C" void fn_80156800(_ENEMY_WORK* self);
extern "C" void fn_80156920(_ENEMY_WORK* self, s32 a1);
extern "C" void fn_80156A10(_ENEMY_WORK* self);
extern "C" void fn_80156A9C(_ENEMY_WORK* self);
extern "C" void fn_80156B28(_ENEMY_WORK* self, s32 a1);
extern "C" void fn_80156C18(_ENEMY_WORK* self, u8 arg1);
extern "C" void fn_80156E9C(_ENEMY_WORK* self, u32 a1);
extern "C" void fn_80156EA4(_ENEMY_WORK* self);
extern "C" void fn_80156FEC(_ENEMY_WORK* self);
extern "C" void fn_80157178(_ENEMY_WORK* self);
extern "C" void fn_801571F0(_ENEMY_WORK* self);
extern "C" void fn_80157330(_ENEMY_WORK* self);
extern "C" void fn_80157424(_ENEMY_WORK* self);
extern "C" void fn_8015757C(_ENEMY_WORK* self);
extern "C" void fn_80157650(_ENEMY_WORK* self);
extern "C" void fn_80157764(_ENEMY_WORK* self, u8 arg1);
extern "C" void fn_80157960(_ENEMY_WORK* self, u8 arg1);
extern "C" void fn_80157ABC(_ENEMY_WORK* self);
extern "C" void fn_80157B40(_ENEMY_WORK* self);
extern "C" void fn_80157EC4(_ENEMY_WORK* self);
extern "C" void fn_80157F44(_ENEMY_WORK* self);
extern "C" void fn_80157FD0(_ENEMY_WORK* self);
extern "C" void fn_801580E8(_ENEMY_WORK* self);
extern "C" void fn_801581F0(_ENEMY_WORK* self);
extern "C" void fn_80158264(_ENEMY_WORK* self);
extern "C" void fn_80158324(_ENEMY_WORK* self);
extern "C" void fn_801583E4(_ENEMY_WORK* self);
extern "C" void fn_80158500(_ENEMY_WORK* self);
extern "C" void fn_801585DC(_ENEMY_WORK* self);
extern "C" void fn_80158668(_ENEMY_WORK* self, u8 a1);
extern "C" void fn_80158764(_ENEMY_WORK* self);
extern "C" void fn_80158820(_ENEMY_WORK* self);
extern "C" void fn_80158928(_ENEMY_WORK* self, u8 a1);
extern "C" void fn_80158A50(_ENEMY_WORK* self);
extern "C" void fn_80158B4C(_ENEMY_WORK* self);
extern "C" void fn_80158BBC(_ENEMY_WORK* self);
extern "C" void fn_80158C48(_ENEMY_WORK* self);
extern "C" void fn_80158E24(_ENEMY_WORK* self, u8 arg1);
extern "C" void fn_80158FF0(_ENEMY_WORK* self, u8 arg1);
extern "C" void fn_801592CC(_ENEMY_WORK* self);
extern "C" void fn_8015941C(_ENEMY_WORK* self);
extern "C" void fn_80159540(_ENEMY_WORK* self, u8 arg1, u8 arg2, u8 arg3);
extern "C" void fn_801596F8(_ENEMY_WORK* self, u8 arg1);
extern "C" void fn_80159984(_ENEMY_WORK* self);
extern "C" void fn_80159AB8(_ENEMY_WORK* self, u8 arg1, u8 arg2, u8 arg3, u32 a);
extern "C" void fn_80159DD8(_ENEMY_WORK* self);
extern "C" void fn_80159E54(_ENEMY_WORK* self, u8 arg1, u8 arg2);
extern "C" void fn_80159FF0(_ENEMY_WORK* self, u8 arg1);
extern "C" void fn_8015A1BC(_ENEMY_WORK* self);
extern "C" void fn_8015A344(_ENEMY_WORK* self);
extern "C" void fn_8015A604(_ENEMY_WORK* self);
extern "C" void fn_8015A6AC(_ENEMY_WORK* self);
extern "C" void fn_8015A6B0(_ENEMY_WORK* self);
extern "C" void fn_8015A6C4(_ENEMY_WORK* self);
extern "C" void fn_8015AB08(_ENEMY_WORK* self);
extern "C" void fn_8015ABAC(_ENEMY_WORK* self);
extern "C" void fn_8015ABD0(_ENEMY_WORK* self);

extern "C" void fn_8015D194(_ENEMY_WORK* self);
extern "C" s32 fn_8015D1B0(_ENEMY_WORK* self, u8 a1);
extern "C" s32 fn_8015D1F0(_ENEMY_WORK* self);
extern "C" void fn_8015D200(_ENEMY_WORK* self);
}

extern u8 lbl_8056FA38[];
extern u8 lbl_8056FA78[];
extern u8 lbl_8056FAB8[];
extern u8 lbl_8056FB24[];
extern u8 lbl_805A4568[];
extern u8 lbl_805A5D18[];
extern u8 lbl_805A4590[];
extern u8 lbl_805A45F0[];
extern u8 lbl_805A55D8[];
extern u8 lbl_805A5638[];
extern u8 lbl_805A5688[];
extern u8 lbl_805A56D0[];
extern u8 lbl_805A5700[];
extern f32 lbl_807970C0;
extern f32 lbl_807970C4;
extern f32 lbl_807970C8;
extern f32 lbl_807970CC;
extern f32 lbl_807970D8;
extern f32 lbl_807970FC;
extern f32 lbl_80797100;
extern f32 lbl_80797104;
extern f32 lbl_80797124;
extern f32 lbl_80797128;
extern f32 lbl_8079712C;
extern f32 lbl_80797130;
extern f32 lbl_80797134;
extern f32 lbl_80797138;
extern f32 lbl_8079713C;
extern f32 lbl_80797140;
extern f32 lbl_80797144;
extern f32 lbl_80797148;
extern f32 lbl_8079714C;
extern f32 lbl_80797150;
extern f32 lbl_80797154;
extern f32 lbl_80797158;
extern f32 lbl_8079715C;
extern f32 lbl_80797160;
extern f32 lbl_80797164;
extern f32 lbl_80797168;
extern f32 lbl_8079717C;
extern f32 lbl_80797180;
extern f32 lbl_80797184;
extern f32 lbl_80797188;
extern f32 lbl_8079718C;
extern f32 lbl_80797198;
extern f32 lbl_807971A4;
extern f32 lbl_807971A8;
extern f32 lbl_807971AC;
extern f32 lbl_807971B0;
extern f32 lbl_807971B4;
extern f32 lbl_807971B8;
extern f32 lbl_807971BC;
extern f32 lbl_807971C0;
extern f32 lbl_807971D4;
extern f32 lbl_807971DC;
extern f32 lbl_807971F0;
extern f32 lbl_807971F4;
extern f32 lbl_807971F8;
extern f32 lbl_80797260;
extern f32 lbl_80797264;
extern f32 lbl_80797268;

/* Resets the effect slot set: the first six slot bytes to 0xFF, the rest of the set and both timers to 0. */
extern "C" void fn_80154E40(_ENEMY_WORK* self) {
    self->init_0x320.bytes_0x328[0] = 0xFF;
    self->init_0x320.bytes_0x328[1] = 0xFF;
    self->init_0x320.bytes_0x32A[0] = 0xFF;
    self->init_0x320.bytes_0x32A[1] = 0xFF;
    self->bytes_0x32C.field_0x32C = 0xFF;
    self->bytes_0x32C.field_0x32D = 0xFF;
    self->field_0x32E = 0;
    self->field_0x32F = 0;
    self->field_0x330 = 0;
    self->field_0x331 = 0;
    self->bytes_0x332.field_0x332 = 0;
    self->timer_0x338 = 0;
    self->bytes_0x332.field_0x333 = 0;
    self->field_0x334 = 0;
    self->field_0x335 = 0;
    self->field_0x336 = 0;
    self->counter_0x33C = 0;
}

/* Attaches the 0x0C-byte effect helper and arms the 0x1C72 effect at the work position. */
extern "C" void fn_80154E90(_ENEMY_WORK* self, u8 arg1) {
    VEC3 pos;
    s32 helper;

    VEC3_ctor(&pos);
    if (arg1 != 0) {
        em_motion_param_set(self, 0, lbl_807970C0);
        em_motion_param_set(self, 0xA, lbl_807970C4);
    }
    if (em_res_user_data_ck(self) == 0) {
        helper = (s32)operator new(0xC);
        if (helper != 0) {
            fn_80154F70((void**)helper);
        }
        em_res_user_data_set(self, (void*)helper);
    }
    if (self->state_0x009 == 0) {
        setVector3(&pos, lbl_807970C0, lbl_807970C8, lbl_807970CC);
        fn_801057A4(self, 0x1A, &pos, lbl_807970C4, 0x1C72);
    }
    self->flags_0x836 = (u16)(self->flags_0x836 | 0x8000);
}

/* Constructs the 0x0C-byte effect helper and installs its `lbl_805A5D18` table. */
/* untyped: opaque handle - the 0x0C-byte effect helper (the constructor returns its `this`) */
extern "C" void** fn_80154F70(void** self) {
    em_res_user_data_ctor(self);
    *self = (void*)lbl_805A5D18;
    return self;
}

/* Remaps the effect id when the action is past its first step. */
extern "C" void fn_80154FAC(_ENEMY_WORK* self, u8* arg1, u8* arg2) {
    switch (*arg1) {
    case 1:
        if (em_mot_finished_ck_c1() == 1 || self->field_0x7c8 >= 0x29) {
            switch (*arg2) {
            case 8:
                *arg2 = 0x19;
                return;
            case 13:
                *arg2 = 0x1A;
                return;
            case 14:
                *arg2 = 0x1B;
                return;
            case 15:
                *arg2 = 0x1C;
                return;
            case 16:
                *arg2 = 0x1D;
                return;
            case 17:
                *arg2 = 0x1E;
                return;
            case 33:
                *arg2 = 0x23;
                return;
            case 34:
                *arg2 = 0x24;
                return;
            }
        }
        return;
    case 2:
        switch (*arg2) {
        case 0:
            if (em_mot_finished_ck_c1() == 1) {
                *arg2 = 3;
            }
            return;
        case 8:
            if (em_mot_finished_ck_c1() == 1) {
                *arg2 = 9;
            }
            return;
        case 10:
            if (em_mot_finished_ck_c1() == 1) {
                *arg2 = 0xB;
            }
            return;
        }
        return;
    }
}

extern "C" void fn_80155664(_ENEMY_WORK* self) {
    s16 temp_r4;
    u16 temp_r0;

    temp_r0 = self->counter_0x33C + 1;
    self->counter_0x33C = temp_r0;
    if (temp_r0 >= 0x32U) {
        self->counter_0x33C = 0U;
    }
    temp_r4 = self->timer_0x338;
    if (temp_r4 > 0) {
        self->timer_0x338 = (s16) (temp_r4 - 1);
    }
}

extern "C" void fn_8015569C(_ENEMY_WORK* self) {
    em_fall_height_get(self);
    em_fall_start(self);
    fn_80128AAC(self, 3, 0xB);
    fn_80133BB4(self);
}

extern "C" void fn_801556E8(_ENEMY_WORK* self) {
    s32 temp_r0;
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        self->field_0x20 = 0x96;
        em_mot_set_ck(self, 1, 0xA, 0);
        return;
    case 1:
        temp_r0 = self->field_0x20 - 1;
        self->field_0x20 = temp_r0;
        if (temp_r0 < 0) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80155770(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 0xA, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801557EC(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x11, 6, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80155868(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1A, 6, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_fall(self);
        }
        return;
    }
}

extern "C" void fn_801558E8(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 3);
        em_mot_set_ck(self, 0x1B, 6, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_walk(self);
        }
        return;
    }
}

extern "C" void fn_80155964(_ENEMY_WORK* self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch ((s32) temp_r0) {                        /* irregular */
    case 0:
        fn_801556E8(self);
        return;
    case 1:
        fn_80155770(self);
        return;
    case 2:
        fn_801557EC(self);
        return;
    case 3:
        fn_80155868(self);
        return;
    case 4:
        fn_801556E8(self);
        return;
    case 6:
        fn_801558E8(self);
        return;
    }
}

extern "C" void fn_801559B8(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 5, 0xA, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80155A34(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x12, 0xA, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80155AB0(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x7C, 2, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80155B2C(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xDD, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80155DD8(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 0x14, 8, 0, 2);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80155E58(_ENEMY_WORK* self) {
    s32 temp_r0;
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x17, 6, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state_0x05 = (u8) (self->state_0x05 + 1);
            em_mot_set(self, 0x18, 0, 0);
            self->field_0x20 = 0x708;
            fn_80132224(self);
            return;
        }
        return;
    case 2:
        fn_8013221C(self, lbl_807970FC, 1, 0xF);
        temp_r0 = self->field_0x20 - 1;
        self->field_0x20 = temp_r0;
        if (temp_r0 <= 0) {
            self->state_0x05 = (u8) (self->state_0x05 + 1);
            em_mot_set(self, 0x19, 4, 0);
            fn_80132264(self);
            return;
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_80155F74(_ENEMY_WORK* self) {
    s32 temp_r0;
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        self->phase_0x06 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 4, 0);
        if (em003_atama_koware_act_ck(self) == 1U) {
            self->field_0x20 = 0x12C;
        } else {
            self->field_0x20 = 0x96;
        }
        if ((u8) self->field_0x7c8 >= 0x29U) {
            em_mot_speed_set(self, lbl_80797100 * (lbl_80797104 / get_em_base_scale(self)));
            self->field_0x20 = (s32) ((f32) self->field_0x20 / lbl_80797104);
        }
        fn_8043B410(self);
        return;
    case 1:
        fn_8043B424(self, self->field_0x334, (u8) ((em003_atama_koware_act_ck(self) - 1) == 0));
        temp_r0 = self->field_0x20 - 1;
        self->field_0x20 = temp_r0;
        if (temp_r0 <= 0) {
            em_state_set(self, 1, 0x15);
        }
        return;
    }
}

extern "C" void fn_801564DC(_ENEMY_WORK* self, u8 a1) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* switch 1; irregular */
    case 0:                                         /* switch 1 */
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        switch ((s32) a1) {                       /* switch 2; irregular */
        case 0:                                     /* switch 2 */
            em_mot_set(self, 0xE6, 4, 0);
            return;
        case 1:                                     /* switch 2 */
            em_mot_set(self, 0xE7, 6, 0);
            return;
        case 2:                                     /* switch 2 */
            em_mot_set(self, 0xE8, 6, 0);
            return;
        }
        break;
    case 1:                                         /* switch 1 */
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801565B4(_ENEMY_WORK* self) {
    u8 temp_r3;

    em_busy_set(self);
    temp_r3 = self->state_0x05;
    switch ((s32) temp_r3) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCC, 0, 0);
        return;
    case 1:
        em_turn_in_window(self, lbl_80797124, lbl_80797128, -0x4000);
        em_turn_in_window(self, lbl_8079712C, lbl_80797130, -0x4000);
        em_turn_in_window(self, lbl_80797134, lbl_80797138, -0x4000);
        em_turn_in_window(self, lbl_80797138, lbl_8079713C, -0x4000);
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8015668C(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC9, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80156708(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xDC, 6, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80156784(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x11, 6, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80156800(_ENEMY_WORK* self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch (temp_r0) {
    case 0:
        fn_801559B8(self);
        return;
    case 1:
        fn_80155A34(self);
        return;
    case 2:
        fn_80155AB0(self);
        return;
    case 3:
        fn_80155B2C(self);
        return;
    case 4:
        fn_80155BA8(self, 0);
        return;
    case 5:
        fn_80155DD8(self);
        return;
    case 6:
        fn_80155E58(self);
        return;
    case 7:
        fn_80155F74(self);
        return;
    case 8:
        fn_801560B8(self, 0);
        return;
    case 9:
        fn_80156258(self, 0);
        return;
    case 10:
        fn_801564DC(self, 0);
        return;
    case 11:
        fn_801564DC(self, 1);
        return;
    case 12:
        fn_801564DC(self, 2);
        return;
    case 13:
        fn_801560B8(self, 0);
        return;
    case 14:
        fn_801560B8(self, 0);
        return;
    case 15:
        fn_801560B8(self, 0);
        return;
    case 16:
        fn_801560B8(self, 0);
        return;
    case 17:
        fn_801560B8(self, 0);
        return;
    case 18:
        fn_801565B4(self);
        return;
    case 19:
        fn_80156258(self, 1);
        return;
    case 20:
        fn_80156258(self, 2);
        return;
    case 21:
        fn_8015668C(self);
        return;
    case 22:
        fn_80156708(self);
        return;
    case 23:
        fn_80155F74(self);
        return;
    case 24:
        fn_80155BA8(self, 1);
        return;
    case 25:
        fn_801560B8(self, 1);
        return;
    case 26:
        fn_801560B8(self, 1);
        return;
    case 27:
        fn_801560B8(self, 1);
        return;
    case 28:
        fn_801560B8(self, 1);
        return;
    case 29:
        fn_801560B8(self, 1);
        return;
    case 30:
        fn_801560B8(self, 1);
        return;
    case 31:
        fn_80155F74(self);
        return;
    case 32:
        fn_80155F74(self);
        return;
    case 33:
        fn_801560B8(self, 0);
        return;
    case 34:
        fn_801560B8(self, 0);
        return;
    case 35:
        fn_801560B8(self, 1);
        return;
    case 36:
        fn_801560B8(self, 1);
        return;
    case 37:
        fn_80156784(self);
        return;
    default:
        return;
    }
}

extern "C" void fn_80156920(_ENEMY_WORK* self, s32 a1) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        if ((a1 & 0xFF) != 1) {
            if ((a1 & 0xFF) != 2) {
                em_approach_start(self, lbl_80797140, 0);
            } else {
                em_approach_start(self, lbl_807970C0, 0);
                if (self->field_0x378 > lbl_80797144) {
                    self->field_0x378 = (f32) lbl_80797144;
                }
            }
        } else {
            em_approach_start(self, lbl_807970C0, 0);
        }
        em_mot_set(self, 6, 6, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80156A10(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, &lbl_8056FA38, 0, 0, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, &lbl_8056FA38) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80156A9C(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, &lbl_8056FA78, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, &lbl_8056FA78) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80156B28(_ENEMY_WORK* self, s32 a1) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        if ((a1 & 0xFF) != 1) {
            if ((a1 & 0xFF) != 2) {
                em_approach_start(self, lbl_80797140, 0);
            } else {
                em_approach_start(self, lbl_807970C0, 0);
                if (self->field_0x378 > lbl_80797144) {
                    self->field_0x378 = (f32) lbl_80797144;
                }
            }
        } else {
            em_approach_start(self, lbl_807970C0, 0);
        }
        em_mot_set(self, 7, 0xA, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80156E9C(_ENEMY_WORK* self, u32 a1) {
    fn_80156C18(self, (u8) a1);
}

extern "C" void fn_80156EA4(_ENEMY_WORK* self) {
    s32 temp_r31;
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_approach_start(self, lbl_807970C0, 0);
        em_mot_set(self, 0xD0, 6, 0);
        return;
    case 1:
        temp_r31 = self->pos_0x1BC.y;
        if (em_approach_step(self, 0, 0x100) == 1U) {
            self->state_0x05 = (u8) (self->state_0x05 + 1);
            em_mot_set(self, 0xDA, 6, 0);
            return;
        }
        fn_8012F860(self, (lbl_80797148 * (f32) (s16) (self->pos_0x1BC.y - temp_r31)) / lbl_8079714C, lbl_80797150);
        fn_8012F7D4(self, 0xD4, 0xD5, (s32) fn_8012F8EC(self), self->frames_0x454[4]);
        return;
    case 2:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80156FEC(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCF, 0, 0);
        em_move_vec2_clr(self);
        self->v_0x310.z = (f32) (lbl_80797154 * get_em_base_scale(self));
        rotVecY(&self->v_0x310, self->pos_0x1BC.y);
        return;
    case 1:
        em_turn_in_window(self, lbl_80797158, lbl_80797134, 0x4000);
        em_turn_in_window(self, lbl_80797134, lbl_807970D8, 2.2959e-41f);
        if (em_frame_check(self, 1e-45f, lbl_8079715C, lbl_807970C0) == 1U) {
            em_move_offset_apply(self);
        }
        if (em_mot_end_ck(self) == 1U) {
            self->state_0x05 = (u8) (self->state_0x05 + 1);
            em_mot_set(self, 0xDB, 0, 0);
            return;
        }
        return;
    case 2:
        if (em_frame_check(self, NULL, lbl_80797160, lbl_807970C0) == 1U) {
            fn_803B9BA0(self, (u32) &self->pos, 0x64);
        }
        if (em_mot_end_ck(self) == 1U) {
            self->state_0x05 = (u8) (self->state_0x05 + 1);
            em_mot_set(self, 0x6B, 4, 0);
            return;
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_80157178(_ENEMY_WORK* self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch (temp_r0) {
    case 0:
        fn_80156920(self, 0);
        return;
    case 1:
        fn_80156A10(self);
        return;
    case 2:
        fn_80156A9C(self);
        return;
    case 3:
        fn_80156B28(self, 0);
        return;
    case 4:
        fn_80156E9C(self, 0);
        return;
    case 5:
        fn_80156E9C(self, 1);
        return;
    case 6:
        fn_80156EA4(self);
        return;
    case 7:
        fn_80156FEC(self);
        return;
    case 8:
        fn_80156920(self, 1);
        return;
    case 9:
        fn_80156B28(self, 1);
        return;
    case 10:
        fn_80156920(self, 2);
        return;
    case 11:
        fn_80156B28(self, 2);
        return;
    default:
        return;
    }
}

extern "C" void fn_801571F0(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x29, 6, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state_0x05 = (u8) (self->state_0x05 + 1);
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0x43, 0, 0);
            em_move_vec_clr(self);
            return;
        }
        return;
    case 2:
        self->v_0x310.y = em_key_curve_eval(self, &lbl_805A45F0);
        em_fall_height_get(self);
        fn_80135584(self, &self->pos_0x1BC);
        if (em_mot_end_ck(self) == 1U) {
            self->state_0x05 = (u8) (self->state_0x05 + 1);
            em_mot_set(self, 0x1A, 6, 0);
            return;
        }
        break;
    case 3:
        em_turn_to_target(self, 0x100);
        if (em_mot_end_ck(self) == 1U) {
            fn_80128A70(self, 3, 2);
        }
        break;
    }
}

extern "C" void fn_80157330(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x29, 6, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state_0x05 = (u8) (self->state_0x05 + 1);
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0x43, 0, 0);
            em_move_vec_clr(self);
            return;
        }
        return;
    case 2:
        self->v_0x310.y = em_key_curve_eval(self, &lbl_805A45F0);
        em_fall_height_get(self);
        fn_80135584(self, &self->pos_0x1BC);
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_fall(self);
        }
        break;
    }
}

extern "C" void fn_80157424(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 6, 0);
        em_dive_start(self);
        return;
    case 1:
        em_dive_step(self);
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1U) {
            self->state_0x05 = (u8) (self->state_0x05 + 1);
            em_move_mode_set(self, 3);
            em_mot_set(self, 0x1D, 6, 0);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1U) {
            self->state_0x05 = (u8) (self->state_0x05 + 1);
            em_mot_set(self, 0x2E, 6, 0);
            return;
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1U) {
            self->state_0x05 = (u8) (self->state_0x05 + 1);
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x41, 0, 0);
            return;
        }
        break;
    case 4:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_8015757C(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 6, 0);
        em_approach_start(self, lbl_807970C0, 0);
        em_move_vec2_clr(self);
        self->v_0x310.z = (f32) lbl_80797164;
        self->v_0x320.y = (f32) lbl_807970FC;
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1U) {
            em_action_finish_fall(self);
            return;
        }
        em_move_offset_step(self, &self->pos_0x1BC);
        if (self->v_0x310.z > lbl_80797158) {
            self->v_0x310.z = (f32) lbl_80797158;
        }
        return;
    }
}

extern "C" void fn_80157650(_ENEMY_WORK* self) {
    f32 var_f1;
    u16 temp_r0;
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_fall_height_get(self);
        em_fall_start(self);
        fn_8012F810(self);
        em_mot_set(self, 0x1F, 0, 0);
        return;
    case 1:
        em_target_pos_set(self, 0);
        {
            u16 ang = (u16) calcVecAng2(&self->pos, &self->target) - self->pos_0x1BC.y;

            if (ang != 0) {
                var_f1 = ((lbl_80797148 * (f32) (s16) ang) / lbl_8079714C) / lbl_80797158;
            } else {
                var_f1 = lbl_807970C0;
            }
        }
        fn_8012F860(self, var_f1, lbl_80797168);
        fn_8012F7D4(self, 0x23, 0x24, (s32) fn_8012F8EC(self), self->frames_0x454[4]);
        return;
    }
}

extern "C" void fn_80157ABC(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 6, 0);
        return;
    case 1:
        if (em_turn_to_target(self, 0x200) == 1U) {
            em_action_finish_fall(self);
        }
        return;
    }
}

extern "C" void fn_80157EC4(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x2D, 6, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_fall(self);
        }
        return;
    }
}

extern "C" void fn_80157F44(_ENEMY_WORK* self) {
    s32 temp_r0;
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1F, 4, 0);
        self->field_0x20 = 0x3C;
        return;
    case 1:
        temp_r0 = self->field_0x20 - 1;
        self->field_0x20 = temp_r0;
        if (temp_r0 <= 0) {
            fn_8015569C(self);
        }
        return;
    }
}

extern "C" void fn_80157FD0(_ENEMY_WORK* self) {
    u8 temp_r0;
    u8 temp_r3;

    em_busy_set(self);
    temp_r3 = self->state_0x05;
    switch ((s32) temp_r3) {                        /* switch 1; irregular */
    case 0:                                         /* switch 1 */
        self->state_0x05 = (u8) (temp_r3 + 1);
        self->phase_0x06 = 0U;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x3D, 6, 0);
        return;
    case 1:                                         /* switch 1 */
        temp_r0 = self->phase_0x06;
        switch ((s32) temp_r0) {                    /* switch 2; irregular */
        case 0:                                     /* switch 2 */
            if (em_frame_check(self, 1, lbl_8079718C, lbl_807970C0) == 1U) {
                self->phase_0x06 = (u8) (self->phase_0x06 + 1);
                em_move_vec2_clr(self);
                self->v_0x310.z = (f32) lbl_80797180;
                self->v_0x320.y = (f32) lbl_80797184;
            }
            break;
        case 1:                                     /* switch 2 */
            em_fall_height_get(self);
            em_move_offset_step_update(self, &self->pos_0x1BC);
            if (self->v_0x310.z > lbl_80797198) {
                self->v_0x310.z = (f32) lbl_80797198;
            }
            break;
        }
        if (em_mot_end_ck(self) == 1U) {
            fn_8015569C(self);
        }
        return;
    }
}

extern "C" void fn_801581F0(_ENEMY_WORK* self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch (temp_r0) {
    case 0:
        fn_801571F0(self);
        return;
    case 1:
        fn_80157330(self);
        return;
    case 2:
        fn_80157424(self);
        return;
    case 3:
        fn_8015757C(self);
        return;
    case 4:
        fn_80157650(self);
        return;
    case 5:
        fn_80157764(self, 0);
        return;
    case 6:
        fn_80157960(self, 0);
        return;
    case 7:
        fn_80157ABC(self);
        return;
    case 8:
        fn_80157B40(self);
        return;
    case 9:
        fn_80157764(self, 1);
        return;
    case 10:
        fn_80157EC4(self);
        return;
    case 11:
        fn_80157F44(self);
        return;
    case 12:
        fn_80157FD0(self);
        return;
    case 13:
        fn_80157960(self, 1);
        return;
    case 14:
        fn_801580E8(self);
        return;
    default:
        return;
    }
}

extern "C" void fn_80158264(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, NULL);
        em_mot_set(self, 0x2A, 6, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state_0x05 = (u8) (self->state_0x05 + 1);
            em_move_mode_set(self, 3);
            em_mot_set(self, 0x44, 0, 0);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_walk(self);
        }
        break;
    }
}

extern "C" void fn_80158324(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x2E, 6, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state_0x05 = (u8) (self->state_0x05 + 1);
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x41, 0, 0);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_801583E4(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x1E, 6, 0);
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_80797164, lbl_807970C0) == 1U) {
            self->state_0x05 = (u8) (self->state_0x05 + 1);
            em_fall_height_get(self);
            em_fall_start(self);
            em_move_vec2_clr(self);
            self->v_0x310.y = (f32) (lbl_80797180 * get_em_base_scale(self));
            self->v_0x320.x = (f32) (lbl_807971A4 * get_em_base_scale(self));
            return;
        }
        return;
    case 2:
        if (em_frame_check(self, 1, lbl_807971A8, lbl_807970C0) == 0) {
            em_move_offset_apply(self);
        } else {
            CancelFade(self);
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_fall(self);
        }
        break;
    }
}

extern "C" void fn_80158500(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 6, 0);
        em_dive_start(self);
        return;
    case 1:
        em_dive_step(self);
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1U) {
            self->state_0x05 = (u8) (self->state_0x05 + 1);
            em_move_mode_set(self, 3);
            em_mot_set(self, 0x1D, 6, 0);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_walk(self);
        }
        break;
    }
}

extern "C" void fn_801585DC(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 3);
        em_turn_seq_start(self, &lbl_8056FAB8, 0, 0, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, &lbl_8056FAB8) == 1U) {
            em_action_finish_walk(self);
        }
        return;
    }
}

extern "C" void fn_80158668(_ENEMY_WORK* self, u8 a1) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x1B, 6, 0);
        if ((s32) a1 != 1) {
            em_approach_start(self, lbl_80797188, 0);
        } else {
            em_approach_start(self, lbl_807971AC, 0);
        }
        em_move_vec2_clr(self);
        self->v_0x310.z = (f32) lbl_807971B0;
        self->v_0x320.y = (f32) lbl_807970C4;
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1U) {
            em_action_finish_walk(self);
            return;
        }
        em_move_offset_step(self, &self->pos_0x1BC);
        if (self->v_0x310.z > lbl_8079718C) {
            self->v_0x310.z = (f32) lbl_8079718C;
        }
        return;
    }
}

extern "C" void fn_80158764(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 3);
        fn_80134F70(self, &lbl_8056FB24);
        em_approach_start(self, lbl_807970C0, 9);
        return;
    case 1:
        if (em_approach_step(self, 0, 0) == 1U) {
            em_action_finish_walk(self);
            return;
        }
        fn_80135000(self, 0, &lbl_8056FB24);
        em_fall_height_get(self);
        fn_80135584(self, &self->pos_0x1BC);
        return;
    }
}

extern "C" void fn_80158928(_ENEMY_WORK* self, u8 a1) {
    s32 temp_r0;
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x1B, 6, 0);
        em_move_vec2_clr(self);
        if ((u32) (a1 - 1) > 1U) {
            if ((s32) a1 == 0) {
                self->v_0x310.z = (f32) lbl_807971B4;
                self->v_0x320.y = (f32) lbl_807971B8;
                self->field_0x20 = 0x16;
                return;
            }
            return;
        }
        self->v_0x310.z = (f32) lbl_807971BC;
        self->v_0x320.y = (f32) lbl_807971C0;
        self->field_0x20 = 0x1E;
        return;
    case 1:
        em_move_offset_step(self, &self->pos_0x1BC);
        if (self->v_0x310.z > lbl_807970C0) {
            self->v_0x310.z = (f32) lbl_807970C0;
        }
        temp_r0 = self->field_0x20 - 1;
        self->field_0x20 = temp_r0;
        if (temp_r0 <= 0) {
            if (((s32) a1 == 0) || ((s32) a1 == 2)) {
                fn_80128A70(self, 4, 1);
                return;
            }
            em_action_finish_walk(self);
        }
        break;
    }
}

extern "C" void fn_80158B4C(_ENEMY_WORK* self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch (temp_r0) {
    case 0:
        fn_80158264(self);
        return;
    case 1:
        fn_80158324(self);
        return;
    case 2:
        fn_801583E4(self);
        return;
    case 3:
        fn_80158500(self);
        return;
    case 4:
        fn_801585DC(self);
        return;
    case 5:
        fn_80158668(self, 0);
        return;
    case 6:
        fn_80158764(self);
        return;
    case 7:
        fn_80158820(self);
        return;
    case 8:
        fn_80158928(self, 0);
        return;
    case 9:
        fn_80158928(self, 1);
        return;
    case 10:
        fn_80158A50(self);
        return;
    case 11:
        fn_80158928(self, 2);
        return;
    case 12:
        fn_80158668(self, 1);
        return;
    default:
        return;
    }
}

extern "C" void fn_80158BBC(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x2F, 4, 0);
        em_hit_window_set_default(self, 0, 1);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801592CC(_ENEMY_WORK* self) {
    u8 temp_r3;
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* switch 1; irregular */
    case 0:                                         /* switch 1 */
        self->state_0x05 = (u8) (temp_r4 + 1);
        self->phase_0x06 = 0U;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xDE, 4, 0);
        em_hit_window_set_default(self, 0, 6);
        return;
    case 1:                                         /* switch 1 */
        if (em_frame_check(self, 1, lbl_807971F0, lbl_807970C0) == 0) {
            em_turn_to_target(self, 0x60);
        }
        temp_r3 = self->phase_0x06;
        switch ((s32) temp_r3) {                    /* switch 2; irregular */
        case 0:                                     /* switch 2 */
            if ((s32) self->field_0xA0D == 0) {
                self->phase_0x06 = (u8) (temp_r3 + 1);
                em_hit_window_set_default(self, 0, 7);
            }
            break;
        case 1:                                     /* switch 2 */
            if ((s32) self->field_0xA0D == 0) {
                self->phase_0x06 = (u8) (temp_r3 + 1);
                em_hit_window_set_default(self, 0, 8);
            }
            break;
        case 2:                                     /* switch 2 */
            if ((s32) self->field_0xA0D == 0) {
                self->phase_0x06 = (u8) (temp_r3 + 1);
                em_hit_window_set_default(self, 0, 9);
            }
            break;
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8015941C(_ENEMY_WORK* self) {
    f32 temp_f1;
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 3);
        fn_80130CDC(self, -0xC);
        em_mot_set(self, 0x36, 2, 0);
        em_hit_window_set_default(self, 0, 5);
        em_move_vec_clr(self);
        temp_f1 = fn_80050EF4(&self->pos, &self->target);
        if (temp_f1 > lbl_8079717C) {
            self->v_0x310.z = (f32) lbl_807971F4;
            return;
        }
        if (temp_f1 > lbl_807971D4) {
            self->v_0x310.z = (f32) ((lbl_807971F4 * temp_f1) / lbl_807971F8);
            return;
        }
        self->v_0x310.z = (f32) lbl_807970C0;
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_80797158, lbl_807970C0) == 0) {
            em_move_offset_rot_apply(self, &self->pos_0x1BC);
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_walk(self);
        }
        return;
    }
}

extern "C" void fn_80159984(_ENEMY_WORK* self) {
    f32 temp_f1;
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 3);
        fn_80130CDC(self, -0xC);
        em_mot_set(self, 0x38, 6, 0);
        em_move_vec2_clr(self);
        fn_801303EC(self, lbl_807970C0);
        em_hit_window_set_default(self, 0, 2);
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_807970CC, lbl_807970C0) == 0) {
            em_turn_to_target(self, 0x100);
        }
        fn_801303FC(self, em_key_curve_eval(self, &lbl_805A4568));
        temp_f1 = self->field_0x1ac;
        if (temp_f1 > lbl_807970C0) {
            self->field_0x1ac = (f32) lbl_807970C0;
        } else if (temp_f1 < lbl_807971DC) {
            self->field_0x1ac = (f32) lbl_807971DC;
        }
        self->v_0x310.z = em_key_curve_eval(self, &lbl_805A4590);
        em_move_offset_rot_apply(self, &self->pos_0x1BC);
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_walk(self);
        }
        return;
    }
}

extern "C" void fn_80159DD8(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE1, 2, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8015A1BC(_ENEMY_WORK* self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch (temp_r0) {
    case 0:
        fn_80158BBC(self);
        return;
    case 1:
        fn_80158C48(self);
        return;
    case 2:
        fn_80158E24(self, 0);
        return;
    case 3:
        fn_80158FF0(self, 0);
        return;
    case 4:
        fn_80158FF0(self, 1);
        return;
    case 5:
        fn_80158FF0(self, 2);
        return;
    case 6:
        fn_80158FF0(self, 3);
        return;
    case 7:
        fn_801592CC(self);
        return;
    case 8:
        fn_8015941C(self);
        return;
    case 9:
        fn_80159540(self, 0, 0, 0);
        return;
    case 10:
        fn_801596F8(self, 0);
        return;
    case 11:
        fn_801596F8(self, 1);
        return;
    case 12:
        fn_80159984(self);
        return;
    case 13:
        fn_80159AB8(self, 0, 0, 0, 0);
        return;
    case 14:
        fn_80159AB8(self, 0, 1, 0, 0);
        return;
    case 15:
        fn_80159DD8(self);
        return;
    case 16:
        fn_801596F8(self, 2);
        return;
    case 17:
        fn_80158E24(self, 1);
        return;
    case 18:
        fn_80158FF0(self, 4);
        return;
    case 19:
        fn_80159E54(self, 0, 0);
        return;
    case 20:
        fn_80159FF0(self, 0);
        return;
    case 21:
        fn_80159FF0(self, 1);
        return;
    case 22:
        fn_80158FF0(self, 0);
        return;
    case 23:
        fn_80159540(self, 1, 0, 0);
        return;
    case 24:
        fn_80159540(self, 1, 1, 0);
        return;
    case 25:
        fn_80159AB8(self, 0, 0, 1, 0);
        return;
    case 26:
        fn_80159AB8(self, 0, 1, 1, 0);
        return;
    case 27:
        fn_80159540(self, 1, 0, 1);
        return;
    case 28:
        fn_80159540(self, 2, 0, 1);
        return;
    case 29:
        fn_80159AB8(self, 0, 0, 0, 1);
        return;
    case 30:
        fn_80159AB8(self, 0, 1, 0, 1);
        return;
    case 31:
        fn_80159E54(self, 1, 0);
        return;
    case 32:
        fn_80159E54(self, 0, 1);
        return;
    case 33:
        fn_80159E54(self, 1, 1);
        return;
    default:
        return;
    }
}

extern "C" void fn_8015A604(_ENEMY_WORK* self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch ((s32) temp_r0) {                        /* irregular */
    case 0:
        em_se_tbl_play_alt(self, &lbl_805A55D8, 0, 0);
        return;
    case 5:
        em_se_tbl_play_alt(self, &lbl_805A5638, 1, 5);
        return;
    case 15:
        em_se_tbl_play_alt(self, &lbl_805A5688, 0, 0xF);
        return;
    case 26:
        em_se_tbl_play_alt(self, &lbl_805A56D0, 0, 0x1A);
        return;
    case 28:
        em_se_tbl_play_alt(self, &lbl_805A5700, 0, 0x1C);
        return;
    default:
        em_se_tbl_play_alt(self, &lbl_805A55D8, 0, 0);
        return;
    }
}

extern "C" void fn_8015A6AC(_ENEMY_WORK* self) {
    fn_80157424(self);
}

extern "C" void fn_8015A6B0(_ENEMY_WORK* self) {
    if ((s32) self->state_sub == 0) {
        fn_8015A6AC(self);
    }
}

extern "C" void fn_8015AB08(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_0x05;
    switch ((s32) temp_r4) {                        /* irregular */
    case 0:
        self->state_0x05 = (u8) (temp_r4 + 1);
        em_fall_height_get(self);
        em_fall_start(self);
        setVector3(&self->pos, lbl_80797260, lbl_80797264, lbl_80797268);
        copyVec3(&self->prev_pos, &self->pos);
        self->pos_0x1BC.x = 0;
        self->pos_0x1BC.y = 0xC44;
        self->pos_0x1BC.z = 0;
        em_mot_set(self, 0x20, 0, 0);
        return;
    case 1:
        fn_8015569C(self);
        return;
    }
}

extern "C" void fn_8015ABAC(_ENEMY_WORK* self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch ((s32) temp_r0) {                        /* irregular */
    case 0:
        fn_8015A6C4(self);
        return;
    case 1:
        fn_8015AB08(self);
        return;
    }
}

extern "C" void fn_8015ABD0(_ENEMY_WORK* self) {
    u8 temp_r0;

    temp_r0 = self->action_0x1E5;
    switch (temp_r0) {
    case 0:
        fn_80155964(self);
        return;
    case 1:
        fn_80156800(self);
        return;
    case 2:
        fn_80157178(self);
        return;
    case 3:
        fn_801581F0(self);
        return;
    case 4:
        fn_80158B4C(self);
        return;
    case 7:
        fn_8015A1BC(self);
        return;
    case 10:
        fn_8015A344(self);
        return;
    case 11:
        fn_8015A604(self);
        return;
    case 12:
        fn_8015A6B0(self);
        return;
    case 13:
        fn_8015ABAC(self);
        return;
        return;
    default:
        return;
    }
}

extern "C" void fn_8015D194(_ENEMY_WORK* self) {
    self->field_0x330 = 0;
    self->field_0x331 = 0;
    self->bytes_0x332.field_0x333 = 0;
    self->field_0x335 = 0;
    self->field_0x336 = 0;
}

extern "C" s32 fn_8015D1B0(_ENEMY_WORK* self, u8 a1) {
    u8 temp_r3;

    if ((a1 == 5) && ((s32) self->action_0x1E5 == 1) && ((temp_r3 = self->state_sub, (((u32) (temp_r3 - 0x13) > 1U) == 0)) || ((s32) temp_r3 == 9))) {
        return 1;
    }
    return 0;
}

extern "C" s32 fn_8015D1F0(_ENEMY_WORK* self) {
    return self->state_0x1E2 == 0;
}

extern "C" void fn_8015D200(_ENEMY_WORK* self) {
    fn_8013A654(self, 3);
}

/* This unit's own `.bss` (`splits.txt` `.bss 0x806A7820..0x806A7868`), in address order: the three two-vector
 * records its static constructor `fn_8015D764` builds (`.data` tables point at them).  Names are GUESSes:
 * each record is a (0, y0, 0) / (0, y1, 0) pair of model-space points. */
VEC3 vec_pair_801550FC_0[2];  /* +0x806A7820 */
VEC3 vec_pair_801550FC_1[2];  /* +0x806A7838 */
VEC3 vec_pair_801550FC_2[2];  /* +0x806A7850 */
