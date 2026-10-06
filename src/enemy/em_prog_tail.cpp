/* enemy/em_prog_tail.cpp - the program tail of the note-pane band: its sub-state machines, up to the em009 program.
 * RANGE. .text 0x80385EE0-0x803868DC (14 functions); .sdata2 0x8079BF90-0x8079BFF8, extab, extabindex.  Its helpers
 *   are declared by `enemy/em_prog_support.h`; `enemy/em009_act.cpp` starts at 0x803868DC.
 * SEAM. The left edge is a GUESS inside 0x80385E7C-0x80386028 (`enemy/em_prog_support.cpp`'s header); the pool
 *   repeats 41c00000 at `lbl_8079BFAC`, first read by `fn_803865B4`.
 * NAMES. `em_prog_tail` is a GUESS from the band's role.
 * RESIDUALS. 3 rows unwritten: 0x80385EF8-0x80386028, 0x803864E4-0x803868DC.
 *  - `fn_80386028`, `fn_803860B8`, `fn_80386160`, `fn_803861F8`, `fn_80386328`, `fn_803863C8`: retail tests the
 *    sub-state with `cmpwi` + `beq` into the case body, ours with `cmplwi` + `bne` around it; all but `fn_803860B8`
 *    and `fn_803861F8` also fuse the timer decrement into `subic.` where retail keeps `subi` + `cmpwi`.
 *   flipcheck: `.sdata2`/`.text`/extab/extabindex short of the claim.
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

extern "C" {

struct _QNPC_W;
}

/* This unit's own members, declared with the argument view their call sites use (the record type is `NoteWork`
 * unless the signature says otherwise). */

extern "C" {
void fn_80386028(NoteWork* self);
void fn_803860B8(NoteWork* self);
void fn_8038613C(NoteWork* self);
void fn_80386160(NoteWork* self);
void fn_803861F8(NoteWork* self);
void fn_80386328(NoteWork* self);
void fn_803863C8(NoteWork* self);
void fn_80386484(NoteWork* self);
void fn_803864C0(NoteWork* self);
void fn_80385EE0(NoteWork* self);
}

/* Callees whose map names are C++ manglings (rule 9: declare the owner's real signature, never the
 * mangled spelling). */
struct _PLW;
void* get_move_work_adrs(u8 kind);
s32 Pl_act_ck(_PLW* plw, u8 a, u16 b);

extern "C" {
void eft_rot_vec_copy(void* dst, void* src);
extern f32 lbl_8079BF94;
extern f32 lbl_8079BF90;
}

/* 0x80385EE0 */
extern "C" void fn_80385EE0(NoteWork* self) {
    note_pane_set_anim_pair(self, 0, 0);
}

/* 0x80385EEC */
extern "C" void fn_80385EEC(NoteWork* self) {
    self->field_0x001 = 0;
    fn_80385EE0(self);
}

/* 0x8038613C */
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

/* 0x80386484 */
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

/* 0x803864C0 */
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

/* 0x80386028 - action sub-machine 0. */
extern "C" void fn_80386028(NoteWork* self) {
    u8 v = self->field_0x169;
    if (v == 0) {
        self->field_0x169 = v + 1;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 1, 0, 0);
        self->field_0x16C = 200;
        self->field_0x1B0 = lbl_8079BF90;
    } else if (v == 1) {
        if (--self->field_0x16C < 0) {
            fn_80385EE0(self);
        }
    }
}

/* 0x803860B8 - action sub-machine 1. */
extern "C" void fn_803860B8(NoteWork* self) {
    u8 v = self->field_0x169;
    if (v == 0) {
        self->field_0x169 = v + 1;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 3, 0, 0);
    } else if (v == 1) {
        if (note_pane_motion_end_ck(self) == 1) {
            note_pane_set_anim_pair(self, 1, 2);
        }
    }
}

/* 0x80386160 - action sub-machine 2. */
extern "C" void fn_80386160(NoteWork* self) {
    u8 v = self->field_0x169;
    if (v == 0) {
        self->field_0x169 = v + 1;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 2, 0, 0);
        self->field_0x16C = 600;
    } else if (v == 1) {
        self->field_0x18C = (u16)(self->field_0x18C + 112);
        if (--self->field_0x16C <= 0) {
            fn_80385EE0(self);
        }
    }
}

/* 0x803861F8 - action sub-machine 3. */
extern "C" void fn_803861F8(NoteWork* self) {
    u8* work = (u8*)get_move_work_adrs(2) + self->field_0x198 * 2848;
    u8 v = self->field_0x169;
    if (v == 0) {
        self->field_0x169 = v + 1;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 2, 0, 0);
        self->field_0x1B0 = lbl_8079BF90;
        copyVec3(&self->vec_0x170, (nw4r::math::VEC3*)(work + 0x3C));
        eft_rot_vec_copy(&self->field_0x188, work + 0x54);
        qn_chr_flag_set((_QNPC_W*)self, 90);
    } else if (v == 1) {
        if (Pl_act_ck((_PLW*)work, 9, 0) == 1) {
            copyVec3(&self->vec_0x170, (nw4r::math::VEC3*)(work + 0x3C));
            eft_rot_vec_copy(&self->field_0x188, work + 0x54);
            qn_chr_flag_set((_QNPC_W*)self, 90);
        } else {
            if (Pl_act_ck((_PLW*)work, 9, 1) == 1) {
                note_pane_set_anim_pair(self, 0, 1);
            } else {
                note_pane_set_anim_pair(self, 1, 2);
            }
        }
    }
}

/* 0x80386328 - action sub-machine 4. */
extern "C" void fn_80386328(NoteWork* self) {
    u8 v = self->field_0x169;
    if (v == 0) {
        self->field_0x169 = v + 1;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 2, 4, 0);
        self->field_0x16C = 53;
    } else if (v == 1) {
        self->field_0x18C = (u16)(self->field_0x18C + 688);
        if (--self->field_0x16C <= 0) {
            note_pane_set_anim_pair(self, 1, 3);
        }
    }
}

/* 0x803863C8 - action sub-machine 5. */
extern "C" void fn_803863C8(NoteWork* self) {
    u8 v = self->field_0x169;
    if (v == 0) {
        self->field_0x169 = v + 1;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 2, 4, 0);
        self->field_0x16C = 100;
    } else if (v == 1) {
        if (--self->field_0x16C <= 0) {
            fn_80385EE0(self);
        }
        if (self->field_0x16C < 5) {
            self->field_0x1B0 = (f32)self->field_0x16C / lbl_8079BF94;
        }
    }
}

