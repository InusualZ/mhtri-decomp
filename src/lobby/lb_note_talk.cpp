/* lobby/lb_note_talk.cpp - the lobby NPC's note-pane talk program.
 * RANGE. .text 0x8038EC44-0x8038F2BC (11 functions); .sdata2 0x8079C298-0x8079C2A8 (-849.0f, 263.0f, 252.0f and the
 *   1.0f the wait step stores), extab 0x800181C4-0x800181F4, extabindex 0x8003810C-0x80038154 (six framed functions).
 *   No .data, .sdata or .bss: the program reads only its `NoteWork` (`enemy/note_work.h`) and calls out.
 * FLAGS. `cflags_lobby`; `#pragma pool_data off` and `#pragma peephole off`, as the neighbouring `lobby/lb_quest_ui.cpp`.
 *   A separate unit because retail pools 1.0f twice in this run (0x8079C2A4 read only here, 0x8079C2AC read by the scene
 *   effect of `lobby/lb_quest_ui.cpp`); one translation unit keeps one copy of a value.
 * NAMES. `lb_note_talk` is the registered GUESS (the program is driven by `enemy/em_prog_support.cpp`'s note-pane
 *   records and talks through the lobby's `npc_talk_*`); every function name is a GUESS from the body.
 *   GUESS: `note_talk_step`, `note_turn_step`, `note_idle_set`, `note_talk_init`, `note_talk_frame`,
 *   GUESS: `note_talk_wait_step`, `note_talk_greet_step`, `note_talk_react_step`, `note_talk_mode_step`,
 *   GUESS: `note_talk_noop`, `note_talk_state_step`
 * RESIDUALS. None: every row matches and the object is the target's (`.sdata2` is the unit's own pool).
 * SHAPES. `note_turn_step` declares its three locals first and assigns them after, which issues the `+0x18C` load
 *   before the `+0x1A8` one.
 */

#pragma pool_data off
#pragma peephole off

#include "types.h"
#include "enemy/note_work.h"
#include "enemy/em_prog_support.h"
#include "menu/note_pane_player_near_ck.h"
#include "Pl/pl_act_stage_latch_set.h"
#include "Pl/pl_skill.h"
#include "Pl/pl_item_add.h"
#include "Pl/fn_80262940.h"
#include "sound/fn_800D7F54.h"
#include "lobby/lb_server_sel_trans.h"
#include "fn_8004CAD8.h"

