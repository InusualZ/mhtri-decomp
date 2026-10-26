/*
 * Pl/pl_motion.cpp - the player actor's motion layer: the act-style `_PLW` state handlers and their class-7
 *   dispatcher (0x80288CEC-0x8028B524), the player scene/quest/move-work driver (0x8028B524-0x8028EF30) and the
 *   root-work accessors (0x8028EF30 on).
 * RANGE. .text 0x80288CEC-0x8028F44C (74 functions); .ctors 0x8056F36C, .data 0x805CBFC0-0x805CDC44, .bss
 *   0x806AB830-0x806AB848, .sdata 0x807921B0-0x80792270, .sbss 0x80794B38-0x80794B58, .sdata2 0x8079A270-0x8079A310,
 *   extab, extabindex.  Left seam: the unit's `.sdata2` run starts with `fn_80288CEC`'s first constant
 *   (`pl_motion_f32_zero` = 0.0f) where the preceding unit's run ends.  `Pl/pl_hit_sphere.cpp` follows it.
 * NAMES. The file name is a GUESS from the motion-step entry points (`fn_80288CEC` steps the player's motion,
 *   `pl_motion_set` loads the motion resources); `pl_motion_set` and `get_gm_daynight` are the map's names.  The root
 *   work `get_move_work_adrs(0)` hands back is `_PL_ROOT` (`Pl/fn_80288CEC.h`), the record `enemy/em_common.cpp`
 *   defines again as `_PLAYER_ROOT` (rule 1).
 *   GUESS (from each body and its callers): quest_spawn_rec_find, quest_spawn_rec_find_in, quest_move_sub_flag_get
 *   GUESS (from the dispatch chain, `Pl/pl_act_class3.cpp`'s scheme): pl_act_class7_dispatch (0x8028B330, the
 *   `field_0x002` class-7 dispatcher on `act_no`) and pl_act_class7_handler_00..26 (0x80288E68-0x8028B298, address
 *   order); pl_act_class7_handler_NN_rows (`PlMotionRow` tables) and pl_act_class7_handler_NN_steps (step tables)
 *   name the handler that reads them; pl_motion_f32_<value> the `.sdata2` constants
 * RESIDUALS. 34 functions unwritten (objdiff scores them 0) in 14 runs: 0x80288E68-0x8028912C, 0x8028921C-0x802893D4,
 *   0x802896F8-0x8028992C, 0x80289CD4-0x80289FF0, 0x8028A400-0x8028A62C, 0x8028A760-0x8028A8D8,
 *   0x8028A9D4-0x8028ADB8, 0x8028AF50-0x8028B0F4, 0x8028B524-0x8028BCF4, 0x8028BD98-0x8028C570,
 *   0x8028C5B4-0x8028D574, 0x8028D5F4-0x8028DF24, 0x8028DF58-0x8028E4F0, 0x8028E528-0x8028EF30; most are jump-table
 *   state machines of 100-450 instructions, and `pl_motion_set` needs the local string-object shape.  3 partial,
 *   none with a recorded cause: `fn_8028DF24`, `quest_spawn_rec_find`, `quest_spawn_rec_find_in`.
 *  - the class-7 step/row tables (`pl_act_class7_handler_NN_steps`/`_rows`) and the `pl_motion_f32_*` constants are
 *    loaded as `extern` map symbols: the object emits none of them, and dtk's split of the TU's `.data` carries
 *    relocations into `fn_8015FD84` inside some tables, so `.data` is not matched.  The two jump tables are
 *    compiler-local (`@NNNN`) against the map's `jumptable_805CBFC0`/`jumptable_805CC278`.
 *  - flipcheck: the object emits no `.bss` (0x18 claimed), `.ctors` (0x4), `.sbss` (0x20) or `.sdata` (0xC0); `.text`
 *    0x1C44, `.data` 0x1A0, `.sdata2` 0x18, extab 0x118 and extabindex 0x1A4 against the claims 0x6760, 0x1C84, 0xA0,
 *    0x228 and 0x33C; every compared section differs, and the `.sdata2` layout lacks the seven `pl_motion_f32_*`
 *    symbols.
 * SHAPES. The two 0xC-byte records `fn_8028F400` fills stay `lbl_806AB830`/`lbl_806AB83C`: a `VEC3[]` view folds the
 *   second into `lbl_806AB830 + 0xC` and loses its relocation.
 */

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"
#include "Pl/pl_act.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/fn_802693C4.h"
#include "Pl/pl_motion.h"
#include "Pl/fn_80288CEC.h" /* `PlBox` and `_PL_ROOT`, shared with `Pl/pl_hit_sphere.cpp` and `Pl/pl_coll.cpp` (rule 1) */
#include "Pl/fn_80262940.h" /* player_init_data_load (the owner's header, rule 2) */
#include "Pl/fn_802840DC.h" /* PlMotionRow, pl_act_attack_set, pl_act_motion_row_apply (rule 2) */
#include "ef/eft029_fx.h" /* eft029_set_scale (rule 2) */
#include "ef/fn_800CDB2C.h"
#include "enemy/em_pop.h" /* quest_flag_8_ck, quest_flag_80_ck (the owner's header, rule 2) */
#include "fn_80047398.h" /* `arena_userdata_apply` (rule 2) */
#include "quest/arenatask.h" /* `arena_user_data_buf` (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "Network/network_pat_control.h" /* the owner's header (rule 2) */
#include "quest/quest_entry.h" /* quest_monsters_release (the owner's header, rule 2) */

