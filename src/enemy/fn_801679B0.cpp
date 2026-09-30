/* enemy/fn_801679B0.cpp - the enemy state-machine unit, 0x801679B0..0x80170600 (117 functions).
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap lookup:
 * every function of the range reports only `zz_<addr>_` in the shared runtime dump, and
 * `config/RMHE08/symbols.txt` carries only the bare `fn_XXXXXXXX` entries).
 *
 * Registration: the range is unclaimed in `splits.txt`; it sits in the `enemy` link band directly above
 * `enemy/fn_8014A1BC.c` (ends 0x801502C8) and every callee it names is an enemy symbol
 * (`em_act_ck__FP11_ENEMY_WORKUcUc`, `get_enemy_data__FP11_ENEMY_WORK`, `get_joint_wpos_em__FP...`), so it
 * registers as `enemy/` (brief section 2, class 3: what the code does plus the naming scheme of its
 * neighbours).
 *
 * Language: C++.  The unit calls `em_frame_check(_ENEMY_WORK*, u16, f32, f32)`, whose map spelling
 * `em_frame_check__FP11_ENEMY_WORKUsff` is a C++ mangling: rule 9 forbids writing the mangled spelling as
 * the callee, and a real signature only produces that symbol from the C++ front-end, so the unit is `.cpp`.
 * The unit's own `fn_*` functions stay flat symbols through `extern "C"`.  The one foreign callee it
 * needs a declaration for (`em_busy_set`, owned by `enemy/fn_8012BDF4.cpp`) and the unsplit enemy-band
 * helpers it calls live in their owner headers (`include/enemy/fn_8012BDF4.h`, `include/unsplit/enemy.h`),
 * not in this file (rule 2).
 *
 * Inventory and per-symbol measurement: `python tools/units/recompile.py enemy/fn_801679B0 --measure <symbol>`.
 *
 * Status.  42 of the 117 symbols are written and every one of them measures at or above the 80 % bar
 * (38 byte-identical at 100 %, `fn_801679B0` 83.13 and `fn_80167BA8` 83.12, `fn_8016E544` 97.75,
 * `fn_8016E610` 96.18, `fn_8016EE00`/`fn_80169360` 93.02).  The shape that closed the bulk is the
 * decompiler's own: each function is one of three families -
 *   * a `switch (self->state_0x05)` start-up advance that seeds `em_mot_set`/`em_mot_set_ck` and waits on
 *     `em_mot_end_ck`,
 *   * a dispatcher `switch (self->state_sub)` / `switch (self->action_0x1E5)` over the per-action handlers,
 *   * a tiny predicate.
 * `m2c` (tools/m2c, fed by `tools/units/m2cinput.py`) recovered all three, so the residual work is the
 * remaining 75 symbols, not a shape still to find.  The largest unwritten ones (each needs a struct this
 * unit reads that `include/enemy.h` does not yet name - 0x1BC/0x1C4, 0x314-0x324, 0x46C, 0x834/0x835, and a
 * u8 at 0x1E4 - plus a `cror eq,lt,eq` float compare m2c reports as `M2C_ERROR`) are listed in the outbox;
 * the four written functions still below 100 % are each one extra `clrlwi` byte-mask or one shared
 * `return` branch (see `fn_8016EE00` below).
 *
 * `fn_8016EE00` 93.02: retail masks the `u8` parameter (`clrlwi r0,r4,24; cmplwi r0,0x1`) where this build
 * folds the compare onto `r4` directly, and its case-1 false path shares the dispatch's exit branch while
 * this source emits a second `b`; the two fixes are a wider parameter with an explicit `(u8)` cast and a
 * fall-through instead of the trailing `return`, both left as recorded residuals rather than guessed.
 */

#include "types.h"
#include "enemy.h"
#include "unsplit/enemy.h"
#include "enemy/fn_8012BDF4.h"

