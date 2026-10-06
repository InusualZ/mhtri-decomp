/* lobby/lb_quest_ui.cpp - a lobby list/detail screen: scrolling rows of sprite icons and glyph text, the selected-row
 *   colour, the cursor, counts and localized strings, driven by `game_ready_ck` and the `fn_8021D5BC` state query.
 * RANGE. .text 0x8038EC44-0x80394038 (63 functions); .data 0x805F0CB8-0x805F13D0, .sdata 0x807933D8-0x80793470, .sdata2
 *   0x8079C298-0x8079C2B0, extab, extabindex.  The left edge is the end of the em009 TU (`fn_8038EBE8`, the deleting
 *   destructor that closes it, is `enemy/em009_act.cpp`'s); the first function of this TU could be anywhere up to
 *   0x8038EF28 (the first reader of the new `.sdata2` pool at `lbl_8079C2A4`), a GUESS.
 * NAMES. `lb_quest_ui` is a GUESS in the module's `lb_*` scheme: nothing pins which lobby screen this is (the `LbStr`
 *   ids index a runtime-loaded table).  Module `lobby`: `lobby_w` +0x0AC, `lb_param_w`, 28 `LbStr` calls; the lobby
 *   menu dispatcher `fn_80211E68` (`lobby/lb_npc.cpp`) calls `fn_80390F20`/`fn_80392CC8` as screen entry points. The
 *   function names are GUESSes from their bodies.
 * RESIDUALS. 46 rows unwritten or scoring zero: 0x8038EC44-0x8038EEC8, 0x8038EF1C-0x8038EF9C, 0x8038EFEC-0x8038F264,
 *   0x8038F4D8-0x8038F748, 0x8038F74C-0x8038F884, 0x8038F8A8-0x8038FAC4, 0x8038FB20-0x803905FC, 0x8039065C-0x80390928,
 *   0x80390984-0x80392328, 0x80392394-0x80392960, 0x80392964-0x80393A70, 0x80393B28-0x80394038, and the written
 *   `lb_ui_tri_sum` (0x8038F2BC-0x8038F344: retail is MWCC's 8x-unrolled countdown loop, its source shape unsettled).
 *  - `lb_ui_pair_offset`: argument register order; `lb_ui_page_flag_set`/`lb_ui_page_flag_ck`: retail narrows both
 *    `u16` arguments before calling `lb_ui_pair_offset`, ours passes them raw;
 *  - `lb_ui_clear`: the `memset` argument setup order; `lb_ui_pair_lookup`: the loop shape;
 *  - `lb_ui_angle_step`, `lb_ui_param_apply`, `lb_ui_effect_list_step`: one register name each.
 *   flipcheck: `.data`/`.sdata`/`.sdata2` claimed, not emitted; `.text`/extab/extabindex short of the claim; the
 *   `.sdata`/`.sdata2` pool is shared with `lobby/lb_quest_board.cpp` and `menu/menu_result.cpp` (fold candidate).
 *   `lb_ui_detail_step` hands its row work to `lb_quest_board_state_next` as the board's `LbQuestBoardWork` (one block,
 *   two views: a cast until the views are merged); `lb_ui_pair_tables`/`lb_ui_param_values` are GUESS names.
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
#include "lobby/lb_quest_board_reset.h"   /* `lb_quest_board_state_next`/`lb_quest_board_effect_retire` (rule 2) */

/* The `.data` pair/lookup tables (0x805F0EC8, this unit's): `lb_ui_pair_tables[kind]` points at a table of
 * 6-byte-stride records whose first u16 is the key, ended by a record whose u16 is 0xFFFF
 * (`lb_ui_pair_lookup`, `lb_ui_pair_offset`). */
extern u16* lb_ui_pair_tables[];
/* The 12-entry signed table (0x805F0CDC, this unit's) the option-parameter copy indexes by a byte flag
 * (`lb_ui_param_apply`). */
extern const s16 lb_ui_param_values[12];

/* One record of the screen's pooled-effect list at `+0x038`: a count followed by the handles the effect library
 * retires; the same record as `lobby/lb_quest_board.h`'s `LbEftList` (a rule-1 duplicate to fold). size: 0x8 */
typedef struct LbUiEftList {
    /* +0x000 */ s32 count_0x000;
    /* +0x004 */ void* handles_0x004[1];
} LbUiEftList;

/* The screen's row work: the box the screen's entry point is handed.  Only the fields the reconstructed bodies touch
 * are named; nothing in this range allocates or `memset`s it. size: 0x1A8 (approximate) */
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
    u16* p = lb_ui_pair_tables[kind];
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
        u8* page = lobby_world_block;
        page[(key >> 3) + 0x516C] |= (u8)(1 << (key & 7));
    } else {
        u8* page = lobby_world_block;
        page[(key >> 3) + 0x525C] |= (u8)(1 << (key & 7));
    }
}

/* Whether the pair's bit is set in the current page's set. */
int lb_ui_page_flag_ck(u16 a, u16 b) {
    u32 key = lb_ui_pair_offset((u16)a, (u16)b);
    if (game_ready_ck() == 0) {
        u8* page = lobby_world_block;
        return (page[(key >> 3) + 0x516C] & (u8)(1 << (key & 7))) != 0;
    }
    u8* page = lobby_world_block;
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
        s16 value = lb_ui_param_values[self->flags_0x4B[i]];
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
        lb_quest_board_state_next((struct LbQuestBoardWork*)self);
        break;
    case 3:
        lb_quest_board_effect_retire((struct LbQuestBoardWork*)self);
        break;
    }
}

}  /* extern "C" */