/* The object `fn_8028BCF4` releases: three `se_handle_clear` handles, 4 bytes apart.
 * size: 0x144 (lower bound) */
struct PlHandleSet {
    /* +0x000 */ u8 unused_0x000[0x138];
    /* +0x138 */ void* handle_0x138;
    /* +0x13C */ void* handle_0x13C;
    /* +0x140 */ void* handle_0x140;
};

/* ------------------------------------------------------------------------------------------------ *
 * Callees.
 *
 * The map spelled these with an argument list, so they are manglings and belong at C++ scope with
 * the real signature, so the front-end reproduces the map's name (rule 9).  The bare `fn_XXXXXXXX`
 * names are the map's own placeholders and stay `extern "C"`.
 * ------------------------------------------------------------------------------------------------ */

/* ef/system_core.cpp owns the pool accessors (`PlayMode_ck` is declared in its header `ef/fn_800CDB2C.h`). */
void* get_move_work_adrs(u8 index);

/* `Pl_zanzo_set` and `Pl_act_ck` are `Pl/pl_act.cpp`'s (`Pl/pl_act.h`, `Pl/Pl_master_ck.h`). */


/* `Pl_frame_check` is `Pl/pl_act.cpp`'s (`Pl/fn_802693C4.h`); `player_init_data_load` is `Pl/player_control.cpp`'s
 * (`Pl/fn_80262940.h`). */

extern "C" {




void draw_shape_stage_load(u8 idx);
void se_handle_clear(void* handle);
void eft_common_load(void);

u32 pl_hit_result_best_get(void* self);
u32 stage_map_kind_get(u8 idx);
void camera_pl_quake_req(void* self, u32 sub);
void em_pop_res_load(u8 idx);

}

/* The two small-data seeds `fn_8028F400` fills (see the unit header); `setVec3` is `ef.h`'s `(Vec*, f32, f32, f32)`,
 * so the call site casts. */
extern u8 lbl_806AB830[];
extern u8 lbl_806AB83C[];

/* This unit's own `.data` step and row tables and `.sdata2` constants: the object does not emit them yet, so the
 * bodies load the map's symbols (see the unit header). */
extern "C" {
extern PlMotionRow pl_act_class7_handler_04_rows[];
extern PlMotionRow pl_act_class7_handler_08_rows[];
extern PlMotionRow pl_act_class7_handler_11_rows[];
extern PlMotionRow pl_act_class7_handler_13_rows[];
extern u32 pl_act_class7_handler_01_steps[];
extern u32 pl_act_class7_handler_03_steps[];
extern u32 pl_act_class7_handler_04_steps[];
extern u32 pl_act_class7_handler_05_steps[];
extern u32 pl_act_class7_handler_07_steps[];
extern u32 pl_act_class7_handler_08_steps[];
extern u32 pl_act_class7_handler_09_steps[];
extern u32 pl_act_class7_handler_11_steps[];
extern u32 pl_act_class7_handler_12_steps[];
extern u32 pl_act_class7_handler_13_steps[];
extern u32 pl_act_class7_handler_14_steps[];
extern u32 pl_act_class7_handler_16_steps[];
extern u32 pl_act_class7_handler_18_steps[];
extern u32 pl_act_class7_handler_24_steps[];
extern const f32 pl_motion_f32_zero;
extern const f32 pl_motion_f32_minus_one;
extern const f32 pl_motion_f32_18;
extern const f32 pl_motion_f32_22;
extern const f32 pl_motion_f32_one;
extern const f32 pl_motion_f32_28;
extern const f32 pl_motion_f32_128;
}