/* ---------------------------------------------------------------------------------------------------
 * callees and pool literals owned by other units (declared by their map spelling; playbook 29)
 * ------------------------------------------------------------------------------------------------- */

/* The real signature of the one mangled callee: the C++ front-end emits the map's
 * `em_frame_check__FP11_ENEMY_WORKUsff` from it (rule 9 never spells the mangling). */
u32 em_frame_check(_ENEMY_WORK* self, u16 a, f32 b, f32 c);

/* Flat map symbols are C-linkage in a C++ TU; without this the front-end mangles every
 * reference and nothing pairs. */
extern "C" {
extern void fn_80128AAC(_ENEMY_WORK *self, s32 a, s32 b);
extern void fn_80168678(_ENEMY_WORK *self);
extern void fn_80168828(_ENEMY_WORK *self);
extern void fn_801688B4(_ENEMY_WORK *self);
extern void fn_80168A78(_ENEMY_WORK *self, s32 a);
extern void fn_80168B7C(_ENEMY_WORK *self);
extern void fn_80168C8C(_ENEMY_WORK *self);
extern void fn_80168CDC(_ENEMY_WORK *self);
extern void fn_8016900C(_ENEMY_WORK *self);
extern void fn_80169750(_ENEMY_WORK *self);
extern void fn_8016BFC4(_ENEMY_WORK *self);
extern void fn_8016D414(_ENEMY_WORK *self);
extern void fn_8016D490(_ENEMY_WORK *self);

extern u8 stage_map_kind_get(u8 a);
extern void em_hit_window_set_default(_ENEMY_WORK *self, s32 a, s32 b);
extern void fn_801697BC(_ENEMY_WORK *self);
extern void fn_801698F4(_ENEMY_WORK *self);
extern void fn_80169998(_ENEMY_WORK *self);
extern void fn_80169AB4(_ENEMY_WORK *self);
extern void fn_80169B58(_ENEMY_WORK *self);
extern void fn_80169EAC(_ENEMY_WORK *self);
extern void fn_80169F5C(_ENEMY_WORK *self);
extern void fn_8016A3C4(_ENEMY_WORK *self);
extern void fn_8016A478(_ENEMY_WORK *self);
extern void fn_8016A65C(_ENEMY_WORK *self);
extern void fn_8016A70C(_ENEMY_WORK *self);
extern void fn_8016A8EC(_ENEMY_WORK *self);
extern void fn_8016A998(_ENEMY_WORK *self);
extern void fn_8016AC0C(_ENEMY_WORK *self);
extern void fn_8016ACB0(_ENEMY_WORK *self);
extern void fn_8016AF30(_ENEMY_WORK *self);
extern void fn_8016AFDC(_ENEMY_WORK *self);
extern void fn_8016B114(_ENEMY_WORK *self);
extern void fn_8016B1C0(_ENEMY_WORK *self);
extern void fn_8016B3C0(_ENEMY_WORK *self);
extern void fn_8016B46C(_ENEMY_WORK *self);
extern void fn_8016B60C(_ENEMY_WORK *self);
extern void fn_8016B6B8(_ENEMY_WORK *self);
extern void fn_8016BA24(_ENEMY_WORK *self);
extern void fn_8016BAD0(_ENEMY_WORK *self);
extern void fn_8016BF18(_ENEMY_WORK *self);
extern void fn_8016DC3C(_ENEMY_WORK *self, s32 a, s32 b);
extern void fn_8016DE40(_ENEMY_WORK *self, s32 a);
extern void fn_8016DEF8(_ENEMY_WORK *self);
extern void fn_8016E088(_ENEMY_WORK *self, s32 a);
extern void fn_8016E2C8(_ENEMY_WORK *self);
extern void fn_80169164(_ENEMY_WORK *self, s32 a);
extern void fn_80169360(_ENEMY_WORK *self, u8 a);
extern void fn_801694C4(_ENEMY_WORK *self, s32 a);
extern void fn_8016D53C(_ENEMY_WORK *self);
extern void fn_8016D5B8(_ENEMY_WORK *self);
extern void fn_8016D63C(_ENEMY_WORK *self);
extern void fn_8016D6C0(_ENEMY_WORK *self);
extern void fn_8016D744(_ENEMY_WORK *self);
extern void fn_8016D7C8(_ENEMY_WORK *self);
extern void fn_8016D84C(_ENEMY_WORK *self);
extern void fn_8016D8D8(_ENEMY_WORK *self);
extern void fn_8016D95C(_ENEMY_WORK *self);
extern void fn_8016D9EC(_ENEMY_WORK *self);
extern void fn_8016DB20(_ENEMY_WORK *self);
extern void fn_8016DBE8(_ENEMY_WORK *self);
extern void fn_8016E3A0(_ENEMY_WORK *self);
extern void fn_8016E444(_ENEMY_WORK *self);
extern void fn_8016E544(_ENEMY_WORK *self);
extern void fn_8016E610(_ENEMY_WORK *self, u8 a);
extern void fn_8016E6EC(_ENEMY_WORK *self);
extern void fn_8016E7D4(_ENEMY_WORK *self);
extern void fn_8016E824(_ENEMY_WORK *self);
extern void fn_8016EB4C(_ENEMY_WORK *self);
extern void fn_8016EBF4(_ENEMY_WORK *self, s32 a);
extern void fn_8016EE00(_ENEMY_WORK *self, u8 a);
extern void fn_8016EEE8(_ENEMY_WORK *self, s32 a);
extern void fn_8016F044(_ENEMY_WORK *self);
extern void fn_8016F180(_ENEMY_WORK *self);
extern void fn_8016F3FC(_ENEMY_WORK *self);
extern void fn_8016F4B0(_ENEMY_WORK *self);
extern void fn_8016F630(_ENEMY_WORK *self);
extern void fn_8016F6E4(_ENEMY_WORK *self);
extern void fn_8016F8F8(_ENEMY_WORK *self);
extern void fn_8016F9AC(_ENEMY_WORK *self);
extern void fn_8016FA00(_ENEMY_WORK *self);

extern f32 lbl_807974FC;
extern f32 lbl_80797518;
extern f32 lbl_8079751C;
extern f32 lbl_80797520;
extern f32 lbl_80797524;
extern f32 lbl_80797528;
extern f32 lbl_807977DC;
extern f32 lbl_807977E0;
extern f32 lbl_8079785C;
extern f32 lbl_80797860;

extern u32 lbl_8056FC10[];
}


