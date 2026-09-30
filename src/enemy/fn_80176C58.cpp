/* enemy/fn_80176C58.cpp - the enemy death/state sub-action unit, 11 functions, 0x80176C58..0x80177608.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every symbol this file defines is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * This is the small enemy-side state block discovery proposed between the enemy action unit
 * (`enemy/fn_8012BDF4.cpp`) and the enemy motion unit that follows it.  It is reached with a
 * `_ENEMY_WORK*` (the `em_*__FP11_ENEMY_WORK...` callees pin that type) and drives the enemy's
 * "sub state" `_ENEMY_WORK::state_sub` (+0x1E6): `fn_801775C0` is the dispatcher that calls one of
 * `fn_80177314`/`fn_801773B0`/`fn_80177430`/`fn_801774B0`/`fn_80177540` for sub states 0/1/2/4/5, and
 * each of those is the same three-step shape - bump `state_0x05`, hand the enemy a motion
 * (`em_move_mode_set` + `fn_8012F5C4`), then wait for it (`em_mot_end_ck`) and finish (`em_action_finish` /
 * `fn_80128030`).
 *
 * The rest is the enemy's damage/death bookkeeping:
 *   - `fn_80176C58(self, arg)` is the per-tick entry: it attaches a 12-byte helper (the vtable object
 *     `lbl_805AA900`, constructed by `fn_80176E50`) when `fn_801391E8` says the enemy has none, then
 *     resets `field_0x1E4`, picks a motion set from `stage_map_kind_get(self->field_0x1E0)`, and - when the
 *     enemy's `state_0x009` is clear - spawns the effect (`setVector3` + `fn_801057A4` + `fn_8010A7D4`).
 *   - `fn_80176E8C` / `fn_8017708C` are the two state transition tables, keyed on the current
 *     `(state, sub-state)` pair read out of two `u8` records the caller passes in.
 *   - `fn_8017708C` also advances the damage meter `field_0x1E4` (its `sub == 6` arm raises it by a
 *     step clamped to `em_parts_damage_level_get(self, 3)`), and `fn_80177258` is the death check
 *     (`em_die_ck`) that arms the death timers `field_0x32F`/`field_0x330`/`timer_0x332`.
 *
 * Language.  The object's mapped callees with an argument list are C++ manglings
 * (`em_die_ck__FP11_ENEMY_WORK`, `em_parts_damage_level_get__FP11_ENEMY_WORKUc`,
 * `setVector3__FPQ34nw4r4math4VEC3fff`) and the helper is allocated with `operator new`
 * (`__nw__FUl`), so the file is C++: the mangled callees are declared through their real signatures
 * (the front-end mangles them back to the map spellings) and the definitions keep their unmangled map
 * names with `extern "C"`.  No `__FILE__` string names a source file, so the map's `fn_XXXXXXXX`
 * placeholder is kept (Naming note above).
 *
 * Types.  `_ENEMY_WORK` is the shared record in `include/enemy.h`; this unit's additions to it
 * (the death meter `field_0x1E4`, the death timers `field_0x32F`/`field_0x330`/`timer_0x332`,
 * `field_0x38A` and the effect scale `field_0x7B0`) are declared there.  The 12-byte helper
 * `fn_80176C58` allocates is defined here (only this unit touches it).
 *
 * Source pragmas, evidenced: `#pragma peephole off` - retail keeps the unfused `clrlwi`+`cmpwi` /
 * `clrlwi`+`slwi` pairs the `-O3` peephole folds into `clrlwi.`/`clrlslwi` (playbook 39, the same
 * finding as `enemy/fn_8012BA00.c` and `enemy/fn_8012BDF4.cpp`; `fn_80176E50` and `fn_80176C58` are
 * the two functions whose score it changes).
 *
 * Status (official report metric, per symbol, measured in the worktree with
 * `recompile.py --main <worktree>`): 9 of the 11 functions are exactly 100 % (`fn_80176E50`,
 * `fn_80176E8C`, `fn_80177258`, `fn_80177314`, `fn_801773B0`, `fn_80177430`, `fn_801774B0`,
 * `fn_80177540`, `fn_801775C0`), `fn_80176C58` is 99.52 % and `fn_8017708C` is 86.52 %.  `extab` is
 * byte-identical to the target; `extabindex` differs only in `fn_8017708C`'s size word (our 0x1EC vs
 * the target's 0x1CC), which closes when that function does.  The object stays `NonMatching`: the
 * helper's vtable (`lbl_805AA900`) and the `.sdata2` pool are not claimed by this unit (the splitter
 * leaves them in the auto band), so the sections are not byte-identical even where the code is.
 *
 * Residual - `fn_8017708C` 86.52 % (target 0x1CC = 460 B, ours 0x1EC = 492 B).  The 32-byte gap is
 * the two nested-switch dispatches (case 5's `{0x19,0x1A}` and case 7's `{0x2A..0x2C}`/`{0x24,0x25}`
 * runs): retail lowers an adjacent case pair to a register range test
 * (`addi r0,r4,-0x19; cmplwi r0,1; ble`) and keeps the masked `sub` in one register across the whole
 * dispatch, while every source shape tried here re-masks and uses the two-compare range form
 * (`cmpwi r0,0x19; blt; cmpwi r0,0x1A; ble`).  Tried: `switch ((u8)sub)`, `u8 s = (u8)sub` and
 * `int s = (u8)sub` locals with `switch (s)`, and an if-chain with an explicit
 * `(u32)(s - lo) <= hi - lo` range test (that one gets the range form but inlines the handlers and
 * scores 82.8 % instead of 86.5 %).  The function is above the 80 % bar; the residual is codegen
 * shape, not comprehension.
 *
 * Inventory: `python tools/units/ledger.py unit enemy/fn_80176C58.cpp`.
 */
