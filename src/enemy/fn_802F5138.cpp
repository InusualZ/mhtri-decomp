/*
 * enemy/fn_802F5138.cpp - phase 4 unit, `.text` 0x802F5138..0x802F9994 (61 functions, 18524 bytes).
 *
 * PHASE 4 (docs/splits/phase4, window d).  Recut of fn_802F5138.cpp: its functions whose address lies in this range,
 * in address order; the rest of the range keeps its original bytes.  27 of 61 functions have a body here.
 *
 * FLAGS.  `cflags_main`.  The tail of the old range (0x802F9994..) is `enemy/em_sub_state_prog.cpp`.
 *
 * Sections: the unit's block in config/RMHE08/splits.txt (.bss, .ctors, .data, .rodata, .sdata, .sdata2, .text, extab,
 * extabindex).
 */
/* ---- header inherited from src/enemy/fn_802F5138.cpp (written against its pre-phase-4 range) ---- */
/* enemy/fn_802F5138.cpp - an enemy action-program band, `.text` 0x802F5138..0x802FA9A0.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>`: every address resolves to a `zz_XXXXXXXX_` dump
 * placeholder and a bare `.text` entry in config/RMHE08/symbols.txt, and no `__FILE__` source-name
 * literal is reachable from the range's `.data`/`.rodata` references).
 *
 * WHAT IT IS.  The per-action state machines of one enemy's AI.  Every body takes the shared
 * `_ENEMY_WORK` record: the action dispatcher `fn_802F96B4` switches on its `action` (+0x1E5) and
 * the sub-state dispatchers (`fn_802F5D24`, `fn_802F92E8`, `fn_802F9678`, `fn_802F9BF0`,
 * `fn_802FA964`) on `state_sub` (+0x1E6); the bodies drive the enemy motion API
 * (`em_move_mode_set`, `em_mot_set`, `em_mot_set_ck`, `em_mot_end_ck`, `fn_80128030`), place the effects
 * (`res_eft_UV_model_create_name`, `em_se_tbl_play`/`em_se_tbl_play_alt` with the record tables
 * `lbl_805D6F70`..`lbl_805D7300`) and step the work record's own state bytes.  The band's `.data`
 * holds those record tables and the compiler-emitted switch tables (`jumptable_805D6E50`,
 * `jumptable_805D6E70`, `jumptable_805D89EC`, `jumptable_805D8A48`, `jumptable_805D8AA4`,
 * `jumptable_805D8C44`).
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string: every `lis`/`addi`
 * pair in the range resolves to the `.sdata2` float pool, a switch table or one of the band's record
 * tables.  2. `dumpmap.py lookup` answers `zz_XXXXXXXX_` for every row but the unrelated 4-byte
 * `fn_802F9714`.  3. The code places the unit in `enemy`: of the range's 194 distinct callees the
 * largest block is the `enemy` module (`em_die_ck`, `em_work_die_ck`, `em_get_mot_no`,
 * `fn_8012EC74.cpp`'s motion band), and the `.data` `em0XX_prog_tbl` program tables bracket the
 * band.  The file therefore keeps the map's `fn_802F5138` stem; no name and no module was invented.
 *
 * LANGUAGE AND SECTIONS.  C++ - the range reaches genuinely mangled callees (`setVector3`,
 * `calcVecAng2`, `rotVecX`, `ran_suu`, `res_eft_UV_model_create_name`) through their real
 * signatures (rule 9); every plain `fn_` definition is `extern "C"` so it keeps the map's name
 * (playbook 42).  Sections: `.text` 0x802F5138..0x802FA9A0, `extab` 0x80015514..0x800156B4
 * (52 records), `extabindex` 0x80033E04..0x80034074 (52 x 12 B) and the `.ctors` word at
 * 0x8056F390 (the file-scope static initialiser `fn_802F9898`).  The `.sdata2` pool the range reads
 * is `extern` here and never defined (playbook 29).
 *
 * SEAM (unproven).  One maximal unclaimed run, registered whole; it sits between `ef/eft035.cpp`
 * (ends 0x802F5138) and the unclaimed run that starts at 0x802FA9A0, and both edges are weak (no
 * `__FILE__` name reaches either).  `tudiscover`'s model spans the range from `hud/cockpit_quest.cpp`'s
 * `__FILE__` anchor with an un-taken candidate seam inside, so the extent may still be more than one
 * original TU; it settles as the functions match.
 *
 * RESIDUALS (measured 2026-09-26; unit 16.14 %, 2248/22632 bytes, 26 of 72 functions at 100 %).
 *  - 41 functions have no body yet: the placement block 0x802F5D60..0x802F8290 (26 functions) and
 *    the tail block 0x802F8B9C..0x802FA8D8 (including `fn_802F9C2C` 0x710 and `fn_802FA33C` 0x4B0).
 *  - `fn_802F5B98` 76.9 % / `fn_802F5C58` 78.2 %: the target's `mode` test is an unsigned range fold
 *    whose A-block the compiler lays out *after* the whole compare chain; the if/else-if, the
 *    `>= 3 && <= 4` and the dense `switch` spellings were all measured and lose more.
 *  - `fn_802FA7EC` 0 %: 4-byte `b fn_80463EE0` thunk; the only available declaration of that callee
 *    (`ef/fn_80114E34.cpp`'s `f32 fn_80463EE0(s16, f32)`) forces an `extsh` the target does not have.
 *  - Our object emits one extra 8-byte `.sdata2` (MWCC's `(f32)(s32)` conversion double).  The
 *    address the target loads is the shared `lbl_8079AA90`, referenced DOL-wide, so it is a shared
 *    pool entry: claim nothing (playbook 58).  `datagap.py --unit enemy/fn_802F5138` is the measure.
 *  - Levers this band needed, all measured: `switch` on a byte where retail keeps a signed `cmpwi`
 *    chain; `(f32)(s32)` where retail keeps the `xoris` conversion; a scoped `#pragma peephole off`
 *    per function (`fn_802F8B28`, `fn_802F9774`, `fn_802F96B4`, `fn_802F51DC`) - file-wide off costs
 *    `fn_802F5B98` 62.5 -> 20.8 %.
 *  - `include/enemy/fn_801251D0.h` gained the four-argument C++ view of 0x801251D0 and
 *    `enemy/em009_act.cpp`'s 28 call sites moved to it (both targets set r6 = the id):
 *    `fn_8038BD28` 96.729 -> 100.0 %.  The outbox asks the orchestrator to confirm that shared-file
 *    edit on the batch.
 * Per-function measurements: `.pi/notes/802f5138-fn-802f5138-046c.md`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "ef.h"
#include "ef/effect.h"
#include "ef/eft_res.h"
#include "enemy/enemy_control.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"
#include "lobby/fn_802FA9A0.h"
#include "stage/stg_w.h"

/* The band's pooled `.sdata2` constants (declared, never defined: the pool belongs to the data pass,
 * playbook 29). */
