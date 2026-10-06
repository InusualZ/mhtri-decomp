/* lobby/lb_quest_screen.cpp - the quest/multiplayer screen band after `menu/multi_result.cpp`, headed by the note-pane
 *   helpers.
 * RANGE. .text 0x803A3A50-0x803AA4A4 (95 functions); .data 0x805F2038-0x805F2940 (from the two private switch tables
 *   `jumptable_805F2038`/`jumptable_805F2068`), .sdata 0x80793530-0x807935D0, .sdata2 0x8079C448-0x8079C4C0, extab,
 *   extabindex.  Not one TU: a `--max-bytes` slice of five bands (note pane, lobby item/quest screen, resource loading,
 *   model/quest bookkeeping, enemy area logic; docs/lobby.md).  The unwind runs tile with `menu/multi_result.cpp`'s.
 * NAMES. Module `lobby` and `lb_quest_screen` are a GUESS from the biggest band (`lobby_w`, `lb_param_w`, `Screen_w`,
 *   `lobby_world_block`, `LbStr`, `lb_item_get_data`, `subTransSet`, `menu_cursor_step`); no `__FILE__` string or dump
 *   name covers the range.  Every function name is a GUESS from its body; the note-pane sub-states from the pane's
 *   state order.
 * RESIDUALS. 86 rows unwritten: 0x803A3A50-0x803A4170, 0x803A4214-0x803A4DD4, 0x803A4EC0-0x803A4F7C,
 *   0x803A5100-0x803AA4A4.  Partial: `note_pane_pos_step`, `note_timer_ready_ck`, `note_value_to_slot`,
 *   `note_slot_to_value`.  The lobby screen band 0x803A52A4-0x803A75D8 waits on a view merge: it reads
 *   `lobby_w.menu_0xAC` +0x28/+0x2A/+0x3C/+0xD0 with meanings `LbMenuWork` (`unsplit/lobby.h`) does not give them, and
 *   re-typing that header moves every includer (playbook 60); the 0x803A7718-0x803A7E1C slot group's array is at
 *   `lobby_w` +0xCC, inside `LbLobbyWork`'s +0x0C0 talk block.
 *   flipcheck: `.data` 0x28 against 0x908; `.sdata`/`.sdata2` claimed, not emitted; `.text`/extab/extabindex short of
 *   the claim; the `.sdata`/`.sdata2` pool is shared with `menu/multi_result.cpp` (fold candidate).
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/note_work.h"
#include "enemy/fn_80382310.h"
#include "ef/fn_800CDB2C.h"
#include "lobby/lb_quest_screen.h"
#include "mh3_pad/vec3.h"
#include "unsplit/lobby.h"

/* -------------------------------------------------------------------------------------------------
 * The note-pane band's own still-unwritten members: the nine tail-called sub-states
 * `note_pane_state_dispatch` selects, in the pane's state order.
 * ------------------------------------------------------------------------------------------------- */
extern "C" {
void note_pane_state_dispatch(NoteWork* self);
void note_pane_state_0(NoteWork* self);
void note_pane_state_1(NoteWork* self);
void note_pane_state_2(NoteWork* self);
void note_pane_state_3(NoteWork* self);
void note_pane_state_4(NoteWork* self);
void note_pane_state_5(NoteWork* self);
void note_pane_state_6(NoteWork* self, u32 variant);
void note_pane_state_9(NoteWork* self);
}

/* -------------------------------------------------------------------------------------------------
 * The note-pane band (0x803A3A50..0x803A52A4).
 * ------------------------------------------------------------------------------------------------- */

/* The lobby lifetime/timer record at `lobby_world_block + 0x5270` that `note_timer_*` reads: a flag, a
 * two-refresh counter and a four-entry field whose refresh loop runs 50 times per entry (the retail
 * source's own leftover nesting, which the target's codegen reproduces exactly).
 * size: 0x18 */
typedef struct NoteTimer {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 flag_0x01;
    /* +0x02 */ u8 pad_0x02[0x04 - 0x02];
    /* +0x04 */ u16 field_0x04;
    /* +0x06 */ s8 count_0x06;
    /* +0x07 */ u8 pad_0x07[0x10 - 0x07];
    /* +0x10 */ u16 field_0x10[4];
} NoteTimer;

/* 0x803A4170 - steps the pane's eased position towards its target (4004 units while its animation pair is (0,4), else
 * 1820), snapping on within one step. */