/* ---------------------------------------------------------------------------------------------------
 * fn_801679B0 - advance the action's start-up state and seed the motion from the action code
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_801679B0(_ENEMY_WORK *self, u8 a, u8 b) {
    if (a == 7) {
        em_busy_set(self);
        em_busy_timer_reset(self);
        em_frame_flag_set(self);
    }

    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xE, 8, 0);
        switch (a) {
        case 0:
            em_approach_start(self, lbl_80797518, 0);
            break;
        case 1:
            em_approach_start(self, lbl_8079751C, 0);
            break;
        case 2:
            em_approach_start(self, lbl_80797520, 0);
            break;
        case 3:
            em_approach_start(self, lbl_80797524, 0);
            break;
        case 4:
            em_approach_start(self, lbl_80797520, 0);
            break;
        case 5:
            em_approach_start(self, lbl_8079751C, 0);
            break;
        case 6:
            em_approach_start(self, lbl_80797528, 0);
            break;
        case 7:
            em_approach_start(self, lbl_807974FC, 0);
            break;
        case 8:
            em_approach_start(self, lbl_807974FC, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x200) == 1) {
            switch (b) {
            case 0:
                self->state_0x05 += 1;
                em_mot_set(self, 0xF, 0, 0);
                break;
            case 1:
                em_action_finish(self);
                break;
            }
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * small dispatchers and predicates
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_80167BA8(_ENEMY_WORK *self, u8 a) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_8056FC10, 0, 0, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_8056FC10) == 1) {
            if (a == 0 || a == 2) {
                self->state_0x05 += 1;
                em_mot_set(self, 0x13, 2, 4);
                return;
            }
            em_action_finish(self);
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

extern "C" s32 fn_8016CB78(_ENEMY_WORK *self) {
    return self->state_0x1E2 == 0;
}

extern "C" s32 fn_801704CC(_ENEMY_WORK *self) {
    return self->state_0x1E2 == 0;
}

extern "C" void fn_8016CD50(void) {}

extern "C" void fn_8016D22C(void) {}

extern "C" void fn_8016D230(void) {}

extern "C" void fn_801704AC(void) {}

extern "C" void fn_8016C054(_ENEMY_WORK *self) {
    switch (self->action_0x1E5) {
    case 0:
        fn_80167404(self);
        return;
    case 1:
        fn_80167968(self);
        return;
    case 2:
        fn_80168678(self);
        return;
    case 7:
        fn_80168C8C(self);
        return;
    case 10:
        fn_80168CDC(self);
        return;
    case 11:
        fn_8016900C(self);
        return;
    case 12:
        fn_80169750(self);
        return;
    case 13:
        fn_8016BFC4(self);
        return;
    }
}

extern "C" void fn_80168C8C(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_80168828(self);
        return;
    case 1:
        fn_801688B4(self);
        return;
    case 2:
        fn_80168A78(self, 0);
        return;
    case 3:
        fn_80168A78(self, 1);
        return;
    case 4:
        fn_80168B7C(self);
        return;
    }
}

extern "C" void fn_8016D50C(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_8016D414(self);
        return;
    case 1:
        fn_8016D490(self);
        return;
    case 2:
        fn_8016D414(self);
        return;
    }
}

extern "C" void fn_8016D384(_ENEMY_WORK *self) {
    em_move_mode_set(self, 0);
    fn_80128AAC(self, 1, 3);
    fn_80133BB4(self);
}

extern "C" void fn_8016D3CC(_ENEMY_WORK *self) {
    em_move_mode_set(self, 0);
    fn_80128AAC(self, 1, 2);
    fn_80133BB4(self);
}

/* Pattern B: advance the start-up byte (state_0x05) and seed the motion */