/* The class-7 handlers whose bodies are not written yet, with the arguments `pl_act_class7_dispatch` passes. */
extern "C" {
void pl_act_class7_handler_00(_PLW* self, s32 kind, s32 variant);
void pl_act_class7_handler_02(_PLW* self, s32 kind);
void pl_act_class7_handler_06(_PLW* self, s32 kind);
void pl_act_class7_handler_10(_PLW* self, s32 kind);
void pl_act_class7_handler_15(_PLW* self, s32 kind);
void pl_act_class7_handler_17(_PLW* self, s32 kind);
void pl_act_class7_handler_19(_PLW* self, s32 kind);
void pl_act_class7_handler_20(_PLW* self, s32 kind);
void pl_act_class7_handler_23(_PLW* self, s32 kind);
}

/* ------------------------------------------------------------------------------------------------ *
 * Bodies, in address order.
 * ------------------------------------------------------------------------------------------------ */

/* Every bare `fn_XXXXXXXX` is the map's own (unmangled) name, so the definitions carry C linkage;
 * `get_gm_daynight` is mangled in the map and stays at C++ scope below. */
extern "C" {

/* 0x80288CEC - the player's primary-act update: `act_no` selects the primary-act flag through a jump table,
 * `pl_hit_result_best_get` latches `+0x565` and `Pl_frame_check` arms `+0x566` and hands the player to `fn_802BE568` once. */
void fn_80288CEC(_PLW* self, u32 sub, f32 dt) {
    u32 primary = 0;
    s32 result;

    if (self->field_0x565 == 0) {
        switch (self->act_no) {
        case 0x10:
        case 0x12:
        case 0x14:
        case 0x17:
        case 0x18:
        case 0x1A:
        case 0x1B:
        case 0x21:
        case 0x23:
        case 0x25:
        case 0x28:
        case 0x29:
        case 0x2B:
        case 0x2C:
        case 0x2F:
        case 0x3A:
            primary = 1;
            break;
        default:
            primary = 0;
            break;
        }

        result = (u8)pl_hit_result_best_get(self);
        switch (result) {
        case 2:
            self->field_0x565 = 1;
            break;
        case 3:
        case 4:
            self->field_0x565 = 1;
            break;
        case 0:
        case 1:
        case 5:
            self->field_0x565 = 1;
            if (primary == 0) {
                if (self->kind_0x09 == 3) {
                    pl_act_enter(self, 4, 0x39, 0x4080);
                } else {
                    pl_act_enter(self, 4, 0xF, 0x4000);
                }
            }
            break;
        }
    }

    if (self->field_0x566 == 0 && self->field_0x565 == 0 && dt > pl_motion_f32_zero &&
        Pl_frame_check(self, 0, dt, pl_motion_f32_zero) == 1) {
        self->field_0x566 = 1;
        camera_pl_quake_req(self, 0);
    }
}

/* 0x80288E58 / 0x80288E60 - the two `sub` bindings of the primary-act update. */
void fn_80288E58(_PLW* self, f32 dt) {
    fn_80288CEC(self, 0, dt);
}

void fn_80288E60(_PLW* self, f32 dt) {
    fn_80288CEC(self, 1, dt);
}

/* 0x8028912C (0xF0): runs the class-7 act whose step table is `pl_act_class7_handler_01_steps`: arms the motion and
 * its attack, then trails an afterimage at frame 18 and hands over at the motion's end. */
void pl_act_class7_handler_01(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x451, 4, 0);
        pl_act_set_frame_timer(self);
        pl_act_attack_set(self, 2, 0);
        pl_act_set_motion_gate(self, 60);
        Pl_act_set_step_table(self, (u32)pl_act_class7_handler_01_steps, 0);
        break;
    case 1:
        if (Pl_frame_check(self, 0, pl_motion_f32_18, pl_motion_f32_zero) == 1) {
            Pl_zanzo_set(self, 9, 3);
        }
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            fn_80288E58(self, pl_motion_f32_minus_one);
        }
        break;
    }
}

/* 0x802893D4 (0x144): runs the two-motion class-7 act whose step table is `pl_act_class7_handler_03_steps`, trailing
 * an afterimage at frame 22 of the second motion. */