extern "C" {

/* The unit's own entry points the bodies below reach before their definitions. */
void note_talk_state_step(NoteWork* self);

/* --------------------------------------------------------------------------------------------- *
 * The note-pane NPC talk program.
 * --------------------------------------------------------------------------------------------- */

/* 0x8038EC44 (0x284): Runs one frame of the NPC's talk: re-arms the player's talk wait, opens the talk the remaining
 * count selects and closes it when the message window does; 0 once the talk has ended. */
s32 note_talk_step(NoteWork* self) {
    _PLW* plw = my_player_work_get();

    if (userdata_progress_flag_ck(19) == 1) {
        plw->talk_left_0x669 = 3;
    }
    if (plw->talk_wait_0x668 == 0) {
        plw->talk_left_0x669--;
        npc_talk_end();
        return 0;
    }
    plw->talk_wait_0x668 = 5;
    switch (plw->talk_left_0x669) {
    default:
        plw->talk_wait_0x668 = 0;
        return 0;
    case 1:
        switch (self->talk_step_0x1B5) {
        case 0:
            self->talk_step_0x1B5++;
            npc_talk_start(0, 0, 0, 1);
            se_talk_point_set(&self->vec_0x170);
            break;
        case 1:
            if (npc_talk_active_ck() == 0) {
                plw->talk_wait_0x668 = 0;
                self->talk_step_0x1B5 = 0;
                return 0;
            }
            break;
        }
        break;
    case 2:
        switch (self->talk_step_0x1B5) {
        case 0:
            if (Pl_item_timer_get(plw, 29) >= 1) {
                self->talk_step_0x1B5 = 1;
                npc_talk_start(0, 9, 0, 1);
                se_talk_point_set(&self->vec_0x170);
            } else {
                self->talk_step_0x1B5 = 2;
                npc_talk_start(0, 14, 0, 1);
                se_talk_point_set(&self->vec_0x170);
            }
            break;
        case 1:
            if (npc_talk_active_ck() == 0) {
                plw->talk_wait_0x668 = 0;
                self->talk_step_0x1B5 = 0;
                pl_item_add(plw, 29, -1);
                userdata_progress_flag_set(19);
                pl_model_state_set(plw, 1, 40, 0);
                return 0;
            }
            break;
        case 2:
            if (npc_talk_active_ck() == 0) {
                plw->talk_wait_0x668 = 0;
                plw->talk_left_0x669--;
                self->talk_step_0x1B5 = 0;
                return 0;
            }
            break;
        }
        break;
    case 3:
        switch (self->talk_step_0x1B5) {
        case 0:
            self->talk_step_0x1B5++;
            npc_talk_start(0, 18, 0, 1);
            se_talk_point_set(&self->vec_0x170);
            break;
        case 1:
            if (npc_talk_active_ck() == 0) {
                plw->talk_wait_0x668 = 0;
                plw->talk_left_0x669--;
                self->talk_step_0x1B5 = 0;
                return 0;
            }
            break;
        }
        break;
    }
    return 1;
}

/* 0x8038EEC8 (0x54): Moves the work's 16-bit angle one 1820-step towards its target, snapping when it is inside one
 * step, and wrapping through 0 the way the record's own 16-bit field does. */
void note_turn_step(NoteWork* self) {
    u32 target;
    u32 cur;
    u16 diff;
    cur = self->field_0x18C;
    target = self->field_0x1A8;
    diff = (u16)(target - (u16)cur);
    if ((u16)(diff + 1820) < 3640) {
        self->field_0x18C = target;
    } else if (diff < 0x8000) {
        self->field_0x18C = (u16)(cur + 1820);
    } else {
        self->field_0x18C = (u16)(cur - 1820);
    }
}

/* 0x8038EF1C (0xC): Puts the pane back on its idle animation pair. */
void note_idle_set(NoteWork* self) {
    note_pane_set_anim_pair(self, 0, 0);
}

/* 0x8038EF28 (0x74): Places the NPC at its spot facing +0x4000, hides model part 22 and starts it idling. */
void note_talk_init(NoteWork* self) {
    self->field_0x001 = 1;
    self->vec_0x170.x = -849.0f;
    self->vec_0x170.y = 263.0f;
    self->vec_0x170.z = 252.0f;
    self->field_0x18C = 0x4000;
    self->field_0x1A8 = 0x4000;
    self->talk_step_0x1B5 = 0;
    self->model.setVisibility(22, false);
    note_idle_set(self);
}

/* 0x8038EF9C (0x50): Runs the program's frame: the state arm, once more when the work raised its one-shot flag, then
 * the turn step. */
void note_talk_frame(NoteWork* self) {
    note_talk_state_step(self);
    if (self->field_0x1A1 == 1) {
        note_talk_state_step(self);
        self->field_0x1A1 = 0;
    }
    note_turn_step(self);
}

/* 0x8038EFEC (0xB0): The waiting mode: starts the idle motion and a 200-frame countdown, idles when it runs out and
 * goes to the greeting pair once the player comes near. */
void note_talk_wait_step(NoteWork* self) {
    switch (self->field_0x169) {
    case 0:
        self->field_0x169++;
        note_pane_mode_set(self, 0);
        note_pane_motion_set(self, 1, 4, 0);
        self->field_0x16C = 200;
        self->field_0x1B0 = 1.0f;
        break;
    case 1:
        if (--self->field_0x16C < 0) {
            note_idle_set(self);
        }
        if (note_pane_player_near_ck(self) == 1) {
            note_pane_set_anim_pair(self, 0, 1);
        }
        break;
    }
}

/* 0x8038F09C (0xB4): The greeting mode: starts the greeting motion, turns to the player and talks until the talk
 * ends, then goes back to the idle pair 2. */
void note_talk_greet_step(NoteWork* self) {
    _PLW* plw = my_player_work_get();

    switch (self->field_0x169) {
    case 0:
        self->field_0x169++;
        note_pane_mode_set(self, 0);
        note_pane_motion_start(self, 2, 4, 0);
        note_talk_step(self);
        self->field_0x1A8 = calcVecAng2(&self->vec_0x170, &plw->vec_0x03C);
        break;
    case 1:
        if (note_talk_step(self) == 0) {
            note_pane_set_anim_pair(self, 0, 2);
        }
        break;
    }
}

/* 0x8038F150 (0x114): The reaction mode: plays the reaction motion, turns to the player when near and talks; idles
 * when the motion or the talk ends. */
void note_talk_react_step(NoteWork* self) {
    _PLW* plw = my_player_work_get();

    switch (self->field_0x169) {
    case 0:
        self->field_0x169++;
        note_pane_mode_set(self, 0);
        note_pane_motion_start(self, 3, 6, 0);
        npc_talk_flag_set(45);
        break;
    case 1:
        if (note_pane_player_near_ck(self) == 1) {
            self->field_0x169++;
            self->field_0x1A8 = calcVecAng2(&self->vec_0x170, &plw->vec_0x03C);
        } else if (note_pane_motion_end_ck(self) != 0) {
            note_idle_set(self);
        }
        break;
    case 2:
        if (note_talk_step(self) == 0) {
            note_idle_set(self);
        } else if (note_pane_motion_end_ck(self) != 0) {
            note_pane_set_anim_pair(self, 0, 1);
        }
        break;
    }
}

/* 0x8038F264 (0x30): Runs the mode the work's +0x19F byte selects. */
void note_talk_mode_step(NoteWork* self) {
    switch (self->field_0x19F) {
    case 0:
        note_talk_wait_step(self);
        break;
    case 1:
        note_talk_greet_step(self);
        break;
    case 2:
        note_talk_react_step(self);
        break;
    }
}

/* 0x8038F294 (0x4): The empty state arm. */
void note_talk_noop(void) {
}

/* 0x8038F298 (0x24): Runs the arm the work's +0x19D byte selects. */
void note_talk_state_step(NoteWork* self) {
    switch (self->field_0x19D) {
    case 0:
        note_talk_mode_step(self);
        break;
    case 1:
        note_talk_noop();
        break;
    }
}

}  /* extern "C" */