extern f32 lbl_8079AA88;
extern f32 lbl_8079AA98;
extern f32 lbl_8079AA98; extern f32 lbl_8079AA9C;
extern f32 lbl_8079AA98; extern f32 lbl_8079AA9C; extern f32 lbl_8079AAA0;
extern f32 lbl_8079AA98; extern f32 lbl_8079AA9C; extern f32 lbl_8079AAA0; extern f32 lbl_8079AAA4;
extern f32 lbl_8079AAB0;
extern f32 lbl_8079AAB0; extern f32 lbl_8079ABA4;
extern f32 lbl_8079AAB0; extern f32 lbl_8079ABA4; extern f32 lbl_8079ABA8;
extern f32 lbl_8079AB74;
extern f32 lbl_8079AB74; extern f32 lbl_8079AB78;
extern f32 lbl_8079AB74; extern f32 lbl_8079AB78; extern f32 lbl_8079AB7C;
extern f32 lbl_8079AB80;
extern f32 lbl_8079AB80; extern f32 lbl_8079AB84;
extern f32 lbl_8079AB80; extern f32 lbl_8079AB84; extern f32 lbl_8079AB88;
extern f32 lbl_8079AB8C;
extern f32 lbl_8079AB8C; extern f32 lbl_8079AB90;
extern f32 lbl_8079AB8C; extern f32 lbl_8079AB90; extern f32 lbl_8079AB94;
extern f32 lbl_8079AB98;
extern f32 lbl_8079AB98; extern f32 lbl_8079AB9C;
extern f32 lbl_8079AB98; extern f32 lbl_8079AB9C; extern f32 lbl_8079ABA0;
extern f32 lbl_8079ABAC;
extern f32 lbl_8079ABAC; extern f32 lbl_8079ABB0;

