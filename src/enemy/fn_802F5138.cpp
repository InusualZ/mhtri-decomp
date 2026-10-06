/* enemy/fn_802F5138.cpp - the per-action state machines of one enemy's AI: the action dispatcher `fn_802F96B4` (on
 *   `action`, +0x1E5), the `state_sub` dispatchers, and the bodies that drive the motion API and place the effects.
 * RANGE. .text 0x802F5138-0x802F9994 (61 functions); .ctors 0x8056F390-0x8056F394 (the static initializer
 *   `fn_802F9898`), .rodata 0x80570700-0x805707C0, .data 0x805D6D10-0x805D8210 (the effect record tables and the switch
 *   tables), .bss 0x806BE078-0x806BE0C0, .sdata 0x80792880-0x807928D0, .sdata2 0x8079AA88-0x8079ABC0, extab,
 *   extabindex.
 *   `enemy/em_sub_state_prog.cpp` continues the band at 0x802F9994.
 * SEAM. Unproven: the band sits between `ef/eft035.cpp` and `lobby/fn_802FA9A0.cpp` and no `__FILE__` name reaches
 *   either edge; `tudiscover`'s model spans it from `hud/cockpit_quest.cpp`'s anchor with an untaken seam inside.
 * NAMES. The map stem; the map and the dump have no other name for the range.
 * RESIDUALS. 34 rows unwritten: 0x802F52F4-0x802F5464, 0x802F548C-0x802F5B98, 0x802F5D60-0x802F621C,
 *   0x802F6334-0x802F8768, 0x802F8B9C-0x802F929C, 0x802F97F0-0x802F9994 (the static initializer among them).
 *  - `fn_802F5B98`, `fn_802F5C58`: retail folds the `mode` test into an unsigned range check whose first block it
 *    lays out after the whole compare chain (the if/else-if, `>= 3 && <= 4` and dense `switch` spellings score lower);
 *  - `fn_802F87B8`, `fn_802F89F8`: ours compares the byte unsigned (`cmplwi`) where retail keeps a signed `cmpwi`;
 *  - `fn_802F51DC`: retail carries one more `b`.
 *   flipcheck: `.bss`/`.ctors`/`.data`/`.rodata`/`.sdata` claimed, not emitted; `.sdata2`/`.text`/extab/extabindex
 *   short of the claim.
 * SHAPES. `#pragma peephole off` around `fn_802F8B28` and from `fn_802F9774` through `fn_802F51DC` only (a file-wide
 *   pragma costs `fn_802F5B98`; docs/enemy.md); `switch` on a byte where retail keeps a signed `cmpwi` chain, and
 *   `(f32)(s32)` where retail keeps the `xoris` conversion.
 */

#include "enemy/em_mot_finished_ck.h" /* em_mot_finished_ck (rule 2: the owner's header) */
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
/* the call sites use the argument-less view: a cast call is the same direct call. */
#define em_mot_finished_ck_c1 ((u32 (*)(void))em_mot_finished_ck)

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
        return em_mot_finished_ck_c1();
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