#include "types.h"
#include "nw4r/math.h"
#include "enemy.h"
#include "unsplit/enemy.h"
#include "unsplit/ef.h"
#include "ef/fn_80105314.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"

#pragma peephole off

/* The 12-byte helper `fn_80176C58` allocates and `fn_80176E50` constructs (storing the vtable
 * `lbl_805AA900` at +0).  Only the vtable slot is touched by this unit; the rest is padding.
 * size: 0xC (traced from the `operator new(0xC)` call in `fn_80176C58`). */
typedef struct Helper_80176E50 {
    /* +0x0 */ void* vtbl;
    /* +0x4 */ u32 unused_0x4;
    /* +0x8 */ u32 unused_0x8;
} Helper_80176E50;

/* The 12-byte helper's vtable (a `.data` label the split has not assigned to a unit). */
extern "C" u8 lbl_805AA900[];

/* --------------------------------------------------------------------------------------------- */
/* callees                                                                                        */
/* --------------------------------------------------------------------------------------------- */

/* Declared in a shared header: `VEC3_ctor`, `setVector3` (`nw4r/math.h`); `fn_8012ECF0`,
 * `em_mot_end_ck`, `em_move_mode_set` (`unsplit/enemy.h`).  The C++ free functions are declared by their
 * real signatures so the front-end mangles them to the map spellings. */
extern "C" u8 stage_map_kind_get(u8 id);
extern "C" void fn_80182978(_ENEMY_WORK* self);
extern "C" void fn_80128A8C(_ENEMY_WORK* self, u32 a, u32 b);
extern "C" u32 quest_id_get(void);
extern "C" void fn_8012A354(_ENEMY_WORK* self);
extern "C" void fn_8012999C(_ENEMY_WORK* self);
extern "C" void fn_801823A0(_ENEMY_WORK* self, u32 a);
extern "C" void fn_80128BF8(_ENEMY_WORK* self, u32 a);
extern "C" void em_action_finish(_ENEMY_WORK* self);
extern "C" void fn_80128030(_ENEMY_WORK* self);

/* This unit's own forward declaration (fn_80176C58 calls it before its definition). */
extern "C" Helper_80176E50* fn_80176E50(Helper_80176E50* self);

/* The two mangled free functions, declared by their real signatures (rule 9: never the mangled
 * spelling - the front-end produces `em_die_ck__FP11_ENEMY_WORK` from this declaration). */
