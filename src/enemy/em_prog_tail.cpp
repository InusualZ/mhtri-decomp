/* enemy/em_prog_tail.cpp - the program tail of the note-pane band: the NPC pane's frame, its sub-state machines and
 *   the motion-keyed joint effects, up to the em009 program.
 * RANGE. .text 0x80385EE0-0x803868DC (14 functions); .sdata2 0x8079BF90-0x8079BFF8, extab, extabindex.  Its helpers
 *   are declared by `enemy/em_prog_support.h`; `enemy/em009_act.cpp` starts at 0x803868DC.
 * SEAM. The left edge is a GUESS inside 0x80385E7C-0x80386028 (`enemy/em_prog_support.cpp`'s header); the pool
 *   repeats 41c00000 at 0x8079BFAC, first read by `note_pane_motion_effects`.
 * NAMES. `em_prog_tail` is a GUESS from the band's role.  GUESS (from the bodies): note_pane_npc_frame (0x80385EF8),
 *   note_pane_joint_effect_set (0x803864E4), note_pane_motion_effects (0x803865B4), and for the callees in
 *   `enemy/em_prog_support.cpp` note_pane_anim_pair_enter (0x80385AA8), note_pane_motion_frame_pass_ck (0x80385C80).
 * RESIDUALS. none.
 * SHAPES. File-wide `#pragma peephole off`; each sub-state machine is a `switch` with `+= 1` on the state byte.
 */

#include "types.h"
#include "enemy/em_prog_support.h"
#include "nw4r/math.h"
#include "sound/mhchar.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8011D448.h"
#include "enemy/fn_8012E968.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "sound/fn_800DD1F0.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "enemy/note_work.h" /* NoteWork and the pane/slot/layout types (rule 1) */
#include "lobby/lb_quest_screen.h" /* note_pane_get_motion (rule 2: the owner's header) */
#include "ef/pRoot.h"
#include "lobby/lb_server_sel_trans.h" /* fn_803C7EAC / fn_803C7F88 (owner's header, rule 2) */
#include "Runtime.PPCEABI.H/CPlusLibPPC.h" /* __construct_array (owner's header, rule 2) */
#include "ef/mtx34_trans_add.h"
#include "ef/mtx34_trans_get.h"
#include "Pl/Pl_master_ck.h" /* Pl_act_ck (the owner's header, rule 2) */
#include "ef/get_move_work_adrs.h"
#include "ef/eft001.h" /* eft_rot_vec_copy (the owner's header, rule 2) */
#include "enemy/fn_80382310.h" /* note_pane_anim_pair_ck / qn_get_motion_no (the owner's header, rule 2) */
#include "fn_8004CAD8/mtx.h" /* MTX34_ctor */
#include "ef/eft004_set.h" /* eft004_set */

/* the mode-1 frame check with the frame pair retail passes through in f1/f2 (the owner reads only the slot). */
#define note_pane_motion_frame_pass_ck_c1 ((u32 (*)(NoteWork*, u32, f32, f32))note_pane_motion_frame_pass_ck)

/* Retail compares each sub-state as `cmpwi` + `beq` into its case body and keeps each decremented timer as `subi` +
 * `cmpwi`; the peephole pass fuses the latter into `subic.`. */
#pragma peephole off


/* This unit's own members, declared with the argument view their call sites use. */
extern "C" {
void fn_80385EE0(NoteWork* self);
void note_pane_npc_init(NoteWork* self);
void note_pane_npc_frame(NoteWork* self);
void fn_80386028(NoteWork* self);
void fn_803860B8(NoteWork* self);
void fn_8038613C(NoteWork* self);
void fn_80386160(NoteWork* self);
void fn_803861F8(NoteWork* self);
void fn_80386328(NoteWork* self);
void fn_803863C8(NoteWork* self);
void fn_80386484(NoteWork* self);
void fn_803864C0(NoteWork* self);
void note_pane_joint_effect_set(NoteWork* self, u8 type, u32 joint, nw4r::math::VEC3* ofs, f32 scale);
}

/* 0x80385EE0 (0xC): returns the pane to animation pair (0, 0). */
extern "C" void fn_80385EE0(NoteWork* self) {
    note_pane_set_anim_pair(self, 0, 0);
}

/* 0x80385EEC (0xC): clears the active flag and returns the pane to pair (0, 0). */
extern "C" void note_pane_npc_init(NoteWork* self) {
    self->field_0x001 = 0;
    fn_80385EE0(self);
}

/* 0x80385EF8 (0x130): the NPC pane's frame: idles while its player is absent or a demo runs, otherwise follows the
 * player's action 9 and steps the pane's sub-state machine. */
