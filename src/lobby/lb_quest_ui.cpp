/* lobby/lb_quest_ui.cpp - the lobby's list/detail UI band, `.text` 0x8038E8E8..0x80394038.
 *
 * WHAT IT IS.  A lobby screen: it draws scrolling list rows with sprite icons and glyph text
 * (`get_lsp_data` / `draw_sprite_ary` / `draw_sprite_idx` / `draw_font` / `draw_font_idx`), colours
 * the selected row with `GetMenuFontColor`, draws the cursor with `put_menu_cursor`, prints counts
 * with `sprintf` and the lobby's localized strings with `LbStr`, and plays UI sounds with
 * `sysSE_req`.  The rows live in the record the screen's entry point is handed (a `+0xC4` array of
 * `{u16 id, u8 payload}` rows) and the screen's state is driven by `game_ready_ck` plus the
 * `fn_8021D5BC` lobby state query.  Its first ten functions (`0x8038E8E8..0x8038EF28`) are another
 * subsystem's - see SEAM below.
 *
 * MODULE AND NAME (brief section 2, evidence order), evidence class 3.  1. No `__FILE__` string
 * covers the range: every `s_*`/`lbl_*` reference of its 65 split objects is a `.data` table, a
 * `.sdata` format string, a jumptable or a `.sdata2` float, never a source-file-name literal.  (The
 * `menu_note.cpp` string the proposal's discovery note cites at `.data` 0x805E91F8 is NOT this
 * unit's: it has exactly one copy in the DOL and is referenced by `src/menu/menu_note.cpp`'s object,
 * which declares it - the note is a tool artefact, and the queue carries the same one for six other
 * proposals of this band.)  2. `dumpmap.py lookup` answers `zz_XXXXXXXX_` for every address of the
 * range (the dump's two real names inside it - `DBClose`, `JASDsp::TChannel::forceStop` - are a
 * different build's layout: the code under them is this range's).  3. Module `lobby` and the
 * screen's name from the code and the band: the range reads the lobby work block `lobby_w`
 * (`.bss` 0x806AAB44) at `+0x0AC` and the option block `lb_param_w` (`.bss` 0x806590B4), calls
 * `LbStr` 28 times and `get_lsp_data`/`draw_sprite_ary`/`GetMenuFontColor`/`put_menu_cursor`
 * throughout, and the registered lobby bands document exactly this callee profile
 * (`lobby/fn_8030121C.cpp`, `lobby/lb_companion_ui.cpp`).  It is reached from the lobby's own menu
 * dispatcher: `fn_80211E68` (`src/lobby/fn_8020C588.cpp`) switches on the lobby menu index and calls
 * two of this range's functions (`fn_80390F20`, `fn_80392CC8`) as screen entry points, and
 * `lobby/fn_802076D4.cpp`/`lobby/fn_80219260.cpp` call `fn_8038F4D8`/`fn_803928F0`.  The file name
 * `lb_quest_ui.cpp` is a **GUESS** (marked: nothing pins which of the lobby's screens this is - the
 * list rows carry no literal screen title, and the ids the code hands `LbStr` index a string table
 * loaded at runtime), chosen under the module's own `lb_*` scheme (`lb_npc`, `lb_companion_ui`,
 * `lb_menu_page`) for what the range plainly is: the frame step, list rows, detail pane and sprites
 * of one lobby screen.
 *
 * SEAM (unproven, two candidates).  The proposal is one maximal unclaimed run and discovery's own
 * note flags a candidate seam inside it: `tudiscover.py` reports a strong `.sdata2` cut at
 * 0x8038EF28 (the range below it owns no `.sdata2` at all; the range above it owns the whole run
 * 0x8079C298..0x8079C2B4).  A second candidate sits at 0x8038EBE8/0x8038EC44, and it is the better
 * supported one: the six functions `0x8038E8E8..0x8038E9F0` are entry slots of `em009_prog_tbl`
 * (`.data` 0x805EF990, the enemy program-table block) and their only `.data` reference is
 * `lbl_805EFAB8` (`.data` 0x805EFAB8 = `{ -1.0f, 0, 20 }`, the enemy band's data), while every other
 * section of the range (`.data` 0x805F0CB8 onward, `.sdata` 0x807933D8, `.sbss` 0x80794880,
 * `.sdata2` 0x8079C298) is the UI band's - so those six are most likely an `em009` action TU.  They
 * are named `em009_act_*` here (evidence: `em009_prog_tbl` lists each of their addresses) and the
 * re-draw is reported for the seam round.
 *
 * SECTIONS.  `.text` 0x8038E8E8..0x80394038 (70 functions / 0x5750 B), extab 0x8001819C..0x8001833C
 * (52 x 8 B) and extabindex 0x800380D0..0x80038340 (52 x 12 B) - each run is exactly the gap the
 * bracketing registered units leave (`enemy/fn_80387844.cpp` ends at 0x8001819C / 0x800380D0, and
 * both runs are contiguous from there).  No `.data`/`.sdata`/`.sdata2` claim: the range *references*
 * 45 `.data` labels, 20 `.sdata` ones and the `.sdata2` run but owns none of the bytes it does not
 * emit, and the two compiler-emitted switch tables it would need (`jumptable_805F0F5C`,
 * `jumptable_805F1020`) sit inside an unclaimed `.data` band - claiming it would take other units'
 * bytes with it (playbook 53/55).
 *
 * FLAGS.  The `lobby` lib's `cflags_lobby` (`-O3 -inline noauto`, `wii/1.3`), whose `-Cpp_exceptions on`
 * (flags-audit 2026-09-28, replacing the per-file pragma) emits the unwind records: the range's 52
 * extab records are the target's own.
 *
 * RULE 2 BOUNDARY ARTEFACT (reported, not resolved here).  Two of the switch arms of
 * `lb_ui_detail_step` tail into 0x80394144 / 0x80394154, which the *next* band owns
 * (`src/lobby/lb_quest_board.cpp` landed on main after this branch was cut).  That unit's header does
 * not declare them, so this branch declares them in the band header `include/unsplit/lobby.h` and
 * reports it: `land.band_ownership_warnings` lists the two lines (a warning the gate never refuses -
 * see its docstring), and the fix on the merged tree is to move the two declarations into
 * `include/lobby/lb_quest_board.h`.
 *
 * Naming note: references only to other units' unrenamed fn_XXXXXXXX symbols (checked with
 * `grep -rn "fn_80" src/lobby/lb_quest_ui.cpp` - the only `fn_` names this file names
 * (`fn_80394144`, `fn_80394154`) are the next band's, declared by the band header it includes).
 *
 * STATUS and RESIDUALS.  21 of the 70 functions carry a body, 13 of them byte-identical (unit fuzzy
 * 5.6330, matched_functions 13 of 70, matched_code 536 of 22352 `.text` bytes).  `lb_ui_tri_sum` has a
 * body and scores 0.0, and seven written bodies sit below the bar (see the list below).  Measured residuals on the written bodies: `lb_ui_page_flag_ck` 81.22 /
 * `lb_ui_page_flag_set` 79.23 - the `.sbss` page pointer is declared `u8 lobby_world_block[]` in
 * `include/unsplit/lobby.h`, so this unit loads it with `lis`/`lwz` where retail uses one `lwz @sda21`
 * (the fix is re-typing that shared declaration, which eight other units also include);
 * `lb_ui_pair_offset` 77.14 (argument register order), `lb_ui_clear` 94.52 (`memset` argument setup
 * order), `lb_ui_effect_list_step` 98.71 / `lb_ui_param_apply` 99.57 / `lb_ui_angle_step` 99.43 (a
 * register name each), `lb_ui_pair_lookup` 17.71 (loop shape),
 * `lb_ui_tri_sum` 0.0 (retail is MWCC's 8x-unrolled countdown loop and the source shape that produces
 * it is not settled yet).  The `em009` fragment's `0x8038E8EC` (0x6C) is not written: its
 * first arm is a masked-compare idiom (`xori 0x708` / `srawi 1` / `andi.` / `subf` / `srwi 31`) whose
 * C source shape is not settled yet; `0x8038EBE8` (0x5C), `0x8038EEC8` (0x54) and the four large
 * UI bodies above 0x8038F150 are unwritten for the same reason - they need the row/panel types the
 * next pass derives.  `0x8038EF1C` (0xC, `b fn_80385AD4`) is unwritten too: the callee is a symbol of
 * `enemy/fn_80382310.cpp` whose record type (`NoteWork`) is that unit's own local type, so the
 * declaration cannot be spelled here without first moving that type into a header (rule 2) - the
 * header move is reported for the round that registers the `em009` fragment.
 */

