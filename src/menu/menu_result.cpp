/*
 * menu/menu_result.cpp - the quest-result screen: the state machine over `get_qResult_work()`'s record, the message
 *   table `q_result_msg_adrs` (by id and language) and the sub-screens gated on `field_0x1E2` (2 loading, 3 loaded).
 *   C++ (the bodies call mangled callees such as `get_joint_wpos__6MHcharFUlPQ34nw4r4math4VEC3`).
 * RANGE. .text 0x803967F0-0x8039D278 (85 functions); extab, extabindex, .data 0x805F1500-0x805F1704 (the icon and
 *   message-id tables and three switch tables, `jumptable_805F1698` among them), .bss 0x806C5528-0x806C5558
 *   (`q_result_msg_adrs`), .sdata 0x80793488-0x80793520, .sdata2 0x8079C2E8-0x8079C330.  The left edge is a TU seam: the
 *   `.data` referrer runs break there (the run below ends at 0x803960BC, this unit's starts at 0x80396BBC).
 * FLAGS. `cflags_menu` (configure.py).
 * NAMES. Module `menu`: its tables sit between `menu_note.cpp` (0x805E91F8) and `menu_placeinfo.cpp` (0x80604780) in
 *   `.data`, and every callee is the menu library's.  No `__FILE__` string reaches the range and the dump answers `zz_`,
 *   so the file name and every function name (the `q_result_*` scheme) are GUESSes from the bodies; `q_result_msg_adrs`
 *   is the runtime dump's own name.
 * RESIDUALS. 57 rows unwritten (objdiff scores them zero): `fn_803967F0`, 0x80396A90-0x80396C18, `fn_80396CAC`,
 *   0x803970FC-0x80397320, 0x80397344-0x803988B4, 0x80398938-0x80398C14, 0x80398C88-0x80398F24,
 *   0x80398F7C-0x80399D44, 0x80399D4C-0x80399EE4, 0x80399F18-0x8039B9B0, 0x8039B9C0-0x8039CD18,
 *   0x8039CD1C-0x8039D278.  The one partial row, `q_result_sub_screen_ready`: MWCC if-converts the second
 *   `return (B == 1)` (a `beq` to the true return) where the target keeps the branch.
 *   flipcheck: `.bss` (0x30), `.sdata` (0x98) and `.sdata2` (0x48) claimed but not emitted; short `.text` 0x5DC of
 *   0x6A88, extab 0x58 of 0x200, extabindex 0x84 of 0x300, `.data` 0x24 of 0x204; the bytes of all four differ; the
 *   pools are partial (a candidate fold with `lobby/lb_quest_ui` and `lobby/lb_quest_board`); `fn_803B4C64` and
 *   `fn_803B4CE8` are defined by no link input.
 */

#include "types.h"
#include "menu/menu_result.h"

#include "Runtime.PPCEABI.H/memset.h"
#include "ef/eft_res.h"
#include "sound/fn_800D7F54.h"
#include "fn_8004CAD8.h"
#include "fn_80047398.h"
#include "enemy/em_pop.h"   /* quest_flag_10_ck (rule 2: its owner) */

/* The `sprintf`/`strcpy`/`strcat` family: their owners' headers do not declare them yet, so they are declared
 * here. */
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
    eft_res_slot_release(self);
}

/* The language-selected message table for one message id: `q_result_msg_adrs[id][language]`. */
char** q_result_msg_table(u8 id)
{
    return q_result_msg_adrs[id][system_w[9]];
}

/* One string of the language-selected message table for message `id`. */
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