/* The band's own `.data` record tables (declared, never defined). */
extern u8 lbl_805D6F70[];
extern u8 lbl_805D6F70[]; extern u8 lbl_805D6F98[];
extern u8 lbl_805D6FC0[];
extern u8 lbl_805D6FC0[]; extern u8 lbl_805D6FE8[];
extern u8 lbl_805D7018[];
extern u8 lbl_805D7018[]; extern u8 lbl_805D7040[];
extern u8 lbl_805D7078[];
extern u8 lbl_805D7078[]; extern u8 lbl_805D70A0[];
extern u8 lbl_805D70C8[];
extern u8 lbl_805D70C8[]; extern u8 lbl_805D70F0[];
extern u8 lbl_805D7118[];
extern u8 lbl_805D7118[]; extern u8 lbl_805D7140[];
extern u8 lbl_805D7150[];
extern u8 lbl_805D7150[]; extern u8 lbl_805D7190[];
extern u8 lbl_805D71E8[];
extern u8 lbl_805D71E8[]; extern u8 lbl_805D7240[];
extern u8 lbl_805D7288[];
extern u8 lbl_805D7288[]; extern u8 lbl_805D72D0[];
extern u8 lbl_805D7300[];
extern u8 lbl_805D7328[];
extern u8 lbl_805D7328[]; extern u8 lbl_805D75C0[];
extern u8 lbl_805D7328[]; extern u8 lbl_805D75C0[]; extern u8 lbl_805D7AC0[];
extern u8 lbl_805D7B80[];
extern u8 lbl_805D7B80[]; extern u8 lbl_805D7DA0[];

/* Forward declarations of the band's own functions (the dispatchers below reference later bodies). */
extern "C" s32 fn_802F5138(_ENEMY_WORK* work);
extern "C" void fn_802F51C8(_ENEMY_WORK* work);
extern "C" void fn_802F5464(_ENEMY_WORK* work, u32 action, u32 state_sub);
extern "C" void fn_802F5B98(_ENEMY_WORK* work);
extern "C" void fn_802F5C58(_ENEMY_WORK* work);
extern "C" void fn_802F5D24(_ENEMY_WORK* work);
extern "C" void fn_802F6880(_ENEMY_WORK* work);
extern "C" void fn_802F7D28(_ENEMY_WORK* work);
extern "C" void fn_802F7F78(_ENEMY_WORK* work);
extern "C" void fn_802F803C(_ENEMY_WORK* work);
extern "C" void fn_802F8100(_ENEMY_WORK* work);
extern "C" void fn_802F8290(_ENEMY_WORK* work, s32 mode);
extern "C" void fn_802F8768(_ENEMY_WORK* work);
extern "C" void fn_802F87B8(_ENEMY_WORK* work);
extern "C" void fn_802F89F8(_ENEMY_WORK* work);
extern "C" void fn_802F8F24(_ENEMY_WORK* work);
extern "C" void fn_802F929C(_ENEMY_WORK* work);
extern "C" void fn_802F92E8(_ENEMY_WORK* work);
extern "C" void fn_802F930C(_ENEMY_WORK* work);
extern "C" void fn_802F943C(_ENEMY_WORK* work);
extern "C" void fn_802F94EC(_ENEMY_WORK* work);
extern "C" void fn_802F95C8(_ENEMY_WORK* work);
extern "C" void fn_802F9678(_ENEMY_WORK* work);
extern "C" s32 fn_802F9718(_ENEMY_WORK* work);
extern "C" s32 fn_802F9740(_ENEMY_WORK* work);
extern "C" s32 fn_802F9774(_ENEMY_WORK* work);
extern "C" void fn_802F96B4(_ENEMY_WORK* work);