extern "C" void fn_80167CBC(_ENEMY_WORK *self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x15, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80167D38(_ENEMY_WORK *self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 2, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80168010(_ENEMY_WORK *self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80169448(_ENEMY_WORK *self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 2, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8016D414(_ENEMY_WORK *self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 6, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8016D490(_ENEMY_WORK *self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8016D53C(_ENEMY_WORK *self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xF, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* Pattern A: dispatch on the action code (action_0x1E5) or the sub-state (state_sub) */

extern "C" void fn_80169750(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_80169164(self, 0);
        return;
    case 1:
        fn_80169360(self, 0);
        return;
    case 2:
        fn_80169448(self);
        return;
    case 3:
        fn_80169164(self, 1);
        return;
    case 4:
        fn_801694C4(self, 1);
        return;
    case 5:
        fn_801694C4(self, 0);
        return;
    case 6:
        fn_80169164(self, 2);
        return;
    case 7:
        fn_801694C4(self, 2);
        return;
    case 8:
        fn_80169360(self, 1);
        return;
    }
}

extern "C" void fn_8016E7D4(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_8016E444(self);
        return;
    case 2:
        fn_8016E544(self);
        return;
    case 3:
        fn_8016E610(self, 0);
        return;
    case 4:
        fn_8016E6EC(self);
        return;
    case 5:
        fn_8016E610(self, 1);
        return;
    }
}

extern "C" void fn_8016F044(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_8016EBF4(self, 0);
        return;
    case 1:
        fn_8016EE00(self, 0);
        return;
    case 2:
        fn_8016EBF4(self, 1);
        return;
    case 3:
        fn_8016EEE8(self, 1);
        return;
    case 4:
        fn_8016EE00(self, 1);
        return;
    }
}

extern "C" void fn_8016F9AC(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_8016F180(self);
        return;
    case 1:
        fn_8016F3FC(self);
        return;
    case 2:
        fn_8016F4B0(self);
        return;
    case 3:
        fn_8016F630(self);
        return;
    case 4:
        fn_8016F6E4(self);
        return;
    case 5:
        fn_8016F8F8(self);
        return;
    }
}

extern "C" void fn_8016DBE8(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 1:
        fn_8016D53C(self);
        return;
    case 2:
        fn_8016D5B8(self);
        return;
    case 3:
        fn_8016D63C(self);
        return;
    case 4:
        fn_8016D6C0(self);
        return;
    case 5:
        fn_8016D744(self);
        return;
    case 6:
        fn_8016D7C8(self);
        return;
    case 7:
        fn_8016D84C(self);
        return;
    case 8:
        fn_8016D8D8(self);
        return;
    case 9:
        fn_8016D95C(self);
        return;
    case 10:
        fn_8016D9EC(self);
        return;
    case 11:
        fn_8016DB20(self);
        return;
    }
}

extern "C" void fn_8016FA00(_ENEMY_WORK *self) {
    switch (self->action_0x1E5) {
    case 0:
        fn_8016D50C(self);
        return;
    case 1:
        fn_8016DBE8(self);
        return;
    case 2:
        fn_8016E3A0(self);
        return;
    case 7:
        fn_8016E7D4(self);
        return;
    case 10:
        fn_8016E824(self);
        return;
    case 11:
        fn_8016EB4C(self);
        return;
    case 12:
        fn_8016F044(self);
        return;
    case 13:
        fn_8016F9AC(self);
        return;
    }
}

extern "C" s32 fn_801704DC(_ENEMY_WORK *self) {
    if ((u32)(stage_map_kind_get(self->field_0x1E0) - 1) <= 1U && self->field_0x011 != 0) {
        self->state_0x1FC = 1;
        self->state_0x1FE = 7;
        self->state_0x1FF = 0;
        return 1;
    }
    return 0;
}

extern "C" void fn_8016D5B8(_ENEMY_WORK *self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x18, 4, 0);
        fn_801321C4(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_8016D3CC(self);
        }
        return;
    }
}