u32 em_die_ck(_ENEMY_WORK* self);
u32 em_parts_damage_level_get(_ENEMY_WORK* self, u8 part);

/* `operator new` is what `fn_80176C58`'s 0xC-byte allocation lowers to (`__nw__FUl`). */
void* operator new(unsigned long size);

/* --------------------------------------------------------------------------------------------- */
/* functions, in address order                                                                    */
/* --------------------------------------------------------------------------------------------- */

extern "C" void fn_80176C58(_ENEMY_WORK* self, u32 arg) {
    VEC3 v;
    Helper_80176E50* helper;
    u8 kind;
    u8 mode;

    VEC3_ctor(&v);
    if (fn_801391E8(self) == 0) {
        helper = (Helper_80176E50*)operator new(0xC);
        if (helper != 0) {
            fn_80176E50(helper);
        }
        fn_801390FC(self, helper);
    }
    fn_80182978(self);
    self->field_0x1E4 = 0;
    mode = arg;
    if ((u32)(mode - 1) <= 1 || mode == 4) {
        /* these modes do not run the motion hand-off */
    } else {
        kind = stage_map_kind_get(self->field_0x1E0);
        switch (kind) {
          case 1:
            if (self->act_id == 5) {
                em_move_mode_set(self, 0);
                fn_80128A8C(self, 0, 0);
            } else {
                em_move_mode_set(self, 2);
                fn_80128A8C(self, 0, 4);
            }
            break;
          case 8:
            em_move_mode_set(self, 0);
            fn_80128A8C(self, 0, 0);
            break;
          case 9:
            if (self->act_id == 0) {
                em_move_mode_set(self, 0);
                fn_80128A8C(self, 0, 0);
            } else {
                em_move_mode_set(self, 2);
                fn_80128A8C(self, 0, 4);
            }
            break;
          default:
            em_move_mode_set(self, 2);
            fn_80128A8C(self, 0, 4);
            break;
        }
    }
    if ((u16)quest_id_get() == 0x3EC) {
        self->field_0x7B0 = 0.5f;
    }
    if (self->state_0x009 == 0) {
        setVector3(&v, 0.0f, -80.0f, 90.0f);
        fn_801057A4(self, 0x1A, &v, 1.5f, 0x18E4);
        fn_8010A7D4(self, 1);
    }
}

extern "C" Helper_80176E50* fn_80176E50(Helper_80176E50* self) {
    fn_80147E2C(self);
    self->vtbl = lbl_805AA900;
    return self;
}

extern "C" void fn_80176E8C(_ENEMY_WORK* self, u8* state, u8* sub) {
    switch (state[0]) {
      case 2:
        switch (sub[0]) {
          case 0:
          case 3:
            if (fn_8012ECF0() == 1) {
                sub[0] = 4;
            }
            break;
          case 5:
            if (fn_8012ECF0() == 1) {
                sub[0] = 7;
            }
            break;
          case 6:
            if (fn_8012ECF0() == 1) {
                sub[0] = 8;
            }
            break;
          case 9:
            if (fn_8012ECF0() == 1) {
                sub[0] = 0xA;
            }
            break;
        }
        break;
      case 5:
        switch (sub[0]) {
          case 2:
            if (fn_8012ECF0() == 1) {
                sub[0] = 0xD;
            }
            break;
          case 0xA:
            if (fn_8012ECF0() == 1) {
                sub[0] = 0xE;
            }
            break;
          case 0xB:
            if (fn_8012ECF0() == 1) {
                sub[0] = 0xF;
            }
            break;
          case 0x1F:
            if (fn_8012ECF0() == 1) {
                sub[0] = 0x20;
            }
            break;
          case 0x24:
            if (fn_8012ECF0() == 1) {
                sub[0] = 0x25;
            }
            break;
        }
        break;
      case 7:
        switch (sub[0]) {
          case 6:
            if (fn_8012EC3C(self) == 1) {
                sub[0] = 0x17;
            }
            break;
          case 7:
          case 0xF:
            if (fn_8012EC3C(self) == 1) {
                sub[0] = 0x18;
            }
            break;
          case 0x22:
            if (fn_8012ECF0() == 1) {
                state[0] = 2;
                sub[0] = 4;
            }
            break;
        }
        break;
    }
}