void pl_act_class7_handler_03(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        Pl_chr_set_attr_default(self, 0x45C, 2, 0);
        pl_act_set_frame_timer(self);
        pl_act_set_motion_gate(self, 40);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x453, 0, 20);
            pl_act_set_frame_timer(self);
            pl_act_attack_set(self, 5, 0);
            Pl_act_set_step_table(self, (u32)pl_act_class7_handler_03_steps, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            fn_80288E58(self, pl_motion_f32_minus_one);
            if (Pl_frame_check(self, 0, pl_motion_f32_22, pl_motion_f32_zero) == 1) {
                Pl_zanzo_set(self, 5, 3);
            }
        }
        break;
    }
}

/* 0x80289518 (0x13C): runs the class-7 act for row `kind` of `pl_act_class7_handler_04_rows`, trailing an afterimage
 * at frame 22 of the step motion. */
void pl_act_class7_handler_04(_PLW* self, s32 kind) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        pl_act_motion_row_apply(self, 0, &pl_act_class7_handler_04_rows[kind], (u32)pl_act_class7_handler_04_steps);
        pl_act_set_motion_gate(self, 40);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_act_motion_row_apply(self, 1, &pl_act_class7_handler_04_rows[kind],
                                    (u32)pl_act_class7_handler_04_steps);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            fn_80288E58(self, pl_motion_f32_minus_one);
            if (Pl_frame_check(self, 0, pl_motion_f32_22, pl_motion_f32_zero) == 1) {
                Pl_zanzo_set(self, 17, 3);
            }
        }
        break;
    }
}

/* 0x80289654 (0xA4): runs the class-7 act whose step table is `pl_act_class7_handler_05_steps`, handing over at the
 * motion's end. */
void pl_act_class7_handler_05(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 0x4C5, 0, 0);
        Pl_act_set_step_table(self, (u32)pl_act_class7_handler_05_steps, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 10, 0);
        }
        break;
    }
}

/* 0x8028992C (0x134): runs the two-motion class-7 act whose step table is `pl_act_class7_handler_07_steps`, trailing
 * an afterimage at frame 18 of the second motion. */
void pl_act_class7_handler_07(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        Pl_chr_set_attr_default(self, 0x463, 2, 0);
        pl_act_set_motion_gate(self, 60);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x451, 0, 16);
            pl_act_attack_set(self, 3, 0);
            Pl_act_set_step_table(self, (u32)pl_act_class7_handler_07_steps, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            if (Pl_frame_check(self, 0, pl_motion_f32_18, pl_motion_f32_zero) == 1) {
                Pl_zanzo_set(self, 9, 3);
            }
            fn_80288E58(self, pl_motion_f32_minus_one);
        }
        break;
    }
}

/* 0x80289A60 (0x178): runs the class-7 act for row `kind` of `pl_act_class7_handler_08_rows`: drains the gauge by 20
 * and resets the three eft029 effect scales, then trails an afterimage at frame 28 of the step motion. */
void pl_act_class7_handler_08(_PLW* self, s32 kind) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        pl_act_motion_row_apply(self, 0, &pl_act_class7_handler_08_rows[kind], (u32)pl_act_class7_handler_08_steps);
        pl_act_add_class7_gauge(self, -20);
        pl_act_set_motion_gate(self, 60);
        eft029_set_scale(self, 11, pl_motion_f32_one);
        eft029_set_scale(self, 12, pl_motion_f32_one);
        eft029_set_scale(self, 13, pl_motion_f32_one);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_act_motion_row_apply(self, 1, &pl_act_class7_handler_08_rows[kind],
                                    (u32)pl_act_class7_handler_08_steps);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            fn_80288E58(self, pl_motion_f32_minus_one);
            if (Pl_frame_check(self, 0, pl_motion_f32_28, pl_motion_f32_zero) == 1) {
                Pl_zanzo_set(self, 8, 4);
            }
        }
        break;
    }
}

/* 0x80289BD8 (0xFC): runs the class-7 act whose step table is `pl_act_class7_handler_09_steps`, trailing an
 * afterimage at frame 22. */
void pl_act_class7_handler_09(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        Pl_chr_set_attr_default(self, 0x453, 4, 0);
        pl_act_attack_set(self, 6, 0);
        pl_act_set_motion_gate(self, 60);
        Pl_act_set_step_table(self, (u32)pl_act_class7_handler_09_steps, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            fn_80288E58(self, pl_motion_f32_minus_one);
            if (Pl_frame_check(self, 0, pl_motion_f32_22, pl_motion_f32_zero) == 1) {
                Pl_zanzo_set(self, 5, 3);
            }
        }
        break;
    }
}