extern "C" void note_pane_npc_frame(NoteWork* self) {
    _PLW* players = (_PLW*)get_move_work_adrs(2);
    _PLW* plw;

    if (players == NULL) {
        return;
    }
    plw = &players[self->field_0x198];
    if (plw->slot_active == 0 || event_demo_ck() == 1) {
        note_pane_anim_pair_enter(self, 0, 0);
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 1, 0, 0);
        self->field_0x001 = 0;
        return;
    }
    if (Pl_act_ck(plw, 9, 0) == 1 && note_pane_anim_pair_ck(self, 1, 1) == 0) {
        note_pane_anim_pair_enter(self, 1, 1);
    }
    fn_803864C0(self);
    if (self->field_0x1A1 == 1) {
        fn_803864C0(self);
        self->field_0x1A1 = 0;
    }
    if ((u32)note_pane_anim_pair_ck(self, 0, 0) == 1) {
        self->field_0x001 = 0;
    } else {
        self->field_0x001 = 1;
    }
}

/* 0x80386028 (0x90): pair (0, 0): plays motion 1 for 200 frames at full step, then goes idle. */
extern "C" void fn_80386028(NoteWork* self) {
    switch (self->field_0x169) {
    case 0:
        self->field_0x169 += 1;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 1, 0, 0);
        self->field_0x16C = 200;
        self->field_0x1B0 = 1.0f;
        break;
    case 1:
        if (--self->field_0x16C < 0) {
            fn_80385EE0(self);
        }
        break;
    }
}

/* 0x803860B8 (0x84): pair (0, 1): plays motion 3 once, then switches to pair (1, 2). */
extern "C" void fn_803860B8(NoteWork* self) {
    switch (self->field_0x169) {
    case 0:
        self->field_0x169 += 1;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 3, 0, 0);
        break;
    case 1:
        if (note_pane_motion_end_ck(self) == 1) {
            note_pane_set_anim_pair(self, 1, 2);
        }
        break;
    }
}

/* 0x8038613C (0x24): runs the pair-0 sub-machine the second byte names. */
extern "C" void fn_8038613C(NoteWork* self) {
    switch (self->field_0x19F) {
    case 0:
        fn_80386028(self);
        return;
    case 1:
        fn_803860B8(self);
        return;
    }
}

/* 0x80386160 (0x98): pair (1, 0): plays motion 2 for 600 frames, turning the pane by 112 a frame. */
extern "C" void fn_80386160(NoteWork* self) {
    switch (self->field_0x169) {
    case 0:
        self->field_0x169 += 1;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 2, 0, 0);
        self->field_0x16C = 600;
        break;
    case 1:
        self->field_0x18C = (u16)(self->field_0x18C + 112);
        if (--self->field_0x16C <= 0) {
            fn_80385EE0(self);
        }
        break;
    }
}

/* 0x803861F8 (0x130): pair (1, 1): follows the pane's player while it holds action 9. */
extern "C" void fn_803861F8(NoteWork* self) {
    _PLW* plw = (_PLW*)get_move_work_adrs(2) + self->field_0x198;

    switch (self->field_0x169) {
    case 0:
        self->field_0x169 += 1;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 2, 0, 0);
        self->field_0x1B0 = 1.0f;
        copyVec3(&self->vec_0x170, &plw->vec_0x03C);
        eft_rot_vec_copy((_CP_VECTOR*)&self->field_0x188, &plw->rot_0x54);
        qn_chr_flag_set((_QNPC_W*)self, 90);
        break;
    case 1:
        if (Pl_act_ck(plw, 9, 0) == 1) {
            copyVec3(&self->vec_0x170, &plw->vec_0x03C);
            eft_rot_vec_copy((_CP_VECTOR*)&self->field_0x188, &plw->rot_0x54);
            qn_chr_flag_set((_QNPC_W*)self, 90);
        } else if (Pl_act_ck(plw, 9, 1) == 1) {
            note_pane_set_anim_pair(self, 0, 1);
        } else {
            note_pane_set_anim_pair(self, 1, 2);
        }
        break;
    }
}

/* 0x80386328 (0xA0): pair (1, 2): plays motion 2/4 for 53 frames, turning the pane by 688 a frame. */
extern "C" void fn_80386328(NoteWork* self) {
    switch (self->field_0x169) {
    case 0:
        self->field_0x169 += 1;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 2, 4, 0);
        self->field_0x16C = 53;
        break;
    case 1:
        self->field_0x18C = (u16)(self->field_0x18C + 688);
        if (--self->field_0x16C <= 0) {
            note_pane_set_anim_pair(self, 1, 3);
        }
        break;
    }
}

/* 0x803863C8 (0xBC): pair (1, 3): plays motion 2/4 for 100 frames, fading the step out over the last five. */
extern "C" void fn_803863C8(NoteWork* self) {
    switch (self->field_0x169) {
    case 0:
        self->field_0x169 += 1;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 2, 4, 0);
        self->field_0x16C = 100;
        break;
    case 1:
        if (--self->field_0x16C <= 0) {
            fn_80385EE0(self);
        }
        if (self->field_0x16C < 5) {
            self->field_0x1B0 = (f32)self->field_0x16C / 5.0f;
        }
        break;
    }
}