extern "C" void fn_8016D63C(_ENEMY_WORK *self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x19, 8, 0x5E);
        fn_801321C4(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_8016D384(self);
        }
        return;
    }
}

extern "C" void fn_8016D6C0(_ENEMY_WORK *self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 4, 0);
        fn_801321C4(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_8016D384(self);
        }
        return;
    }
}

extern "C" void fn_8016D744(_ENEMY_WORK *self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1C, 4, 0);
        fn_801321C4(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_8016D384(self);
        }
        return;
    }
}

extern "C" void fn_8016D7C8(_ENEMY_WORK *self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x1D, 4, 0);
        fn_801321C4(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_8016D3CC(self);
        }
        return;
    }
}

extern "C" void fn_8016D8D8(_ENEMY_WORK *self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x1F, 4, 0);
        fn_801321C4(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_8016D384(self);
        }
        return;
    }
}

extern "C" void fn_80168828(_ENEMY_WORK *self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 4, 0);
        em_hit_window_set_default(self, 0, 1);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8016D84C(_ENEMY_WORK *self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x1A, 4, 0);
        fn_801321C4(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_801321D0(self);
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8016D95C(_ENEMY_WORK *self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x19, 4, 0);
        fn_801321C4(self);
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_807977E0, lbl_807977DC) == 1) {
            fn_8016D384(self);
        }
        return;
    }
}

