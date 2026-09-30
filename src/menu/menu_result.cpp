/* menu/menu_result.cpp - the quest-result screen band.
 *
 * `.text` 0x803967F0..0x8039D278 (85 functions, 0x6A88 B) plus the data it owns: `.data`
 * 0x805F1500..0x805F1704 (the icon/message-id tables and three switch tables), `.sdata`
 * 0x80793488..0x8079351C (the same kind of id arrays) and `.sdata2` 0x8079C2EC..0x8079C330 (the
 * band's own literal pool).  All three runs are *private*: every label inside them is referenced
 * from this range only, and the `.data` referrer runs break exactly at 0x803967F0 - the run below
 * ends at 0x803960BC (fn_803960BC's object) and this unit's first referrer is 0x80396BBC, so the
 * left edge is a real translation-unit seam, unlike the brief's `--max-bytes` cap.  The right edge
 * is bounded the same way: the next `.data` label is `em029_prog_tbl` (0x805F1708), an enemy
 * program table, whose own band's jumptables resume at 0x805F1774.
 *
 * NAME: module `menu` (evidence class 3) - the `.data` band this unit's tables sit in carries
 * `menu_note.cpp` (0x805E91F8) below and `menu_placeinfo.cpp` (0x80604780) above, and every callee
 * of the range is the menu library's (`get_menu_lsp_tbl`, `put_menu_cursor`, `GetMenuFontColor`,
 * `ItemName`, `GetItemData`, `PutPageArrow`, `font_print_ex`).  The file name `menu_result.cpp` is a
 * GUESS (no `__FILE__` string reaches the range and `dumpmap.py` answers `zz_` for every address):
 * the band's state machine drives `get_qResult_work()`'s record, reads the `q_result_msg_adrs`
 * message table by id and language (0x806C5528, the runtime dump's own name) and gates on
 * `field_0x1E2` (2 while loading, 3 when loaded), i.e. it is the screen drawn while the quest result
 * is loaded and shown.  A later pass with the screen's own symbols can sharpen it.
 *
 * NAMING: every symbol this unit defines is named for what its body does (or for the record field
 * it touches) plus the `q_result_*` prefix of the band; there is no `__FILE__` string and the
 * runtime dump answers `zz_` for all 85 addresses, so *every name here is a GUESS* - the evidence
 * behind each one is the body comment in the unit's own header and this file's functions.
 *
 * FLAGS: `menu`'s `cflags_menu` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`, mw version
 * Wii/1.3) - the band is C++ (its bodies call genuinely mangled callees such as
 * `get_joint_wpos__6MHcharFUlPQ34nw4r4math4VEC3`) and it keeps unfused narrow-load pairs the
 * peephole would fuse.
 *
 * RESIDUAL: 28 of the 85 functions are written in this pass and 27 of them are at 100.0;
 * `q_result_sub_screen_ready` is at 99.33 - its second `!= 1` test is emitted with the opposite
 * branch polarity (`beq` to the TRUE return) however the two calls are nested, i.e. MWCC
 * if-converts the `return (B == 1)` here where the target kept the branch; the body is otherwise
 * instruction-identical.  The remaining 57 (largest first:
 * `fn_80398F7C` 0xC80, `fn_8039756C` 0xB00, `fn_8039AFBC` 0x774, `fn_8039BFF8` 0x734,
 * `fn_8039B9C0` 0x638, ...) are still map stems and measure 0 %, so their names and bodies are the
 * next pass's work.  Their data tables (.data/.sdata/.sdata2 above) are not emitted by this object
 * yet, so the two id-table runs stay unclaimed in `splits.txt` until the bodies that own them
 * land.  The one data range that IS claimed is the switch jump table this object emits
 * (`.data` 0x805F1698..0x805F16BC, the target's `jumptable_805F1698`, 36 B, referenced by this
 * unit only) - without it `datagap.py` reports an `ours-extra .data 36B` row.
 *
 * Naming note: references only to other units' unrenamed fn_XXXXXXXX symbols (each checked
 * against the map's owner: `fn_800F886C` -> `ef/eft_res.cpp`, `fn_800DBDD4` ->
 * `sound/fn_800D7F54.cpp`, `item_pair_copy` -> `fn_80047398.cpp`, and `fn_803B5030`, `fn_802DF6E4`,
 * `fn_803B4C64`/`fn_803B4CE8` -> unregistered bands).  No `fn_` name
 * this unit *defines* is left unrenamed: every body above carries the name `symbols.txt` now has.
 */