/* 0x80386484 (0x3C): runs the pair-1 sub-machine the second byte names. */
extern "C" void fn_80386484(NoteWork* self) {
    switch (self->field_0x19F) {
    case 0:
        fn_80386160(self);
        return;
    case 1:
        fn_803861F8(self);
        return;
    case 2:
        fn_80386328(self);
        return;
    case 3:
        fn_803863C8(self);
        return;
    }
}

/* 0x803864C0 (0x24): runs the sub-machine set the first pair byte names. */
extern "C" void fn_803864C0(NoteWork* self) {
    switch (self->field_0x19D) {
    case 0:
        fn_8038613C(self);
        return;
    case 1:
        fn_80386484(self);
        return;
    }
}

/* 0x803864E4 (0xD0): places eft004 effect `type` at `ofs` in the model's joint `joint`, raised by the pane's
 * effect height. */
extern "C" void note_pane_joint_effect_set(NoteWork* self, u8 type, u32 joint, nw4r::math::VEC3* ofs, f32 scale) {
    nw4r::math::VEC3 pos;
    nw4r::math::MTX34 mtx;

    VEC3_ctor(&pos);
    MTX34_ctor(&mtx);
    mhchar_joint_mtx_get(&self->model, joint, &mtx);
    mulVecMat(ofs, &mtx);
    mtx34_trans_add(&mtx, ofs);
    mtx34_trans_get(&mtx, &pos);
    pos.y = 5.0f + self->effect_y_ofs_0x1A4;
    eft004_set(type, &pos, scale, self->field_0x18C, self->field_0x195);
}

/* 0x803865B4 (0x328): the motion-keyed effect cues: motion 2's two frame sets, motion 3's three frames. */
extern "C" void note_pane_motion_effects(NoteWork* self) {
    nw4r::math::VEC3 ofs;

    VEC3_ctor(&ofs);
    switch (qn_get_motion_no((_QNPC_W*)self)) {
    case 2:
        if (note_pane_motion_frame_pass_ck_c1(self, 0, 4.0f, 0.0f) || note_pane_motion_frame_pass_ck_c1(self, 0, 14.0f, 0.0f) ||
            note_pane_motion_frame_pass_ck_c1(self, 0, 24.0f, 0.0f) || note_pane_motion_frame_pass_ck_c1(self, 0, 34.0f, 0.0f) ||
            note_pane_motion_frame_pass_ck_c1(self, 0, 44.0f, 0.0f) || note_pane_motion_frame_pass_ck_c1(self, 0, 54.0f, 0.0f) ||
            note_pane_motion_frame_pass_ck_c1(self, 0, 64.0f, 0.0f) || note_pane_motion_frame_pass_ck_c1(self, 0, 74.0f, 0.0f)) {
            setVector3(&ofs, 90.0f, 0.0f, 0.0f);
            note_pane_joint_effect_set(self, 4, 2, &ofs, 1.2f);
        }
        if (note_pane_motion_frame_pass_ck_c1(self, 0, 6.0f, 0.0f) || note_pane_motion_frame_pass_ck_c1(self, 0, 16.0f, 0.0f) ||
            note_pane_motion_frame_pass_ck_c1(self, 0, 26.0f, 0.0f) || note_pane_motion_frame_pass_ck_c1(self, 0, 36.0f, 0.0f) ||
            note_pane_motion_frame_pass_ck_c1(self, 0, 46.0f, 0.0f) || note_pane_motion_frame_pass_ck_c1(self, 0, 56.0f, 0.0f) ||
            note_pane_motion_frame_pass_ck_c1(self, 0, 66.0f, 0.0f)) {
            setVector3(&ofs, -90.0f, 0.0f, 0.0f);
            note_pane_joint_effect_set(self, 4, 2, &ofs, 1.2f);
        }
        break;
    case 3:
        if (note_pane_motion_frame_pass_ck_c1(self, 0, 16.0f, 0.0f)) {
            setVector3(&ofs, 0.0f, 0.0f, 0.0f);
            note_pane_joint_effect_set(self, 18, 22, &ofs, 1.0f);
        }
        if (note_pane_motion_frame_pass_ck_c1(self, 0, 46.0f, 0.0f)) {
            setVector3(&ofs, 0.0f, 0.0f, 0.0f);
            note_pane_joint_effect_set(self, 8, 2, &ofs, 1.2f);
        }
        if (note_pane_motion_frame_pass_ck_c1(self, 0, 60.0f, 0.0f)) {
            setVector3(&ofs, 0.0f, 0.0f, 0.0f);
            note_pane_joint_effect_set(self, 6, 22, &ofs, 0.5f);
        }
        break;
    }
}