/* Reports whether the record's own counters put it in the ratio state the band entry tests. */
extern "C" s32 fn_802F5138(_ENEMY_WORK* work) {
    switch (work->field_0x00A) {
    case 0:
    case 5:
        if ((f32)(s32)work->field_0x7A0 / (f32)(s32)work->field_0x7A4 <= lbl_8079AA88) {
            return 1;
        }
        return 0;
    default:
        return fn_8012ECF0();
    }
}

/* Clears the record's effect-slot bytes. */
extern "C" void fn_802F51C8(_ENEMY_WORK* work) {
    work->uv_model_0x328.slot_0x328 = 0;
    work->uv_model_0x328.created_0x33E = 0;
    work->uv_model_0x328.armed_0x329 = 0;
}

/* Arms the record's "model created" flag for the two idle modes the caller reports. */
extern "C" void fn_802F5464(_ENEMY_WORK* work, u32 action, u32 state_sub) {
    switch ((u8)action) {
    case 5: {
        u8 state = state_sub;

        if ((u32)(state - 7) <= 1) {
            work->uv_model_0x328.created_0x33E = 1;
        }
        break;
    }
    }
}

/* First step of the "idle" action: advances the state byte, starts the motion and asks for the
 * model, then retires one state later. */
extern "C" void fn_802F5B98(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 0);
        if (work->field_0x00A <= 2 || (u8)(work->field_0x00A - 5) <= 1) {
            em_mot_set_ck(work, 1, 20, 0);
        } else if (work->field_0x00A == 3 || work->field_0x00A == 4) {
            em_mot_set_ck(work, 10, 10, 0);
        }
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            em_action_finish(work);
        }
        break;
    }
}

/* Second step of the "idle" action: the same shape as the first, with the second motion set and the
 * band's own retire helper. */
extern "C" void fn_802F5C58(_ENEMY_WORK* work) {
    fn_80133C3C(work);
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 2);
        if (work->field_0x00A <= 2 || (u8)(work->field_0x00A - 5) <= 1) {
            em_mot_set_ck(work, 1, 20, 0);
        } else if (work->field_0x00A == 3 || work->field_0x00A == 4) {
            em_mot_set_ck(work, 10, 10, 0);
        }
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            fn_80128030(work);
        }
        break;
    }
}

/* Sub-state dispatcher of the "idle" action. */
extern "C" void fn_802F5D24(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0:
        fn_802F5B98(work);
        break;
    case 1:
        fn_802F5B98(work);
        break;
    case 2:
        fn_802F5B98(work);
        break;
    case 4:
        fn_802F5C58(work);
        break;
    }
}

/* Selects the record table of the UV-model placement by the record's state byte. */
extern "C" void fn_802F8768(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0:
        fn_802F7F78(work);
        break;
    case 1:
        fn_802F803C(work);
        break;
    case 2:
        fn_802F8100(work);
        break;
    case 3:
        fn_802F8290(work, 0);
        break;
    case 4:
        fn_802F8290(work, 1);
        break;
    }
}