/* 0x80289FF0 (0x128): runs the class-7 act for row `kind` of `pl_act_class7_handler_11_rows`; row 3 also steps the
 * act timer. */
void pl_act_class7_handler_11(_PLW* self, s32 kind) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        self->field_0x018 = 1;
        pl_act_motion_row_apply(self, 0, &pl_act_class7_handler_11_rows[kind], (u32)pl_act_class7_handler_11_steps);
        pl_act_set_motion_gate(self, 60);
        if (kind == 3) {
            pl_act_timer_step(self);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_act_motion_row_apply(self, 1, &pl_act_class7_handler_11_rows[kind],
                                    (u32)pl_act_class7_handler_11_steps);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            fn_80288E58(self, pl_motion_f32_minus_one);
        }
        break;
    }
}

/* 0x8028A118 (0xD0): runs the class-7 act whose step table is `pl_act_class7_handler_12_steps`. */
void pl_act_class7_handler_12(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x482, 4, 0);
        pl_act_attack_set(self, 15, 0);
        pl_act_set_motion_gate(self, 60);
        Pl_act_set_step_table(self, (u32)pl_act_class7_handler_12_steps, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            fn_80288E58(self, pl_motion_f32_minus_one);
        }
        break;
    }
}

/* 0x8028A1E8 (0x110): runs the class-7 act for row `kind` of `pl_act_class7_handler_13_rows`. */
void pl_act_class7_handler_13(_PLW* self, s32 kind) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        pl_act_motion_row_apply(self, 0, &pl_act_class7_handler_13_rows[kind], (u32)pl_act_class7_handler_13_steps);
        pl_act_set_motion_gate(self, 60);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_act_motion_row_apply(self, 1, &pl_act_class7_handler_13_rows[kind],
                                    (u32)pl_act_class7_handler_13_steps);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            fn_80288E58(self, pl_motion_f32_minus_one);
        }
        break;
    }
}

/* 0x8028A2F8 (0x108): runs the two-motion class-7 act whose step table is `pl_act_class7_handler_14_steps`. */
void pl_act_class7_handler_14(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x48A, 4, 0);
        pl_act_set_motion_gate(self, 60);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x484, 0, 10);
            pl_act_attack_set(self, 17, 0);
            Pl_act_set_step_table(self, (u32)pl_act_class7_handler_14_steps, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            fn_80288E58(self, pl_motion_f32_minus_one);
        }
        break;
    }
}

/* 0x8028A62C (0x134): runs the two-motion class-7 act whose step table is `pl_act_class7_handler_16_steps`, trailing
 * an afterimage at frame 18 of the second motion. */
void pl_act_class7_handler_16(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x491, 4, 0);
        pl_act_set_motion_gate(self, 60);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x482, 0, 16);
            pl_act_attack_set(self, 20, 0);
            Pl_act_set_step_table(self, (u32)pl_act_class7_handler_16_steps, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            if (Pl_frame_check(self, 0, pl_motion_f32_18, pl_motion_f32_zero) == 1) {
                Pl_zanzo_set(self, 9, 3);
            }
            fn_80288E58(self, pl_motion_f32_minus_one);
        }
        break;
    }
}

/* 0x8028A8D8 (0xFC): runs the class-7 act whose step table is `pl_act_class7_handler_18_steps`, trailing an
 * afterimage at frame 22. */
void pl_act_class7_handler_18(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x484, 4, 0);
        pl_act_attack_set(self, 22, 0);
        Pl_act_set_step_table(self, (u32)pl_act_class7_handler_18_steps, 0);
        pl_act_set_motion_gate(self, 60);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            fn_80288E58(self, pl_motion_f32_minus_one);
            if (Pl_frame_check(self, 0, pl_motion_f32_22, pl_motion_f32_zero) == 1) {
                Pl_zanzo_set(self, 5, 3);
            }
        }
        break;
    }
}

/* 0x8028ADB8 (0x100): runs the gauge-spending class-7 act on motion 0x467 (attack 30, gauge -100) and enters act 0x30
 * at frame 128. */