#include "types.h"
#include "menu/menu_result.h"

#include "Runtime.PPCEABI.H/memset.h"
#include "ef/eft_res.h"
#include "sound/fn_800D7F54.h"
#include "fn_8004CAD8.h"
#include "fn_80047398.h"
#include "enemy/em_pop.h"   /* quest_flag_10_ck (rule 2: its owner) */

/* The `sprintf`/`strcpy`/`strcat` family sits in an address band whose bracketing registered units
 * name different modules, so rule 2's home for them is `include/unsplit/*.h`'s documented gap and
 * they are declared here (the same spelling `menu/fn_802E4978.cpp` uses). */
extern "C" {
int sprintf(char*, const char*, ...);
char* strcpy(char*, const char*);
char* strcat(char*, const char*);
}

extern "C" {

/* Bumps the screen's per-frame animation counter at +0x05. */
void q_result_anim_counter_inc(QResultScreen* self)
{
    self->anim_step += 1;
}

/* Hands the screen's character record to the effect system's release (0x800F886C). */
void q_result_release_effect(QResultScreen* self)
{
    fn_800F886C(self);
}

/* The language-selected message table for one message id: `q_result_msg_adrs[id][language]`. */
char** q_result_msg_table(u8 id)
{
    return q_result_msg_adrs[id][system_w[9]];
}

/* One string of the message table `q_result_msg_entry`'s owner selected. */
char* q_result_msg_entry(u8 id, u8 index)
{
    return q_result_msg_adrs[id][system_w[9]][index];
}

/* The same table as `q_result_msg_table`, instruction for instruction - the band's
 * `fn_8039AFBC` calls both spellings, so the original source carried two names for it. */
char** q_result_msg_table_alt(u8 id)
{
    return q_result_msg_adrs[id][system_w[9]];
}

/* Message set 10's string array, indexed by id. */
char* q_result_msg_string(u8 id)
{
    return q_result_msg_table(10)[id];
}

/* Message set 11's string array, indexed by id (the sibling `q_result_msg_string` reads set 10). */
char* q_result_msg_string_alt(u8 id)
{
    return q_result_msg_table(11)[id];
}

/* Zeroes the 0x350C-byte screen work buffer, or reports that the screen has none yet. */
BOOL q_result_work_clear(QResultScreen* self)
{
    if (self->work == NULL) {
        return FALSE;
    }
    memset(self->work, 0, 0x350C);
    return TRUE;
}

/* Whether the band's resource set has finished loading (0x803B5030's flag for the default set). */
u32 q_result_file_ready(QResultScreen* self)
{
    (void)self;
    return quest_flag_10_ck(0) == 1;
}

/* The `ready_flag` gate `fn_80396F34` switches on. */
BOOL q_result_ready_ck(QResultScreen* self)
{
    return self->ready_flag != 0;
}

/* The screen's swap counter at +0x306E. */
u8 q_result_swap_counter_get(QResultScreen* self)
{
    return self->swap_counter;
}

/* Whether the result record reports load phase 2. */
u32 q_result_phase_is_2(QResultScreen* self)
{
    (void)self;
    return get_qResult_work()->phase_0x1E2 == 2;
}

/* Whether the result record reports load phase 3 (loaded). */
u32 q_result_phase_is_3(QResultScreen* self)
{
    (void)self;
    return get_qResult_work()->phase_0x1E2 == 3;
}

/* Latches the one-shot `init_flag` at +0x3027. */
void q_result_init_flag_set(QResultScreen* self)
{
    if (self->init_flag == 0) {
        self->init_flag = 1;
    }
}

/* Arms the 90-frame swap timer and requests the swap sound effect. */
void q_result_swap_start(QResultScreen* self)
{
    self->swap_timer = 0x5A;
    fn_800DBDD4();
}

/* The first list's cursor as a flat index: column plus row times eight. */
u16 q_result_grid_cursor_index(QResultScreen* self)
{
    return self->grid_cursor_col + (self->grid_cursor_row << 3);
}

/* The second list's cursor as a flat index: column plus row times eight. */
u16 q_result_list_cursor_index(QResultScreen* self)
{
    return self->list_cursor_col + (self->list_cursor_row << 3);
}
/* Draws the fixed sprite-plus-value panel at sprite 0x11F2 (the band's money/points readout). */
void q_result_draw_icon_value(void)
{
    fn_802DF6E4(0x11F2);
}

/* Draws row `row`'s label with sprite `sprite_id` and the default sub-index. */
void q_result_font_print_a(s16 row, u8 sprite_id)
{
    q_result_font_print_row(row, 0, 0, sprite_id);
}

/* Draws row `row`'s label with sprite `sprite_id` and sub-index `flag`. */
void q_result_font_print_b(s16 row, s32 flag, u8 sprite_id)
{
    q_result_font_print_row(row, 1, flag, sprite_id);
}

/* Switches the band's page to mode 1. */
void q_result_page_step_a(QResultScreen* self)
{
    q_result_page_set(self, 1);
}

/* Switches the band's page to mode 3. */
void q_result_page_step_b(QResultScreen* self)
{
    q_result_page_set(self, 3);
}

/* The per-phase gate the screen's phase advance drives with: mode picks which load condition the
 * phase waits for, and the return says the phase may advance. */
BOOL q_result_phase_ck(QResultScreen* self, u8 mode)
{
    Q_ResultWork* q = get_qResult_work();

    switch (mode) {
    case 0:
        return FALSE;
    case 1:
        if (q_result_phase_is_2(self) == 1) {
            return FALSE;
        }
        if (q_result_phase_is_3(self) == 1) {
            return FALSE;
        }
        break;
    case 2:
        if (q_result_phase_is_2(self) == 1) {
            return FALSE;
        }
        if (q_result_ready_ck(self) == 0) {
            return FALSE;
        }
        if (q_result_phase_is_3(self) == 1) {
            return FALSE;
        }
        break;
    case 3:
        if (q->present_0x3A4 == 0) {
            return FALSE;
        }
        if (q_result_phase_is_3(self) == 1) {
            return FALSE;
        }
        break;
    case 4:
        break;
    case 5:
        if (q_result_phase_is_3(self) == 1) {
            return FALSE;
        }
        break;
    case 6:
        if (q->progress_0x1E0 < 10000) {
            return FALSE;
        }
        if (q_result_phase_is_3(self) == 1) {
            return FALSE;
        }
        if (q_result_file_ready(self) == 1) {
            return FALSE;
        }
        break;
    case 7:
        if (self->field_0x0100 <= 0) {
            return FALSE;
        }
        if (q_result_phase_is_3(self) == 1) {
            return FALSE;
        }
        if (q_result_file_ready(self) == 1) {
            return FALSE;
        }
        break;
    case 8:
        if (q_result_swap_counter_get(self) == 0) {
            return FALSE;
        }
        break;
    }

    return TRUE;
}

/* Whether either of the two sub-screen resource sets has finished loading. */
BOOL q_result_sub_screen_ready(void)
{
    if (fn_803B4C64(NULL) != 1) {
        if (fn_803B4CE8(NULL) != 1) {
            return FALSE;
        }
    }
    return TRUE;
}

/* The first grid's entry for the grid cursor, as a pointer into the record's inline table. */
u32* q_result_grid_entry(QResultScreen* self, u8 which)
{
    u32* table;

    switch (which) {
    case 0:
    case 2:
        table = self->grid_entries;
        break;
    case 1:
        table = self->list_entries;
        break;
    default:
        return NULL;
    }
    return &table[q_result_grid_cursor_index(self)];
}

/* The second list's entry for its cursor: the first 0x18 come from the record's own table and the
 * rest continue in the second one. */
u32* q_result_list_entry(QResultScreen* self)
{
    u16 index = q_result_list_cursor_index(self);

    if (index < 0x18) {
        return &self->table_a_0x3048[index];
    }
    return &self->table_b_0x304C[(u16)(index - 0x18)];
}

/* Resets one result row's state to the pair the caller hands in. */
void q_result_row_init(QResultScreen* self, void* src, u8 a, u8 b)
{
    self->field_0x3036 = 0;
    item_pair_copy(&self->field_0x3044, src);
    self->field_0x302A = a;
    self->field_0x302B = b;
}

/* The band's empty hook: the call site exists, the body is a bare return. */
void q_result_noop(void* unused)
{
    (void)unused;
}

} /* extern "C" */