/* Places the model record of a sub-state whose table is chosen by the record's mode. */
extern "C" void fn_802F87B8(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0xA:
        if ((u32)(work->field_0x00A - 3) <= 1) {
            em_se_tbl_play(work, lbl_805D6FC0, 2, 0xA);
        } else if (work->field_0x00A == 1) {
            em_se_tbl_play(work, lbl_805D6F98, 2, 0xA);
        } else {
            em_se_tbl_play(work, lbl_805D6F70, 2, 0xA);
        }
        break;
    case 0x89:
        if (work->field_0x00A == 1) {
            em_se_tbl_play(work, lbl_805D7040, 2, 0x89);
        } else {
            em_se_tbl_play(work, lbl_805D6FE8, 2, 0x89);
        }
        break;
    case 0x8A:
        em_se_tbl_play(work, lbl_805D70A0, 2, 0x8A);
        break;
    case 0xA1:
        if (work->field_0x00A == 1) {
            em_se_tbl_play(work, lbl_805D7078, 2, 0xA1);
        } else {
            em_se_tbl_play(work, lbl_805D7018, 2, 0xA1);
        }
        break;
    case 0xA2:
        if (work->field_0x00A == 1) {
            em_se_tbl_play(work, lbl_805D7078, 2, 0xA2);
        } else {
            em_se_tbl_play(work, lbl_805D7018, 2, 0xA2);
        }
        break;
    case 0x8B:
        if (work->field_0x00A == 1) {
            em_se_tbl_play(work, lbl_805D70F0, 2, 0x8B);
        } else {
            em_se_tbl_play(work, lbl_805D70C8, 2, 0x8B);
        }
        break;
    case 0x8C:
        em_se_tbl_play(work, lbl_805D7150, 2, 0x8C);
        break;
    case 0xAA:
        if (work->field_0x00A == 1) {
            em_se_tbl_play(work, lbl_805D7140, 2, 0xAA);
        } else {
            em_se_tbl_play(work, lbl_805D7118, 2, 0xAA);
        }
        break;
    case 0xB1:
        em_se_tbl_play(work, lbl_805D6F70, 2, 0xB1);
        break;
    case 0xCB:
        if (work->field_0x00A == 1) {
            em_se_tbl_play(work, lbl_805D6F98, 2, 0xCB);
        } else {
            em_se_tbl_play(work, lbl_805D6F70, 2, 0xCB);
        }
        break;
    default:
        fn_80128030(work);
        break;
    }
}

/* Places the second model record set of the same table family. */
extern "C" void fn_802F89F8(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0x0:
        if (work->field_0x00A == 1) {
            em_se_tbl_play_alt(work, lbl_805D71E8, 2, 0);
        } else {
            em_se_tbl_play_alt(work, lbl_805D7190, 2, 0);
        }
        break;
    case 0xA:
        if (work->field_0x00A == 1) {
            em_se_tbl_play_alt(work, lbl_805D71E8, 2, 0xA);
        } else {
            em_se_tbl_play_alt(work, lbl_805D7190, 2, 0xA);
        }
        break;
    case 0x33:
        if ((u32)(work->field_0x00A - 3) <= 1) {
            em_se_tbl_play_alt(work, lbl_805D72D0, 2, 0x33);
        } else if (work->field_0x00A == 1) {
            em_se_tbl_play_alt(work, lbl_805D7288, 2, 0x33);
        } else {
            em_se_tbl_play_alt(work, lbl_805D7240, 2, 0x33);
        }
        break;
    case 0x38:
        em_se_tbl_play_alt(work, lbl_805D7300, 2, 0x38);
        break;
    default:
        if (work->field_0x00A == 1) {
            em_se_tbl_play_alt(work, lbl_805D71E8, 2, 0xA);
        } else {
            em_se_tbl_play_alt(work, lbl_805D7190, 2, 0xA);
        }
        break;
    }
}

/* Advances the record's own motion. */
extern "C" u16 fn_802F8B24(_ENEMY_WORK* work) {
    return em_get_mot_no(work);
}

/* Reads one of the record's three effect-slot bytes by index. */
#pragma peephole off