void pl_act_class7_handler_21(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        Pl_chr_set_attr_default(self, 0x467, 4, 0);
        pl_act_attack_set(self, 30, 2);
        pl_act_set_motion_gate(self, 60);
        pl_act_add_class7_gauge(self, -100);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            fn_80288E60(self, pl_motion_f32_minus_one);
            if (Pl_frame_check(self, 0, pl_motion_f32_128, pl_motion_f32_zero) == 1) {
                self->field_0x018 = 0;
                pl_act_enter(self, 4, 0x30, 2);
            }
        }
        break;
    }
}

/* 0x8028AEB8 (0x98): runs the one-motion class-7 act on motion 0x467, handing over at the motion's end. */
void pl_act_class7_handler_22(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        Pl_chr_set_attr_default(self, 0x467, 0, 0x80);
        self->field_0x018 = 0;
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    }
}

/* 0x8028B0F4 (0xA4): runs the class-7 act whose step table is `pl_act_class7_handler_24_steps`, handing over at the
 * motion's end. */
void pl_act_class7_handler_24(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        Pl_chr_set_attr_default(self, 0x4F7, 0, 0);
        Pl_act_set_step_table(self, (u32)pl_act_class7_handler_24_steps, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 10, 0);
        }
        break;
    }
}

/* 0x8028B198 (0x100): runs the gauge-spending class-7 act on motion 0x495 (attack 31, gauge -100) and enters act 0x3B
 * at frame 128. */
void pl_act_class7_handler_25(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x495, 4, 0);
        pl_act_attack_set(self, 31, 2);
        pl_act_set_motion_gate(self, 60);
        pl_act_add_class7_gauge(self, -100);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            fn_80288E60(self, pl_motion_f32_minus_one);
            if (Pl_frame_check(self, 0, pl_motion_f32_128, pl_motion_f32_zero) == 1) {
                self->field_0x018 = 0;
                pl_act_enter(self, 4, 0x3B, 2);
            }
        }
        break;
    }
}

/* 0x8028B298 (0x98): runs the one-motion class-7 act on motion 0x495, handing over at the motion's end. */
void pl_act_class7_handler_26(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x495, 0, 0x80);
        self->field_0x018 = 0;
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        break;
    }
}

/* 0x8028B330 (0x1F4): runs the class-7 act handler `act_no` selects, with the variant the act number implies; an
 * act number past the table hands the motion slot over. */