#include "types.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "ef/fn_800CDB2C.h"
#include "ef/eft_res.h"
#include "sound/fn_800DD1F0.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8011D448.h"
#include "unsplit/enemy.h"
#include "unsplit/lobby.h"

/* The screen's row work: the box the screen's entry point is handed.  Only the fields the
 * reconstructed bodies touch are named; the byte mass between them is untouched here and the size is
 * an approximation (nothing in this range allocates or `memset`s it). size: 0x1A8 (approximate) */
/* One record of the screen's pooled-effect list at `+0x038`: a count followed by the handles the
 * effect library retires.  This unit keeps its own view because the next band's header
 * (`include/lobby/lb_quest_board.h`, which documents the same record as `LbEftList`) only exists on
 * main - this branch was cut before it landed, so the fold is recorded in the outbox instead. size: 0x8 */
typedef struct LbUiEftList {
    /* +0x000 */ s32 count_0x000;
    /* +0x004 */ void* handles_0x004[1];
} LbUiEftList;

typedef struct LbQuestRowWork {
    /* +0x000 */ u8 state_0x00;
    /* +0x001 */ u8 active_0x01;
    /* +0x002 */ u8 sub_state_0x02;
    /* +0x003 */ u8 unused_0x03[2];
    /* +0x005 */ u8 detail_0x05;
    /* +0x006 */ u8 unused_0x06[2];
    /* +0x008 */ u8 unused_0x08[0x12];
    /* +0x01A */ s16 count_0x1A;
    /* +0x01C */ u8 unused_0x1C[0x0C];
    /* +0x028 */ u8 unused_0x28[0x10];
    /* +0x038 */ LbUiEftList* eft_list_0x38;
    /* +0x03C */ u8 unused_0x3C[0x0F];
    /* +0x04B */ s8 flags_0x4B[3];
    /* +0x04E */ u8 unused_0x4E[0x70 - 0x4E];
    /* +0x070 */ u8 slots_0x70[12];
    /* +0x07C */ u8 unused_0x7C[0xF4];
    /* +0x170 */ f32 pos_0x170[3];
    /* +0x17C */ u8 unused_0x17C[0x10];
    /* +0x18C */ u32 cur_0x18C;
    /* +0x190 */ u8 unused_0x190[0x0D];
    /* +0x19D */ u8 mode_0x19D;
    /* +0x19E */ u8 unused_0x19E[1];
    /* +0x19F */ u8 sub_mode_0x19F;
    /* +0x1A0 */ u8 unused_0x1A0[1];
    /* +0x1A1 */ u8 flag_0x1A1;
    /* +0x1A2 */ u8 unused_0x1A2[0x06];
    /* +0x1A8 */ u32 target_0x1A8;
    /* +0x1AC */ u8 unused_0x1AC[0x09];
    /* +0x1B5 */ u8 field_0x1B5;
    /* +0x1B6 */ u8 unused_0x1B6[2];
} LbQuestRowWork; /* size: 0x1B8 */

