/* lobby/lb_quest_screen.cpp - the quest/multiplayer screen band that follows the registered
 * `menu/multi_result.cpp`, plus the note-pane helpers that share its head.
 *
 * `.text` 0x803A3A50..0x803AA4A4 (95 functions, 27220 B), registered whole as the proposal
 * `803A3A50_fn_803A3A50` asked.  The seam is UNPROVEN and the range is *not* one translation unit:
 * discovery's `--max-bytes` cap cut a 176 KB unclaimed run here, this range is its first slice, and
 * the run holds several original objects the linker placed next to each other.
 *
 * SEAM EVIDENCE (measured this session):
 *   - no `__FILE__` string covers the range.  Scanning every `<name>.cpp` literal in the DOL's own
 *     section table finds exactly six in the menu/lobby area - `menu_item.cpp`, `cockpit.cpp`,
 *     `layout.cpp`, `cockpit_quest.cpp`, `menu_infomation.cpp`, `menu_note.cpp` - and none of them is
 *     referenced from 0x803A3A50..0x803AA4A4.
 *   - the runtime dump answers `zz_XXXXXXXX_` for 92 of the range's 95 functions; the three it does
 *     name are junk duplicates (`DBClose` at 295 addresses, `JKRArchive::getResSize(void)` at two).
 *     So there is no class-1 or class-2 name evidence and the module/file name below are a GUESS.
 *   - the range is a sequence of five bands, each with its own subsystem:
 *       * 0x803A3A50..0x803A52A4 - the note pane: `fn_803A3A50`'s parameter is a `struct NoteWork`
 *         (the same +0x19D/+0x19F/+0x1A1 record `enemy/fn_80382310.cpp` drives), it calls that band's
 *         `note_pane_anim_pair_ck`/`note_pane_set_anim_pair`, and its CALLEES at 0x803A4E30..0x803A4F7C are declared there as
 *         `NoteWork*` consumers.
 *       * 0x803A52A4..0x803A75D8 - the lobby item/quest screen: `lobby_w`, `lb_param_w`,
 *         `lb_item_get_data`, `subTransSet`, `LbStr`, `get_lsp_data`/`draw_sprite_*`/`draw_font_idx`
 *         and `menu_cursor_step`; a paged list with item icons and page arrows.
 *       * 0x803A7718..0x803A86EC - resource loading (`nwAddResource`, `load_file_req`, `work_mem_*`).
 *       * 0x803A86F0..0x803A96C4 - enemy/player model + quest bookkeeping (`Screen_w`, `em_area_ck`,
 *         `get_em_chg_scale`, the `lbl_8079C478..C4BC` float run).
 *       * 0x803A96C4..0x803AA4A4 - enemy area/quest logic plus `lb_sub16_send`.
 *   - the extab/extabindex runs tile in link order and show no jump at either edge: extab
 *     0x8001882C..0x80018A6C is exactly contiguous with `menu/multi_result.cpp`'s run below it, and
 *     extabindex 0x80038AA8..0x80038E08 is exactly contiguous with its 0x800387C0..0x80038AA8.  Both
 *     are the gap `dol split` derived, not a proven TU edge.
 *
 * MODULE AND NAME (brief section 2).  Class 1 (no `__FILE__`) and class 2 (no dump name) have no
 * evidence.  Class 3: the range's globals are the lobby's (`lobby_w` 0x806AAB44, `lb_param_w`,
 * `Screen_w`, `lbl_80794880`), its dominant API is the lobby's (`LbStr`, `lb_item_get_data`,
 * `subTransSet`, `menu_cursor_step`) and the nearest registered same-lib family is
 * `lobby/lb_quest_ui.cpp` / `lobby/lb_quest_board.cpp`; the direct left neighbour is
 * `menu/multi_result.cpp`.  So the module is `lobby` and `lb_quest_screen.cpp` is a **GUESS** derived
 * from what the biggest band in the range does.  Every function name below is a GUESS derived from
 * its own body; the decision is recorded here so the seam re-draw can move the file and the map rows
 * together.
 *
 * LANGUAGE AND SECTIONS.  C++ with exceptions on: every framed function of the range carries its own
 * extab/extabindex record in the target object.  Claimed: `.text` 0x803A3A50..0x803AA4A4, extab
 * 0x8001882C..0x80018A6C, extabindex 0x80038AA8..0x80038E08, and `.data` 0x805F2038..0x805F2090 - the
 * two switch tables `jumptable_805F2038` (sole referrer 0x803A3B0C) and `jumptable_805F2068` (sole
 * referrer 0x803A4DE0), both private to this range and claimed nowhere else.  The range's other
 * `.data` tables (0x805F20F0, 0x805F2100, 0x805F26B4, 0x805F2720, 0x805F2738..0x805F2A38,
 * 0x80793530..0x807935C4) are referenced as the map's `lbl_` symbols and are NOT claimed: they are
 * separate runs, one of them (0x805F20F0) has a referrer outside the range (0x803C09F0), and a
 * partial run of one section puts an `auto_*` unit inside this unit's range (playbook 53).
 *
 * RESIDUAL / NEXT PASS.
 *   - 12 of the 95 functions are written (the note-pane head).  The 83 unwritten ones carry 25620 of
 *     27220 `.text` bytes; the biggest are `fn_803A3A50` 0x694, `fn_803A8128` 0x5C4, `fn_803A5B60`
 *     0x4FC, `fn_803A96C4` 0x4B8, `fn_803A5680` 0x4A8, `fn_803A52A8` 0x3A8, `fn_803A6A48` 0x394,
 *     `fn_803A8994` 0x3B8, `fn_803A79C0` 0x318.
 *   - the lobby screen band (0x803A52A4..0x803A75D8) is BLOCKED on a view merge, not on effort: it
 *     switches on `lobby_w.menu_0xAC`'s +0x00 byte but reads that object's +0x28/+0x2A/+0x3C/+0xD0
 *     with meanings that do not agree with `LbMenuWork` in `include/unsplit/lobby.h` (+0x2A has no
 *     name at all, +0x3C is `value_0x3C`).  Re-typing it changes the declaration set of every unit
 *     that includes that header (playbook 60), so it belongs in one merge pass with the
 *     `lobby/fn_8020C588.h` / `fn_8021E1EC.h` / `fn_801F3294.h` views of the same block.
 *   - the 0x803A7718..0x803A7E1C group (`fn_803A7718` a free-slot scan returning the first record
 *     whose +0x00 is clear, `fn_803A77A8` the id lookup over the same 8 x 0xC array, and
 *     `fn_803A78DC`/`fn_803A79C0` the eight-slot constant-size resource loaders) is blocked by the
 *     same thing: its array lives at `lobby_w + 0xCC`, inside `LbLobbyWork::unused_0x0B0[0x7C]`.
 *   - `note_pane_state_dispatch` selects ten tail-called sub-states that are still unwritten; they
 *     are named from the pane's state order and their bodies are the next pass's work.
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

/* The lobby lifetime/timer record at `lbl_80794880 + 0x5270` that `note_timer_*` reads: a flag, a
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

/* 0x803A4170 - steps the pane's eased position towards its target: 4004 units while its animation
 * pair is (0,4) and 1820 otherwise, snapping onto the target when it is within one step. */
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

/* 0x803A4E58 - reads the quest NPC's motion number through `qn_get_motion_no`.  The record is the
 * same 0x1F8 bytes as the pane's `NoteWork`; the vector local is the retail call-site idiom
 * `VEC3 v; VEC3_ctor(&v);`. */
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

/* 0x803A4F7C - the inverse of `note_slot_to_value`: the flat slot index whose table value is
 * `value`, searched over the four 6-entry tables of `note_slot_table` and then the flat run at
 * `note_slot_flat_table`; 255 when the value is not in them. */
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