extern "C" u8 fn_802F8B28(_ENEMY_WORK* work, u32 index) {
    u8 slot = index;

    switch (slot) {
    case 0:
        return work->uv_model_0x328.slot_0x328;
    case 1:
        return work->uv_model_0x328.created_0x33E;
    case 2:
        return work->uv_model_0x328.armed_0x329;
    default:
        return 0;
    }
}
#pragma peephole on

/* Retires the record's model when it is in the idle mode. */
extern "C" s32 fn_802F8B68(_ENEMY_WORK* work) {
    if (work->field_0x00A == 1) {
        fn_8033A920(20);
    }
    return 0;
}

/* First step of the band's own "place" action. */
extern "C" void fn_802F929C(_ENEMY_WORK* work) {
    em_move_mode_set(work, 2);
    em_mot_set(work, 10, 0, 0);
    fn_80128030(work);
}

/* Sub-state dispatcher of the band's own "place" action. */
extern "C" void fn_802F92E8(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0:
        fn_802F8F24(work);
        break;
    case 1:
        fn_802F929C(work);
        break;
    default:
        break;
    }
}

/* First step of the "roar" action: starts the second motion set and seeds the three effect sizes. */
extern "C" void fn_802F930C(_ENEMY_WORK* work) {
    fn_80133C3C(work);
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 2);
        em_mot_set(work, 2, 0, 0x46);
        em_demo_pos_set(work, lbl_8079AB74, lbl_8079AB78, lbl_8079AB7C);
        em_demo_rot_set(work, lbl_8079AB80, lbl_8079AB84, lbl_8079AB88);
        em_demo_key_apply(work, em_demo_frame_get(), lbl_805D7328, lbl_805D75C0, 5, 7);
        break;
    case 1:
        if (em_demo_time_ck(0x26C) == 0) {
            em_demo_key_apply(work, em_demo_frame_get(), lbl_805D7328, lbl_805D75C0, 5, 7);
        } else {
            em_demo_key_apply(work, em_demo_frame_get(), lbl_805D7328, lbl_805D7AC0, 5, 5);
        }
        break;
    }
}

/* First step of the "stance" action. */
extern "C" void fn_802F943C(_ENEMY_WORK* work) {
    fn_80133C3C(work);
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 2);
        em_mot_set(work, 1, 0, 0);
        em_demo_pos_set(work, lbl_8079AB8C, lbl_8079AB78, lbl_8079AB90);
        em_demo_rot_set(work, lbl_8079AAB0, lbl_8079AB94, lbl_8079AAB0);
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            fn_80128030(work);
        }
        break;
    }
}

/* First step of the "movement" action. */
extern "C" void fn_802F94EC(_ENEMY_WORK* work) {
    fn_80133C3C(work);
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 2);
        em_mot_set(work, 2, 0, 0x14);
        em_demo_pos_set(work, lbl_8079AB98, lbl_8079AB9C, lbl_8079ABA0);
        em_demo_key_apply(work, em_demo_frame_get(), lbl_805D7B80, lbl_805D7DA0, 5, 7);
        break;
    case 1:
        em_demo_key_apply(work, em_demo_frame_get(), lbl_805D7B80, lbl_805D7DA0, 5, 7);
        break;
    }
}

/* First step of the "crouch" action. */
extern "C" void fn_802F95C8(_ENEMY_WORK* work) {
    fn_80133C3C(work);
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 2);
        em_mot_set(work, 1, 0, 0);
        em_demo_pos_set(work, lbl_8079ABA4, lbl_8079ABA8, lbl_8079ABAC);
        em_demo_rot_set(work, lbl_8079AAB0, lbl_8079AAB0, lbl_8079ABB0);
        break;
    case 1:
        if (em_mot_end_ck(work) == 1) {
            fn_80128030(work);
        }
        break;
    }
}

/* Sub-state dispatcher of the "crouch" action. */
extern "C" void fn_802F9678(_ENEMY_WORK* work) {
    switch (work->state_sub) {
    case 0:
        fn_802F930C(work);
        break;
    case 1:
        fn_802F943C(work);
        break;
    case 2:
        fn_802F94EC(work);
        break;
    case 3:
        fn_802F95C8(work);
        break;
    }
}