/* The layout is load-bearing - every body below is measured against these offsets - so it is checked
 * at compile time: any padding that drifts makes one of these array sizes negative. */
typedef char lb_ui_layout_check_size[(sizeof(LbQuestRowWork) == 0x1B8) ? 1 : -1];

/* One 5-byte row record the screen's copy helper moves. size: 0x5 */
typedef struct LbQuestRowRec {
    /* +0x0 */ u16 id_0x00;
    /* +0x2 */ u8 value_0x02;
    /* +0x3 */ u8 kind_0x03;
    /* +0x4 */ u8 flag_0x04;
} LbQuestRowRec;

extern "C" {

/* The unit's own unwritten entry points, declared so the dispatch bodies below can call them with
 * the map's new names (rule 2: a symbol this unit owns is declared here, not in a band header). */
void lb_ui_submode_0_step(LbQuestRowWork* self);   /* 0x8038EFEC */
void lb_ui_submode_1_step(LbQuestRowWork* self);   /* 0x8038F09C */
void lb_ui_submode_2_step(LbQuestRowWork* self);   /* 0x8038F150 */
void lb_ui_row_draw(LbQuestRowWork* self);          /* 0x80391570 */
void lb_ui_rows_draw(LbQuestRowWork* self);         /* 0x8039193C */
void lb_ui_row_update(LbQuestRowWork* self, u32 mode); /* 0x80391EF4 */
void lb_ui_row_select_draw(LbQuestRowWork* self);   /* 0x803921A4 */
void lb_ui_detail_0_draw(LbQuestRowWork* self);     /* 0x80393B28 */
void lb_ui_detail_1_draw(LbQuestRowWork* self);     /* 0x80393D4C */
void lb_ui_slot_set(LbQuestRowWork* self, u32 id);   /* 0x8038F74C */
void lb_ui_slot_clear(LbQuestRowWork* self, u32 id); /* 0x8038F9A4 */

/* Runs the sub-mode the row work's +0x19F byte selects. */
void lb_ui_submode_step(LbQuestRowWork* self) {
    switch (self->sub_mode_0x19F) {
    case 0:
        lb_ui_submode_0_step(self);
        break;
    case 1:
        lb_ui_submode_1_step(self);
        break;
    case 2:
        lb_ui_submode_2_step(self);
        break;
    }
}

/* The empty sub-mode arm: a table slot with nothing to do. */
void lb_ui_noop(void) {
}

/* Runs the arm the row work's +0x19D byte selects. */
void lb_ui_mode_step(LbQuestRowWork* self) {
    switch (self->mode_0x19D) {
    case 0:
        lb_ui_submode_step(self);
        break;
    case 1:
        lb_ui_noop();
        break;
    }
}

/* Copies one 5-byte row record. */
void lb_ui_copy_row(LbQuestRowRec* dst, const LbQuestRowRec* src) {
    dst->id_0x00 = src->id_0x00;
    dst->value_0x02 = src->value_0x02;
    dst->kind_0x03 = src->kind_0x03;
    dst->flag_0x04 = src->flag_0x04;
}

/* Whether the lobby screen whose list this unit draws is in a state that wants its rows moved. */
s32 lb_screen_list_state_ck(void) {
    return fn_80217934();
}

/* The same query for the screen's detail pane. */
s32 lb_screen_info_state_ck(void) {
    return fn_80217934();
}

/* --------------------------------------------------------------------------------------------- *
 * The `em009` entry slots (see SEAM): `em009_prog_tbl` lists each of these addresses.
 * --------------------------------------------------------------------------------------------- */

/* An unused `em009` slot handler. */
void em009_act_noop(void) {
}

/* Clears the handler's 0x33A parameter word. */
void em009_act_clear_param(_ENEMY_WORK* work) {
    work->init_0x328.field_0x33A = 0;
}

/* Fills a slot-kind pair after arming the work's effect slot 4. */
void em009_act_slot_init(_ENEMY_WORK* work, u8* kind, u8* value) {
    em_move_mode_set(work, 4);
    *kind = 12;
    *value = 0;
}

/* Whether the work's part 6 is damaged. */
int em009_act_part_broken_ck(_ENEMY_WORK* work) {
    return em_parts_damage_level_get(work, 6) >= 1;
}


/* --------------------------------------------------------------------------------------------- *
 * The screen's per-frame state, its row keys and its page bit sets.
 * --------------------------------------------------------------------------------------------- */

/* Moves the work's 16-bit angle one 1820-step towards its target, snapping when it is inside one
 * step, and wrapping through 0 the way the record's own 16-bit field does. */
void lb_ui_angle_step(LbQuestRowWork* self) {
    u32 target = self->target_0x1A8;
    u32 cur = self->cur_0x18C;
    u16 diff = (u16)(target - (u16)cur);
    if ((u16)(diff + 1820) < 3640) {
        self->cur_0x18C = target;
    } else if (diff < 0x8000) {
        self->cur_0x18C = (u16)(cur + 1820);
    } else {
        self->cur_0x18C = (u16)(cur - 1820);
    }
}

/* Runs the screen's frame step: the mode arm, once more when the work raised its one-shot flag, then
 * the angle step. */
void lb_ui_screen_step(LbQuestRowWork* self) {
    lb_ui_mode_step(self);
    if (self->flag_0x1A1 == 1) {
        lb_ui_mode_step(self);
        self->flag_0x1A1 = 0;
    }
    lb_ui_angle_step(self);
}

/* The triangular number of a count - what a row's key space is offset by.  The second argument is
 * part of the retail call shape and unused here. */
s32 lb_ui_tri_sum(u32 count, u32 unused) {
    u16 n = (u16)(count - 1);
    s32 sum = 0;
    s32 value = n;
    for (u32 i = 0; i < n; i++) {
        sum += value;
        value--;
    }
    return sum;
}

/* The base key of the pair (`a`,`b`): the smaller id plus the number of keys the larger id's rows
 * occupy below it. */
u16 lb_ui_pair_offset(u32 a, u32 b) {
    u32 base = ((u16)a < (u16)b) ? a : b;
    return (u16)(base + lb_ui_tri_sum((u16)a, a));
}

/* Looks the pair up in the type's key table: the table is walked in 6-byte records and terminated by
 * a record whose first u16 is 0xFFFF, whose address the not-found answer is. */
u16* lb_ui_pair_lookup(u8 kind, u8 a, u8 b) {
    u16* p = lbl_805F0EC8[kind];
    u16 key;
    if (a < b) {
        key = (u16)((a << 8) | b);
    } else {
        key = (u16)((b << 8) | a);
    }
    u16 value = *p;
    while (value != 0xFFFF) {
        if (value == key) {
            return p;
        }
        p += 3;
        value = *p;
    }
    return p;
}

/* Sets the pair's bit in the current page's set - in the offline set while the game is not ready,
 * in the online one afterwards. */
void lb_ui_page_flag_set(u16 a, u16 b) {
    u32 key = lb_ui_pair_offset((u16)a, (u16)b);
    if (game_ready_ck() == 0) {
        u8* page = *(u8**)lobby_world_block;
        page[(key >> 3) + 0x516C] |= (u8)(1 << (key & 7));
    } else {
        u8* page = *(u8**)lobby_world_block;
        page[(key >> 3) + 0x525C] |= (u8)(1 << (key & 7));
    }
}

/* Whether the pair's bit is set in the current page's set. */
int lb_ui_page_flag_ck(u16 a, u16 b) {
    u32 key = lb_ui_pair_offset((u16)a, (u16)b);
    if (game_ready_ck() == 0) {
        u8* page = *(u8**)lobby_world_block;
        return (page[(key >> 3) + 0x516C] & (u8)(1 << (key & 7))) != 0;
    }
    u8* page = *(u8**)lobby_world_block;
    return (page[(key >> 3) + 0x525C] & (u8)(1 << (key & 7))) != 0;
}

/* Empties the screen's slot block and both of its selection tables. */
void lb_ui_clear(LbQuestRowWork* self) {
    u8* slots = self->slots_0x70;
    memset(slots, 0xFF, 12);
    self->active_0x01 = 0;
    self->count_0x1A = 0;
    lb_ui_slot_set(self, 0);
    lb_ui_slot_clear(self, 0);
}

/* --------------------------------------------------------------------------------------------- *
 * The screen's per-row key work and its per-detail arms.
 * --------------------------------------------------------------------------------------------- */

/* Applies the screen's three key flags to the option-parameter block: each flag byte is copied and
 * its signed value indexes the 12-entry table whose word lands beside it. */
void lb_ui_param_apply(LbQuestRowWork* self) {
    for (s32 i = 0; i < 3; i++) {
        lb_param_w.flag_0x0C[i] = self->flags_0x4B[i];
        s16 value = lbl_805F0CDC[self->flags_0x4B[i]];
        lb_param_w.value_0x10[i] = value;
    }
}

/* Runs the frame step, the row draws and the update arm the screen's state selects. */
void lb_ui_page_step(LbQuestRowWork* self) {
    lb_ui_row_draw(self);
    lb_ui_rows_draw(self);
    if (self->active_0x01 == 2) {
        lb_ui_row_select_draw(self);
        lb_ui_row_update(self, 0);
    } else if (self->active_0x01 != 0) {
        lb_ui_row_update(self, 1);
    }
}

/* Steps every record of the screen's 0x38 table: the record's own pointer is released and the record
 * is flagged. */
void lb_ui_effect_list_step(LbQuestRowWork* self) {
    LbUiEftList* list = self->eft_list_0x38;
    for (s32 i = 0; i < list->count_0x000; i++) {
        fn_800E26C4(list->handles_0x004[i]);
        fn_800F8A44((void*)&list->handles_0x004[i], 1);
    }
}

/* Runs the detail arm the screen's +0x05 byte selects. */
void lb_ui_detail_step(LbQuestRowWork* self) {
    switch (self->detail_0x05) {
    case 0:
        lb_ui_detail_0_draw(self);
        break;
    case 1:
        lb_ui_detail_1_draw(self);
        break;
    case 2:
        fn_80394144(self);
        break;
    case 3:
        fn_80394154(self);
        break;
    }
}

}  /* extern "C" */