extern "C" void fn_8016BFC4(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_801697BC(self);
        return;
    case 1:
        fn_801698F4(self);
        return;
    case 2:
        fn_80169998(self);
        return;
    case 3:
        fn_80169AB4(self);
        return;
    case 4:
        fn_80169B58(self);
        return;
    case 5:
        fn_80169EAC(self);
        return;
    case 6:
        fn_80169F5C(self);
        return;
    case 7:
        fn_8016A3C4(self);
        return;
    case 8:
        fn_8016A478(self);
        return;
    case 9:
        fn_8016A65C(self);
        return;
    case 10:
        fn_8016A70C(self);
        return;
    case 11:
        fn_8016A8EC(self);
        return;
    case 12:
        fn_8016A998(self);
        return;
    case 13:
        fn_8016AC0C(self);
        return;
    case 14:
        fn_8016ACB0(self);
        return;
    case 15:
        fn_8016AF30(self);
        return;
    case 16:
        fn_8016AFDC(self);
        return;
    case 17:
        fn_8016B114(self);
        return;
    case 18:
        fn_8016B1C0(self);
        return;
    case 19:
        fn_8016B3C0(self);
        return;
    case 20:
        fn_8016B46C(self);
        return;
    case 21:
        fn_8016B60C(self);
        return;
    case 22:
        fn_8016B6B8(self);
        return;
    case 23:
        fn_8016BA24(self);
        return;
    case 24:
        fn_8016BAD0(self);
        return;
    case 25:
        fn_8016BF18(self);
        return;
    }
}

extern "C" void fn_8016E3A0(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_8016DC3C(self, 0, 0);
        return;
    case 1:
        fn_8016DC3C(self, 1, 1);
        return;
    case 2:
        fn_8016DC3C(self, 2, 1);
        return;
    case 3:
        fn_8016DC3C(self, 3, 1);
        return;
    case 4:
        fn_8016DC3C(self, 4, 1);
        return;
    case 5:
        fn_8016DE40(self, 0);
        return;
    case 6:
        fn_8016DEF8(self);
        return;
    case 7:
        fn_8016E088(self, 0);
        return;
    case 8:
        fn_8016E2C8(self);
        return;
    case 9:
        fn_8016E088(self, 1);
        return;
    case 10:
        fn_8016DC3C(self, 5, 1);
        return;
    case 11:
        fn_8016DE40(self, 1);
        return;
    case 12:
        fn_8016DC3C(self, 6, 1);
        return;
    }
}

extern "C" void fn_8016E544(_ENEMY_WORK *self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 0, 0);
        em_hit_window_set_default(self, 0, 1);
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_8079785C, lbl_807977DC) == 1) {
            self->state_0x05 += 1;
            em_hit_window_set_default(self, 0, 2);
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

extern "C" void fn_8016E610(_ENEMY_WORK *self, u8 a) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x12, 4, 0);
        em_hit_window_set_default(self, 0, 3);
        return;
    case 1:
        switch (a) {
        case 0:
            if (em_mot_end_ck(self) == 1) {
                em_action_finish(self);
                return;
            }
            return;
        case 1:
            if (em_frame_check(self, 1, lbl_80797860, lbl_807977DC) == 1) {
                self->state_0x05 += 1;
                em_state_set(self, 7, 4);
            }
            break;
        }
        break;
    }
}

extern "C" void fn_8016EE00(_ENEMY_WORK *self, u8 a) {
    if (a == 1) {
        em_frame_flag_set(self);
    }
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xA, 8, 0);
        em_approach_start(self, lbl_807977DC, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0xA0) == 1) {
            self->state_0x05 += 1;
            em_mot_set(self, 0xB, 2, 0);
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

extern "C" void fn_80169360(_ENEMY_WORK *self, u8 a) {
    if (a == 1) {
        em_frame_flag_set(self);
    }
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xE, 8, 0);
        em_approach_start(self, lbl_807974FC, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0xA0) == 1) {
            self->state_0x05 += 1;
            em_mot_set(self, 0xCA, 6, 0);
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