extern "C" void note_pane_pos_step(NoteWork* self) {
    u32 step = (note_pane_anim_pair_ck(self, 0, 4) != 0) ? 4004 : 1820;
    u32 target = self->field_0x1A8;
    u32 cur = (u16)self->field_0x18C;
    u16 avail = (u16)(target - cur);
    if ((u16)(avail + step) < (u32)(step * 2)) {
        self->field_0x18C = target;
    } else if (avail < 0x8000) {
        self->field_0x18C = (u16)(cur + step);
    } else {
        self->field_0x18C = (u16)(cur - step);
    }
}

/* 0x803A4208 - puts the pane on its animation pair (0,1). */
extern "C" void note_pane_anim_pair_0_1(NoteWork* self) {
    note_pane_set_anim_pair(self, 0, 1);
}

/* 0x803A4E30 - the pane's null sub-state. */
extern "C" void note_pane_idle(NoteWork* self) {
}

/* 0x803A4E34 - runs the pane sub-state `+0x19D` selects: 0 dispatches on the pane's state byte,
 * 1 is the null one. */
extern "C" void note_pane_dispatch(NoteWork* self) {
    switch (self->field_0x19D) {
    case 0: note_pane_state_dispatch(self); break;
    case 1: note_pane_idle(self); break;
    }
}

/* 0x803A4DD4 - the note pane's per-state dispatcher: `NoteWork::field_0x19F` selects one of ten
 * tail-called sub-state bodies. */
extern "C" void note_pane_state_dispatch(NoteWork* self) {
    switch (self->field_0x19F) {
    case 0: note_pane_state_0(self); break;
    case 1: note_pane_state_1(self); break;
    case 2: note_pane_state_2(self); break;
    case 3: note_pane_state_3(self); break;
    case 4: note_pane_state_4(self); break;
    case 5: note_pane_state_5(self); break;
    case 6: note_pane_state_6(self, 0); break;
    case 7: note_pane_state_6(self, 1); break;
    case 8: note_pane_state_6(self, 2); break;
    case 9: note_pane_state_9(self); break;
    }
}

/* 0x803A4E58 - reads the quest NPC's motion number through `qn_get_motion_no` (the record is the pane's 0x1F8-byte
 * `NoteWork`; the vector local is retail's `VEC3 v; VEC3_ctor(&v);` idiom). */
extern "C" void note_pane_get_motion(NoteWork* self) {
    nw4r::math::VEC3 v;
    VEC3_ctor(&v);
    qn_get_motion_no((_QNPC_W*)self);
}

/* 0x803A4E90 - true while the lifetime record still has a live countdown and a set flag. */
extern "C" s32 note_timer_ready_ck(NoteTimer* timer) {
    if ((s8)timer->count_0x06 <= 0) {
        if ((s8)timer->flag_0x01 != 0) {
            return 1;
        }
    }
    return 0;
}

/* 0x803A4F7C - the inverse of `note_slot_to_value`: the flat slot index holding `value` in `note_slot_table`'s four
 * 6-entry tables, then `note_slot_flat_table`; 255 when absent. */
extern "C" u32 note_value_to_slot(u16 value) {
    u32 index = 0;
    s8 t;
    for (t = 0; t < 4; t++) {
        const u16* p = note_slot_table[t];
        u32 j;
        for (j = 0; j < 6; j++) {
            if (value == p[j]) {
                return index;
            }
            index++;
        }
    }
    {
        const u16* q = note_slot_flat_table;
        while (*q != 0) {
            if (value == *q) {
                return index;
            }
            index++;
            q++;
        }
    }
    return 255;
}

/* 0x803A5070 - maps a flat slot index to its table value over the same four 6-entry tables and the
 * flat run at `note_slot_flat_table`; 0 past the end of both. */
extern "C" u16 note_slot_to_value(u8 slot) {
    u32 index = 0;
    u16 t;
    for (t = 0; t < 4; t++) {
        const u16* p = note_slot_table[t];
        u32 j;
        for (j = 0; j < 6; j++) {
            if (slot == (u8)index) {
                return p[j];
            }
            index++;
        }
    }
    {
        const u16* q = note_slot_flat_table;
        while (*q != 0) {
            if (slot == (u8)index) {
                return *q;
            }
            index++;
            q++;
        }
    }
    return 0;
}