/* Does nothing; the action's state byte is never 4. */
extern "C" void fn_802F9714(void) {
}

/* Reports whether the record's mode puts it in the retirable states. */
extern "C" s32 fn_802F9718(_ENEMY_WORK* work) {
    u8 mode = work->field_0x00A;

    if (mode == 0 || mode == 5 || mode == 3) {
        return fn_802F5138(work);
    }
    return 1;
}

/* Reports whether the record's mode puts it in one of the four "seated" modes. */
extern "C" s32 fn_802F9740(_ENEMY_WORK* work) {
    u8 mode = work->field_0x00A;

    if (mode == 0 || mode == 2 || mode == 5 || mode == 6) {
        return 1;
    }
    return 0;
}

#pragma peephole off

/* Reports whether the record's area lookup and attack timer allow the action. */
extern "C" s32 fn_802F9774(_ENEMY_WORK* work) {
    switch (stage_map_kind_get(work->field_0x1E0)) {
    case 1:
    case 3:
    case 11:
        if (work->field_0x011 != 0) {
            work->field_0x1FC = 1;
            work->field_0x1FE = 7;
            work->field_0x1FF = 0;
            return 1;
        }
        break;
    }
    return 0;
}

#pragma peephole off

/* The action dispatcher: picks the record's step machine by its action id. */
extern "C" void fn_802F96B4(_ENEMY_WORK* work) {
    switch (work->action) {
    case 0:
        fn_802F5D24(work);
        break;
    case 5:
        fn_802F7D28(work);
        break;
    case 7:
        fn_802F8768(work);
        break;
    case 10:
        fn_802F87B8(work);
        break;
    case 11:
        fn_802F89F8(work);
        break;
    case 12:
        fn_802F92E8(work);
        break;
    case 13:
        fn_802F9678(work);
        break;
    }
}


#pragma peephole off

/* Adjusts the record's blend weight by the mode's factor, then asks for the model when the record is
 * in one of the two idle modes. */
extern "C" void fn_802F51DC(_ENEMY_WORK* work, u32 arg) {
    switch (work->field_0x00A) {
    default:
        work->field_0x1CC = work->field_0x1CC * lbl_8079AA98;
        break;
    case 1:
        work->field_0x1CC = work->field_0x1CC * lbl_8079AA9C;
        break;
    case 2:
        work->field_0x1CC = work->field_0x1CC * lbl_8079AAA0;
        break;
    case 3:
        break;
    case 4:
        work->field_0x1CC = work->field_0x1CC * lbl_8079AAA4;
        break;
    case 5:
        work->field_0x1CC = work->field_0x1CC * lbl_8079AA98;
        break;
    case 6:
        work->field_0x1CC = work->field_0x1CC * lbl_8079AAA0;
        break;
    }
    if ((u8)arg == 0) {
        em_move_mode_set(work, 2);
        fn_80128A8C(work, 0, 4);
        switch (work->field_0x00A) {
        case 0:
        case 5:
            fn_802F6880(work);
            break;
        }
    }
}

#pragma peephole on

/* First step of the walk-into-place action: keeps the motion set and the model record alive one
 * state longer. */
extern "C" void fn_802F621C(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 2);
        em_turn_seq_start(work, lbl_80570740, 0, 1, 0);
        break;
    case 1:
        if (em_turn_seq_step(work, lbl_80570740) == 1) {
            fn_80128030(work);
        }
        break;
    }
}

/* The second walk-into-place action, on the second model record. */
extern "C" void fn_802F62A8(_ENEMY_WORK* work) {
    switch (work->state) {
    case 0:
        work->state = work->state + 1;
        em_move_mode_set(work, 2);
        em_turn_seq_start(work, lbl_80570780, 0, 1, 0);
        break;
    case 1:
        if (em_turn_seq_step(work, lbl_80570780) == 1) {
            fn_80128030(work);
        }
        break;
    }
}