void pl_act_class7_dispatch(_PLW* self) {
    switch (self->act_no) {
    case 0:
        pl_act_class7_handler_00(self, 0, 0);
        break;
    case 1:
        pl_act_class7_handler_00(self, 0, 1);
        break;
    case 2:
        pl_act_class7_handler_00(self, 1, 0);
        break;
    case 3:
        pl_act_class7_handler_00(self, 2, 0);
        break;
    case 4:
        pl_act_class7_handler_01(self);
        break;
    case 5:
        pl_act_class7_handler_02(self, 0);
        break;
    case 6:
        pl_act_class7_handler_02(self, 1);
        break;
    case 7:
        pl_act_class7_handler_03(self);
        break;
    case 8:
        pl_act_class7_handler_04(self, 0);
        break;
    case 9:
        pl_act_class7_handler_04(self, 1);
        break;
    case 10:
        pl_act_class7_handler_04(self, 2);
        break;
    case 11:
        pl_act_class7_handler_04(self, 3);
        break;
    case 12:
        pl_act_class7_handler_04(self, 4);
        break;
    case 13:
        pl_act_class7_handler_04(self, 5);
        break;
    case 14:
        pl_act_class7_handler_04(self, 6);
        break;
    case 15:
        pl_act_class7_handler_05(self);
        break;
    case 16:
        pl_act_class7_handler_06(self, 0);
        break;
    case 17:
        pl_act_class7_handler_06(self, 1);
        break;
    case 18:
        pl_act_class7_handler_06(self, 2);
        break;
    case 19:
        pl_act_class7_handler_06(self, 3);
        break;
    case 20:
        pl_act_class7_handler_06(self, 4);
        break;
    case 21:
        pl_act_class7_handler_06(self, 5);
        break;
    case 22:
        pl_act_class7_handler_07(self);
        break;
    case 23:
        pl_act_class7_handler_08(self, 0);
        break;
    case 24:
        pl_act_class7_handler_08(self, 1);
        break;
    case 25:
        pl_act_class7_handler_09(self);
        break;
    case 26:
        pl_act_class7_handler_10(self, 0);
        break;
    case 27:
        pl_act_class7_handler_10(self, 1);
        break;
    case 28:
        pl_act_class7_handler_11(self, 0);
        break;
    case 29:
        pl_act_class7_handler_12(self);
        break;
    case 30:
        pl_act_class7_handler_13(self, 0);
        break;
    case 31:
        pl_act_class7_handler_14(self);
        break;
    case 32:
        pl_act_class7_handler_11(self, 1);
        break;
    case 33:
        pl_act_class7_handler_15(self, 0);
        break;
    case 34:
        pl_act_class7_handler_15(self, 1);
        break;
    case 35:
        pl_act_class7_handler_15(self, 2);
        break;
    case 36:
        pl_act_class7_handler_15(self, 3);
        break;
    case 37:
        pl_act_class7_handler_15(self, 4);
        break;
    case 38:
        pl_act_class7_handler_15(self, 5);
        break;
    case 39:
        pl_act_class7_handler_16(self);
        break;
    case 40:
        pl_act_class7_handler_17(self, 0);
        break;
    case 41:
        pl_act_class7_handler_17(self, 1);
        break;
    case 42:
        pl_act_class7_handler_18(self);
        break;
    case 43:
        pl_act_class7_handler_19(self, 0);
        break;
    case 44:
        pl_act_class7_handler_19(self, 1);
        break;
    case 45:
        pl_act_class7_handler_20(self, 0);
        break;
    case 46:
        pl_act_class7_handler_20(self, 1);
        break;
    case 47:
        pl_act_class7_handler_21(self);
        break;
    case 48:
        pl_act_class7_handler_22(self);
        break;
    case 49:
        pl_act_class7_handler_13(self, 1);
        break;
    case 50:
        pl_act_class7_handler_13(self, 2);
        break;
    case 51:
        pl_act_class7_handler_13(self, 3);
        break;
    case 52:
        pl_act_class7_handler_13(self, 4);
        break;
    case 53:
        pl_act_class7_handler_13(self, 5);
        break;
    case 54:
        pl_act_class7_handler_11(self, 2);
        break;
    case 55:
        pl_act_class7_handler_23(self, 0);
        break;
    case 56:
        pl_act_class7_handler_23(self, 1);
        break;
    case 57:
        pl_act_class7_handler_24(self);
        break;
    case 58:
        pl_act_class7_handler_25(self);
        break;
    case 59:
        pl_act_class7_handler_26(self);
        break;
    case 60:
        pl_act_class7_handler_11(self, 3);
        break;
    default:
        Pl_act_set_motion_slot(self, 0, 4, 0);
        break;
    }
}

/* 0x8028BCF4 - release the three `se_handle_clear` handles the mode object carries. */
void fn_8028BCF4(PlHandleSet* self) {
    if (self == 0) {
        return;
    }
    if (self->handle_0x138 != 0) {
        se_handle_clear(self->handle_0x138);
    }
    if (self->handle_0x13C != 0) {
        se_handle_clear(self->handle_0x13C);
    }
    if (self->handle_0x140 != 0) {
        se_handle_clear(self->handle_0x140);
    }
}

/* 0x8028BD54 - is the root work in mode 2? */
u32 root_mode2_ck(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return root->mode_0x00 == 2;
}

/* 0x8028D574 - the `flag_0x114 == 1` predicate, 1 when the work is absent. */
u32 quest_move_idle_ck(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 1;
    }
    return root->flag_0x114 <= 0;
}

/* 0x8028D5B8 - store the flag. */
void root_wait_frames_set(s32 value) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root != 0) {
        root->flag_0x114 = value;
    }
}

/* 0x8028C570 - the mode object's per-scene player data init. */
void fn_8028C570(_PL_ROOT* self) {
    player_init_data_load();
    eft_common_load();
    quest_monsters_release();
    em_pop_res_load(self->area_no_0xED);
    draw_shape_stage_load(self->area_no_0xED);
}

/* 0x8028DF24 - arm the mode object's sequence: state 0 -> 1, then hand over. */
void fn_8028DF24(_PL_ROOT* self) {
    switch (self->seq_0x01) {
    case 0:
    case 1:
        self->seq_0x01 = self->seq_0x01 + 1;
        self->mode_0x00 = 1;
        self->seq_0x01 = 0;
        break;
    }
}

/* 0x8028E4F0 - the root work's `+0x22D9` byte. */
u8 quest_move_sub_flag_get(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return root->field_0x22D9;
}