extern "C" void fn_8017708C(_ENEMY_WORK* self, u32 kind, u32 sub) {
    s32 step;
    s32 limit;

    switch ((u8)kind) {
      case 1:
        switch ((u8)sub) {
          case 6:
            fn_80130F74(self);
            break;
          case 7:
            fn_801376B4(self);
            break;
        }
        break;
      case 5: {
        int s = (u8)sub;

        switch (s) {
          case 0x19:
          case 0x1A:
            fn_8012A354(self);
            break;
          case 0x1D:
            fn_80130F74(self);
            break;
          case 0x26:
            fn_801376B4(self);
            break;
        }
        break;
      }
      case 7: {
        int s = (u8)sub;

        switch (s) {
          case 6: {
            u8 v;

            step = 0x32;
            if (self->field_0x7c8 >= 0x29) {
                step = 0x64;
            }
            v = em_parts_damage_level_get(self, 3);
            limit = 0xFF;
            if (v >= 3) {
                limit = 0x63;
            }
            if ((s32)self->field_0x1E4 < limit - step) {
                self->field_0x1E4 += (u8)step;
            } else {
                self->field_0x1E4 = (u8)limit;
            }
            break;
          }
          case 7:
          case 0xF:
            if (self->field_0x1E4 > 0x64) {
                self->field_0x1E4 -= 0x64;
            } else {
                self->field_0x1E4 = 0;
            }
            break;
          case 0x24:
          case 0x25:
          case 0x2A:
          case 0x2B:
          case 0x2C:
            if (self->field_0x1E4 > 0x1E) {
                self->field_0x1E4 -= 0x1E;
            } else {
                self->field_0x1E4 = 0;
            }
            break;
        }
        break;
      }
      case 0xA:
        switch ((u8)sub) {
          case 0xB1:
            fn_8012999C(self);
            break;
          case 0xC9:
          case 0xD1:
            fn_80135C5C(self, 0, 0);
            break;
        }
        break;
    }
}

extern "C" void fn_80177258(_ENEMY_WORK* self) {
    fn_80182978(self);
    if (em_die_ck(self) == 0) {
        if (fn_8012EC3C(self) == 1) {
            if (self->state_0x1E2 == 0) {
                self->field_0x32F = 1;
                self->field_0x330 = 1;
            }
        } else {
            self->field_0x32F = 0;
        }
        if (self->field_0x38F != 0) {
            if (self->timer_0x332 < 0x384) {
                self->timer_0x332 += 1;
            }
        } else {
            self->timer_0x332 = 0;
        }
    }
    if (self->state_0x1E2 == 2) {
        self->field_0x38A = 1;
    } else {
        self->field_0x38A = 0;
    }
}

extern "C" void fn_80177314(_ENEMY_WORK* self) {
    fn_801823A0(self, 1);
    switch (self->state_0x05) {
      case 0:
        self->state_0x05++;
        em_move_mode_set(self, 0);
        fn_8012F5C4(self, 1, 0x14, 0, 1);
        break;
      case 1:
        fn_80128BF8(self, 0);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_801773B0(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
      case 0:
        self->state_0x05++;
        em_move_mode_set(self, 0);
        fn_8012F5C4(self, 0x14, 0x14, 0, 1);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_80177430(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
      case 0:
        self->state_0x05++;
        em_move_mode_set(self, 0);
        fn_8012F5C4(self, 0x1D, 0x14, 0, 1);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_801774B0(_ENEMY_WORK* self) {
    fn_801823A0(self, 1);
    switch (self->state_0x05) {
      case 0:
        self->state_0x05++;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 0x28, 0x28, 0, 3);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_80177540(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
      case 0:
        self->state_0x05++;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 0x36, 0x1E, 0, 3);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_801775C0(_ENEMY_WORK* self) {
    switch (self->state_sub) {
      case 0:
        fn_80177314(self);
        break;
      case 1:
        fn_801773B0(self);
        break;
      case 2:
        fn_80177430(self);
        break;
      case 4:
        fn_801774B0(self);
        break;
      case 5:
        fn_80177540(self);
        break;
    }
}