/* 0x8028EF30 - the root work's byte array at +0x136, indexed by the caller's byte. */
u8 Pl_area_flag_get(u8 index) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return root->bytes_0x136[index];
}

/* 0x8028EF7C - find the root entry whose key is `key`: the six fixed slots at +0x2274 first, then
 * the area record `quest_arena_item_count_get()` names, then the single extra pointer at +0x2258. */
PlRootEntry* quest_spawn_rec_find(u32 key) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    PlRootEntry* entry;
    u32 index;
    u32 count;
    int i;

    if (root == 0) {
        return 0;
    }

    if (key < 0x20) {
        PlRootEntry* slot = root->sparse_0x2274;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        return 0;
    }

    index = quest_arena_item_count_get();
    entry = root->recs_0x154[index].entries_0x00;
    count = root->rec_counts_0x2154[index];
    for (i = 0; i < (int)count; i++) {
        if (entry[i].key_0x00 == key) {
            return &entry[i];
        }
    }
    if (root->extra_0x2258 != 0) {
        if (root->extra_0x2258->key_0x00 == key) {
            return root->extra_0x2258;
        }
    }
    return 0;
}

/* 0x8028F0B4 - the same search, but the caller names the area index itself. */
PlRootEntry* quest_spawn_rec_find_in(u32 key, u32 index) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    PlRootEntry* entry;
    u32 count;
    int i;

    if (root == 0) {
        return 0;
    }

    if (key < 0x20) {
        PlRootEntry* slot = root->sparse_0x2274;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        slot++;
        if (slot->key_0x00 == key) {
            return slot;
        }
        return 0;
    }

    entry = root->recs_0x154[index].entries_0x00;
    count = root->rec_counts_0x2154[index];
    for (i = 0; i < (int)count; i++) {
        if (entry[i].key_0x00 == key) {
            return &entry[i];
        }
    }
    if (root->extra_0x2258 != 0) {
        if (root->extra_0x2258->key_0x00 == key) {
            return root->extra_0x2258;
        }
    }
    return 0;
}

/* 0x8028F1E8 - hand the caller's `+0x08` byte to the `arena_user_data_buf` table as a 256-byte stride. */
void fn_8028F1E8(u8* self) {
    arena_userdata_apply(arena_user_data_buf + ((u32)self[8] << 8), self);
}

/* 0x8028F204 - the `+0x22D7` byte == 1 predicate. */
u32 pl_root_lock_ck(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return (s8)root->field_0x22D7 == 1;
}

/* 0x8028F24C - the `+0xE9` area byte through `stage_map_kind_get`. */
u32 pl_map_kind_get(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return stage_map_kind_get(root->area_no_0xE9);
}

/* 0x8028F288 - the root work's `+0x22DB` byte. */
u8 fn_8028F288(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return root->field_0x22DB;
}

/* 0x8028F2F8 - arm the mode sequence: 6 for the "no argument" call, 4 otherwise. */
void pl_quest_end_request(u32 arg) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return;
    }
    if (quest_flag_8_ck(0) == 0) {
        return;
    }
    if (arg == 0) {
        root->seq_0xFB = 6;
    } else {
        root->seq_0xFB = 4;
    }
}

/* 0x8028F368 - hand the mode work over to the network move system; 1 on success. */
u32 pl_net_move_handover(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    u8* mode;

    if (root == 0) {
        return 0;
    }
    mode = root->mode_work_0xDC;
    if (mode == 0) {
        return 0;
    }
    if (quest_flag_80_ck(0) == 0) {
        return 0;
    }
    mode[0x6A3E] = 4;
    if (isServerSelectState() != 1) {
        root->seq_0xFC = 4;
    }
    return 1;
}

/* 0x8028F400 - seed the two `setVec3` vectors from the small-data literals. */
void fn_8028F400(void) {
    setVec3((nw4r::math::VEC3*)lbl_806AB830, 573.0f, -1265.0f, 3392.0f);
    setVec3((nw4r::math::VEC3*)lbl_806AB83C, 1413.0f, -571.0f, 1740.0f);
}

} /* extern "C" */

/* 0x8028F2C0 - the stage's day/night byte (`get_move_work_adrs(0)` + 0x22D8), 0 when absent. */
u8 get_gm_daynight(void) {
    _PL_ROOT* root = (_PL_ROOT*)get_move_work_adrs(0);
    if (root == 0) {
        return 0;
    }
    return root->field_0x22D8;
}
