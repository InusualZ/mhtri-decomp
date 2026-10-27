/*
 * menu/fn_8031A6C0.cpp - the item combination screens and the eft043 hand-glow effect.  `MenuSel` (this unit's view
 *   of the 0x330-byte working record `menu/menu_item.h` names `MenuSlot`) is the screen state: the input bits at
 *   +0x4/+0x8, the 3+8 0x18-byte entry arrays at +0x24/+0x6C, the slot pairs at +0x210 and the embedded
 *   `MenuSelCursor` at +0x1EC that picks, confirms and runs one combination (`lb_combo_*`); `self[1]` is the second
 *   record the recipe list runs on.  `menu_equip_panel_step`/`menu_equip_panel_draw` are menu page 1 (combine from
 *   the pouch), `menu_page02_*` page 2 (combine from the eight recipe slots).  The tail 0x8031D294.. is the eft043
 *   player effect (`_EFT` records, `eft043_*`).  C++ callees, `extern "C"` bodies.
 * RANGE. .text 0x8031A6C0-0x8031DAA8 (45 functions); extab, extabindex, .ctors 0x8056F398, .data 0x805DCE98-0x805DCF38,
 *   .bss 0x806BE108-0x806BE120, .sdata 0x80792C20-0x80792C48, .sdata2 0x8079AE50-0x8079AE70.  Not one TU: the eft043
 *   tail's data starts on fresh 8-byte boundaries (`.data` 0x805DCED4-0x805DCED8 and `.sdata` 0x80792C3A-0x80792C40 are
 *   padding), it owns every `.sdata2` word, the `.bss` and the `.ctors` word, and its only outside caller is
 *   `Pl/fn_802840DC.cpp` - seam request filed (menu-8031a6c0 requests).  The left edge continues
 *   `menu/menu_infomation.cpp`'s extab run.
 * FLAGS. `cflags_menu` (configure.py: `-opt nopeephole`); `#pragma optimization_level 4` around
 *   `menu_equip_entries_update` and `menu_page02_draw` (playbook 108: retail's subi+cmplwi switch range tests).
 * NAMES. Module `menu` from the callees (`get_menu_lsp_tbl`, `GetMenuFontColor`, `ItemName`, `draw_font`, `lb_combo_*`)
 *   and the bracketing menu units; no `__FILE__` string covers the range and the dump answers `zz_`, so the file keeps
 *   the map's stem (a rename to `menu/menu_combo.cpp` plus `ef/eft043.cpp` is in the seam request).  `eft043` from the
 *   record's type byte (0x2B), as `menu/fn_8031EA8C.cpp`'s `eft045`.
 *   GUESS (from each body and its callers): every `menu_sel_*`, `menu_equip_*`, `menu_item_cell_draw`,
 *   `menu_item_two_page_*`, `menu_shared_slot_id_*`, `menu_page02_*` name, the `eft043_*` family and the data names;
 *   foreign (registry): `pl_ryugeki_se_req`, `pl_act4_phase_ck`, `se_combo_mix_req`, `se_combo_result_req`,
 *   `menu_combo_list_open`, `menu_combo_list_step`, `pl_weapon_drawn_ck`.
 * RESIDUALS. Every row is written.  The rows short of 100 %, by cause:
 *  - `menu_equip_entries_update`: register allocation of the slot/cell pair (r22/r24) once the level-4 switch matches;
 *  - argument evaluation order: retail computes the count, the `!= 0xFFFF` flag and `focused` in argument order, ours
 *    loads `pos`/`item` first (`menu_sel_pair_draw`; locals for all three measured worse);
 *  - a byte prompt index retail re-extends with `extsb` at its compare and at `get_menu_msg` (`menu_equip_panel_draw`,
 *    `menu_page02_draw`, `menu_page02_step`'s last case), and `li` placement around it (level 4 changes nothing
 *    in the other open rows);
 *  - register allocation only (`menu_item_cell_draw` keeps `pos` in r28, `eft043_start`/`eft043_move` swap the loop
 *    registers, `menu_sel_cursor_confirm_step` r30/r31, `menu_sel_tabs_build` r30/r31, `menu_sel_cursor_status_update`
 *    the `clrlwi` of `userdata_box_capacity`'s result);
 *  - `menu_sel_cursor_result_draw`: retail compares `mode_0x16 == 2` with `cmpwi`, and builds the digit colour as
 *    `-1 & mask`; `menu_sel_cursor_confirm_draw`: one `extsh` ours adds to the window offset.
 *   flipcheck: `.sdata` 0x22 of 0x28 and the eft043 data shifted by the seam padding; `.text` 4 bytes long; foreign
 *   names `pl_ryugeki_se_req`, `pl_act4_phase_ck`, `se_combo_mix_req`, `se_combo_result_req`, `menu_combo_list_step`
 *   wait for the registry batch.
 */

#include "types.h"
#include "ef/eft_res.h"
#include "menu/menu_effect_slot.h"
#include "menu/fn_8031A6C0.h"
#include "menu/menu_item.h"
#include "menu/menu_message.h"
#include "sound/fn_800D7F54.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/effect.h"
#include "pl.h" /* _PLW */
#include "gx.h" /* _GXColor */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 */
#include "fn_8004CAD8.h" /* calcVecAngXY / rotVecX / rotVecY / cpSetRotMatrix / getKeyData3 / MTX34_ctor */
#include "sound/fn_800DD1F0.h" /* mhchar_joint_mtx_get */
#include "g3d/g3d_calcworld.h" /* addVec3To */
#include "ef/eft001.h" /* mtx34_set_trans */
#include "ef/mtx34_trans_get.h"
#include "ef/eft_stage_spawn_flag.h"
#include "ef/RetireParticleAll.h"
#include "ef/push_eft_effect_heap_num.h"
#include "unsplit/ef_control.h" /* eft_control */
#include "Pl/Pl_master_ck.h"
#include "Pl/pl_act4_phase_ck.h"
#include "Pl/pl_weapon_drawn_ck.h"
#include "sound/pl_ryugeki_se_req.h"
#include "stage/stg_w.h" /* get_now_areano */
#include "MSL_C/printf.h" /* sprintf */
#include "font/flfnt.h" /* font_flush */
#include "ef/system_core.h" /* get_str_tbl */
#include "lobby/spr_data_copy.h"
#include "menu/menu_select_window_draw.h"
#include "lobby/lb_combo_rate_get.h"
#include "lobby/lb_combo_rate_sprintf.h"
#include "lobby/lb_combo_pair_recipe_find.h"
#include "lobby/lb_page_row_pair_get.h"
#include "menu/menu_slot_index_is_page_b.h"
#include "menu/menu_combo_list_step.h"
#include "menu/menu_choice_window_draw.h"
#include "menu/item_page_draw.h"
#include "menu/menu_cursor_column_step.h"
#include "menu/menu_frame_draw_page.h"
#include "menu/menu_frame_entries_build.h"
#include "hud/cockpit.h" /* item_get_with_msg */
#include "ef/eft052.h" /* hud_item_msg_push */
#include "hud/layout.h" /* get_lsp_data and the draw_* family */
#include "hud/hud_notice_spawn.h"
#include "menu/menu_slot_find.h"
#include "lobby/lb_combo_recipe_learn.h"
#include "lobby/lb_combo_recipe_known_ck.h"
#include "sound/se_combo_mix_req.h"
#include "sound/se_combo_result_req.h"
#include "userdata_item.h" /* item_slots_free_count */
#include "userdata_item/item_slots_room_get.h"
#include "userdata_item/item_slots_count_sum.h"
#include "userdata_item/userdata_box_capacity.h"

/* Callees other units own (`menu_slot_get` `menu/menu_message.cpp`, `lb_combo_item_known_ck` `lobby/lb_companion_ui.cpp`,
 * `item_count_find` `userdata_item.cpp`, ...) in this unit's record views; their owners' headers do not declare them yet. */

extern "C" {

u16* menu_slot_get(MenuSel* self, s16 index);
s32  lb_combo_item_known_ck(u16 a);
s32  lb_combo_recipe_find(u16 a, u16 b, s32 c);
s32  lb_combo_exec(u8* yield, u16 a, u16 b, s32 kind, u16 c, u16 d);
s32  item_count_find(u16 a, IdValue* b, s32 c);
s32  item_room_or_full_get(u16 a, IdValue* b, s32 c);
extern MenuSharedSlots* lobby_world_block;
}

extern "C" {
/* The eight layout ids the equipment panel's combination result is drawn at. */
u16 menu_equip_result_lsp_ids[8] = { 0x038B, 0x03B3, 0x03B7, 0x03AC, 0x03C4, 0x03D2, 0x03CF, 0x03B8 };
/* The digit sprites the box count draws with. */
u16 menu_sel_box_digit_ids[4] = { 0x03C1, 0x03C2, 0x03C3, 0xFFFF };
/* The two row-frame tables the second page draws with (GUESS). */
u8 menu_page02_row_frames_a[4] = { 9, 8, 10, 0 };
u8 menu_page02_row_frames_b[4] = { 2, 11, 13, 0 };
/* The count format the item cells and the result panel print with. */
char menu_sel_count_fmt[3] = "%d";
}

/* This unit's functions, declared ahead of their first use. */
extern "C" {
void menu_sel_cursor_init(MenuSelCursor* cur, u8 kind, MenuSel* owner);
void menu_sel_cursor_first_set(MenuSelCursor* cur, u16 slot, u16 item);
u32 menu_sel_cursor_second_set(MenuSelCursor* cur, u16 slot, u16 item);
u32 menu_sel_cursor_status_update(MenuSelCursor* cur);
s32 menu_sel_cursor_confirm_step(MenuSelCursor* cur, s32* flag, u16 a, u16 b, u16 c);
void menu_sel_cursor_combine_start(MenuSelCursor* cur, u8 which);
u32 menu_sel_cursor_combine_step(MenuSelCursor* cur, u16 trig, u16 hold);
void menu_sel_cursor_apply_start(MenuSelCursor* cur);
s32 menu_sel_cursor_apply_step(MenuSelCursor* cur, u16 trig, u16 hold);
void menu_sel_cursor_blend_tick(MenuSelCursor* cur);
void menu_sel_pair_draw(MenuSel* self, u8 side);
void menu_sel_cursor_result_draw(MenuSelCursor* cur, const u16* lsp_ids);
void menu_sel_cursor_confirm_draw(MenuSelCursor* cur, s32 sel, u16 lsp_id);
s32 menu_sel_slot_item_ck(MenuSel* self, u16 slot, u16 item);
s32 menu_sel_pair_resolve(MenuSel* self, u16 a, u16 b);
void menu_equip_entries_update(MenuSel* self);
s32 menu_sel_cursor_hold_tick(MenuSelCursor* cur, u16 bits);
void menu_sel_record_copy(u16* dst, u16* src);
}

/* 0x8031A6C0 (0x544): the equipment panel's frame step: picks the first and second item of a combination, confirms
 * it, runs it and files the result; returns 2 when the panel closes. */
extern "C" s32 menu_equip_panel_step(MenuSel* self) {
    s32 result = 0;
    MenuSelCursor* cur = &self->cursor_0x1EC;
    u16 keys = (self->input_0x008 & 0xF) | (self->input_0x004 & 0x70);
    u16 slot;
    u16 item;

    self->value_0x19C = keys;
    self->value_0x19E = 0;
    if (self->state_0x001 <= 1) {
        if (self->fade_0x23A != 0) self->fade_0x23A--;
    } else {
        if (self->fade_0x23A < 3) self->fade_0x23A++;
    }
    menu_sel_cursor_blend_tick(cur);
    switch (self->state_0x001) {
    case 0:
        if (keys & 0x10) {
            slot = self->sel_0x1A2 + self->count_0x1A3 * self->col_0x1A0;
            item = menu_slot_get(self, slot)[0];
            if (menu_sel_slot_item_ck(self, slot, item) != 0) {
                self->state_0x001 = 1;
                menu_sel_cursor_first_set(cur, slot, item);
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (keys & 0x20) {
            result = 2;
            sysSE_req(1);
        } else {
            menu_cursor_column_step((MenuListWork*)&self->value_0x19C);
        }
        menu_slot_entry_b_select((MenuSlot*)self, self->sel_0x1A2);
        break;
    case 1:
        if (menu_sel_slot_item_ck(self, cur->slot_a_0x08, cur->item_a_0x04) == 0) {
            menu_sel_cursor_init(cur, 0, self);
            self->state_0x001 = 0;
            break;
        }
        if (keys & 0x10) {
            slot = self->sel_0x1A2 + self->count_0x1A3 * self->col_0x1A0;
            if (menu_sel_pair_resolve(self, slot, cur->slot_a_0x08) != 0) {
                if (menu_sel_cursor_second_set(cur, slot, menu_slot_get(self, slot)[0]) == 1) {
                    self->flag_0x18C = 0;
                    if (menu_sel_cursor_status_update(cur) == 0) {
                        self->flag_0x18C = 1;
                    }
                    self->state_0x001 = 2;
                    sysSE_req(0);
                } else {
                    sysSE_req(2);
                }
            } else {
                sysSE_req(2);
            }
        } else if (keys & 0x20) {
            menu_sel_cursor_init(cur, 0, self);
            self->state_0x001 = 0;
            sysSE_req(1);
        } else {
            menu_cursor_column_step((MenuListWork*)&self->value_0x19C);
        }
        menu_slot_entry_b_select((MenuSlot*)self, self->sel_0x1A2);
        break;
    case 2:
        cur->mode_0x16 = 0;
        if (menu_sel_cursor_status_update(cur) == 0) {
            self->flag_0x18C = 1;
        }
        switch (menu_sel_cursor_confirm_step(cur, &self->flag_0x18C, self->input_0x006, self->input_0x008,
                                             self->input_0x004)) {
        case 1:
            menu_sel_cursor_combine_start(cur, 0);
            self->state_0x001 = 3;
            self->wait_0x013 = 1;
            break;
        case 2:
            if (cur->slot_a_0x08 == 0xFFFF || cur->slot_b_0x0A == 0xFFFF) {
                menu_sel_cursor_init(cur, 0, self);
                self->state_0x001 = 0;
            } else {
                menu_sel_cursor_second_set(cur, 0xFFFF, 0);
                self->state_0x001 = 1;
            }
            break;
        }
        menu_slot_entry_b_select((MenuSlot*)self, 8);
        break;
    case 3:
        self->wait_0x013 = 1;
        if (menu_sel_cursor_combine_step(cur, self->input_0x004, self->input_0x006) != 1) {
            break;
        }
        self->state_0x001 = 4;
        /* fall through */
    case 4:
        self->wait_0x013 = 1;
        switch (item_get_with_msg(self->worker_0x190, cur->value_0x0C, self->ticket_0x1E8, 0, cur->anim_0x15)) {
        case 0:
        case 1:
            menu_sel_cursor_apply_start(cur);
            self->state_0x001 = 5;
            break;
        case 2:
        case 3:
            hud_item_msg_push(2, 0x11, cur->value_0x0C);
            menu_sel_cursor_apply_start(cur);
            self->state_0x001 = 5;
            break;
        case 5:
            self->state_0x001 = 6;
            cur->mode_0x16 = 0;
            cur->timer_0x0F = 0;
            break;
        }
        break;
    case 5:
        self->wait_0x013 = 1;
        switch (menu_sel_cursor_apply_step(cur, self->input_0x004, self->input_0x006)) {
        case 1:
            menu_sel_cursor_combine_start(cur, 1);
            self->state_0x001 = 3;
            break;
        case 2:
            if (cur->status_0x10 >= 0) {
                self->flag_0x18C = 1;
            }
            self->state_0x001 = 2;
            break;
        }
        break;
    case 6:
        self->wait_0x013 = 1;
        self->state_0x001 = 2;
        if (menu_sel_cursor_status_update(cur) == 0) {
            self->flag_0x18C = 1;
        }
        if (cur->slot_a_0x08 != 0xFFFF && cur->slot_b_0x0A != 0xFFFF &&
            lb_combo_recipe_find(cur->item_a_0x04, cur->item_b_0x06, 0) == 0) {
            menu_sel_cursor_init(cur, 0, self);
            self->state_0x001 = 0;
        }
        break;
    }
    menu_equip_entries_update(self);
    return result;
}

/* ============================================================================================== */
/* 0x8031AC04 - hand the display worker to its update routine                                     */
/* ============================================================================================== */
extern "C" void menu_slot_worker_busy_ck(MenuSel* self) {
    pl_weapon_drawn_ck(self->worker_0x190);
}

/* 0x8031AC0C - does item id `a` resolve to the slot `b`? */
extern "C" s32 menu_sel_slot_item_ck(MenuSel* self, u16 a, u16 b) {
    if (a == 0xFFFF) return 0;
    u16* rec = menu_slot_get(self, (s16)a);
    if ((s16)rec[1] == 0) return 0;
    if (rec[0] != b) return 0;
    return lb_combo_item_known_ck(b);
}

/* 0x8031AC80 - resolve the pair `(a, b)` into the cursor's record */
extern "C" s32 menu_sel_pair_resolve(MenuSel* self, u16 a, u16 b) {
    if (a == 0xFFFF) return 0;
    if (a == b) return 0;
    u16* rec = menu_slot_get(self, (s16)a);
    if ((s16)rec[1] == 0) return 0;
    u16* rec2 = menu_slot_get(self, (s16)b);
    s32 r = lb_combo_recipe_find(rec2[0], rec[0], 0);
    self->cursor_0x1EC.record_0x00 = (MenuComboRecipe*)r;
    return r != 0;
}

/* 0x8031AD30 - the item's base value plus the second slot's */
extern "C" s32 menu_item_two_page_count(u16 id, IdValue* b, IdValue* c) {
    s32 r = item_count_find(id, b, 0x18);
    if (c != 0) r += item_count_find(id, c, 8);
    return r;
}

/* 0x8031AD9C - files the item into the second page when the first has none, else into the first */
extern "C" s32 menu_item_two_page_store(u16 id, IdValue* a, IdValue* b) {
    if (item_count_find(id, a, 0x18) == 0 && b != 0 && GetItemData(id)->kind_0x00 == 1) {
        s32 r = item_room_or_full_get(id, b, 8);
        if (r >= 0) return r;
    }
    return item_room_or_full_get(id, a, 0x18);
}

#pragma optimization_level 4
/* 0x8031AE38 (0x248): refreshes the colours and availability of the eight visible entries for the panel's state. */
extern "C" void menu_equip_entries_update(MenuSel* self) {
    s16 slot = (s8)self->count_0x1A3 * (s8)self->col_0x1A0;
    s16 index = slot;
    MenuSelEntry* cell;
    s32 i;
    u16* rec;

    if (menu_slot_index_is_page_b((MenuSlot*)self, slot) != 0) {
        slot -= 0x18;
    }
    cell = self->entries_b_0x06C;
    i = 0;
    do {
        cell->slot_0x06 = slot;
        cell->available_0x02 = 0;
        cell->color_a_0x0C = 0;
        rec = menu_slot_get(self, index);
        switch (self->state_0x001) {
        case 0:
            if (lb_combo_item_known_ck(rec[0]) != 0) {
                cell->available_0x02 = 1;
            }
            break;
        case 1:
            if (lb_combo_recipe_find(self->cursor_0x1EC.item_a_0x04, rec[0], 0) != 0) {
                cell->available_0x02 = 1;
            }
            if (self->cursor_0x1EC.slot_a_0x08 == index) {
                cell->color_a_0x0C = 0xF3D73EFF;
                cell->color_b_0x10 = 0xF53737FF;
                cell->color_c_0x14 = -1;
            }
            break;
        case 2:
        case 3:
            if (self->cursor_0x1EC.slot_a_0x08 == index || self->cursor_0x1EC.slot_b_0x0A == index) {
                cell->color_a_0x0C = 0xF3D73EFF;
                cell->color_b_0x10 = 0xF53737FF;
                cell->color_c_0x14 = -1;
            }
            if (self->cursor_0x1EC.flag_0x12 == 0 && self->cursor_0x1EC.record_0x00 != NULL &&
                rec[0] == (s16)self->cursor_0x1EC.record_0x00->result_0x02) {
                if ((s16)rec[1] < GetItemData(rec[0])->max_num_0x003) {
                    cell->color_a_0x0C = 0xF0F0F0FF;
                } else {
                    cell->color_a_0x0C = 0xF53737FF;
                }
                cell->color_b_0x10 = 0xF53737FF;
                cell->color_c_0x14 = -1;
            }
            break;
        case 4:
        case 5:
        case 6:
            if (self->cursor_0x1EC.slot_a_0x08 == index || self->cursor_0x1EC.slot_b_0x0A == index) {
                cell->color_a_0x0C = 0xF3D73EFF;
                cell->color_b_0x10 = 0xF53737FF;
                cell->color_c_0x14 = -1;
            }
            if (rec[0] == self->cursor_0x1EC.value_0x0C) {
                if ((s16)rec[1] < GetItemData(rec[0])->max_num_0x003) {
                    cell->color_a_0x0C = 0xF0F0F0FF;
                } else {
                    cell->color_a_0x0C = 0xF53737FF;
                }
                cell->color_b_0x10 = 0xF53737FF;
                cell->color_c_0x14 = -1;
            }
            break;
        }
        i++;
        cell++;
        index++;
        slot++;
    } while (i < 8);
}
#pragma optimization_level reset

/* 0x8031B080 (0x250): draws the equipment panel: the item page, the pair, the result panel and the state's prompt. */
extern "C" void menu_equip_panel_draw(MenuSel* self) {
    MenuSelCursor* cur = &self->cursor_0x1EC;
    s32 page_b = 0;
    s32 flags = 4;
    u8 result = 0;
    s32 confirm = 0;
    s8 prompt = -1;
    _mh_ivec2_ pos;

    if (self->value_0x19E & 4) {
        flags |= 8;
    }
    if (self->value_0x19E & 8) {
        flags |= 0x10;
    }
    if (menu_slot_index_is_page_b((MenuSlot*)self, (s8)self->col_0x1A0 * (s8)self->count_0x1A3) != 0) {
        page_b = 1;
        flags |= 1;
    }
    switch (self->state_0x001) {
    case 0:
        prompt = 0;
        break;
    case 1:
        prompt = 1;
        break;
    case 2:
        prompt = 3;
        if (self->flag_0x18C == 0) {
            prompt = 2;
        }
        result = 1;
        confirm = 1;
        break;
    case 3:
        prompt = 6;
        result = (cur->flag_0x11 == 0) + 1;
        break;
    case 5:
        result = 3;
        prompt = 5;
        if (cur->value_0x0C != 0x178) {
            prompt = 4;
        }
        break;
    case 6:
        result = 3;
        break;
    }
    if (self->flag_0x1B0 == 0) {
        menu_frame_draw_page(page_b == 0 ? self->page_a_0x194 : self->page_b_0x198, (MenuEntry*)self->entries_b_0x06C,
                             self->col_0x1A0 + 1, self->row_0x1A1, self->fade_0x23A, flags);
    } else {
        menu_frame_entries_build((u16*)self->page_a_0x194, (u16*)self->page_b_0x198, self->row_0x1A1, 0x274, 0);
    }
    menu_sel_pair_draw(self, self->state_0x001);
    if (result != 0) {
        menu_sel_cursor_result_draw(cur, menu_equip_result_lsp_ids);
    }
    if (prompt >= 0) {
        get_lsp_data(0x2DC, &pos);
        draw_window_frame_preset(pos.x, pos.y, 0);
        font_set_size(0x10, 0x10);
        font_print_ex(pos.x + 0x18, pos.y + 0x10, 0, (s8*)get_menu_msg(0x26, prompt));
    }
    if (confirm != 0) {
        menu_sel_cursor_confirm_draw(cur, self->flag_0x18C, 0x3F0);
    }
}

/* ============================================================================================== */
/* 0x8031B2D0 / 0x8031B2FC / 0x8031B31C - the eight-slot id table the screen shares with the menu  */
/* ============================================================================================== */
extern "C" u16 menu_shared_slot_id_get(s16 idx) {
    if (idx >= 8) return 0xFFFF;
    return lobby_world_block->ids_0x39A2[idx];
}

extern "C" void menu_shared_slot_id_set(s16 idx, u16 value) {
    if (idx < 8) lobby_world_block->ids_0x39A2[idx] = value;
}

extern "C" void menu_shared_slot_id_clear(s16 idx) {
    if (idx < 8) lobby_world_block->ids_0x39A2[idx] = 0xFFFF;
}

/* 0x8031B344 - mark the visible window's entries available */
extern "C" void menu_sel_window_reset(MenuSel* self) {
    MenuSelEntry* cell = self->entries_b_0x06C;
    s16 slot = (s16)((s8)self->col_0x1A0 * (s8)self->count_0x1A3);
    for (s32 i = 0; i < (s8)self->count_0x1A3; i++) {
        cell->available_0x02 = 1;
        cell->slot_0x06 = slot;
        cell->color_a_0x0C = 0;
        slot++;
        cell++;
    }
}

/* 0x8031B39C (0xC0): fills the eight slot pairs of the visible window from the shared slot ids (the recipe's result
 * and its count), then resets the window's entries. */
extern "C" void menu_sel_window_build(MenuSel* self) {
    s16 slot = (s8)self->col_0x1A0 * (s8)self->count_0x1A3;
    s32 i = 0;
    MenuComboRecipe* recipe;

    do {
        u16 id = menu_shared_slot_id_get(slot);
        if (id != 0xFFFF && (recipe = (MenuComboRecipe*)lb_combo_pair_recipe_find(id, 0)) != NULL) {
            self->slots_0x210[i].id_0x00 = recipe->result_0x02;
            self->slots_0x210[i].amount_0x02 =
                menu_item_two_page_count(recipe->result_0x02, self->page_a_0x194, self->page_b_0x198);
        } else {
            self->slots_0x210[i].id_0x00 = 0;
            self->slots_0x210[i].amount_0x02 = 0;
        }
        i++;
        slot++;
    } while (i < 8);
    menu_sel_window_reset(self);
}

/* 0x8031B45C - clear one column and its slot pair */
extern "C" void menu_sel_column_clear(MenuSel* self, u8 idx) {
    menu_shared_slot_id_clear(self->entries_b_0x06C[idx].slot_0x06);
    self->slots_0x210[idx].id_0x00 = 0;
    self->slots_0x210[idx].amount_0x02 = 0;
    menu_sel_window_reset(self);
}

/* 0x8031B4C4 (0x10C): loads the selected column's recipe pair into the cursor and resolves both items' slots. */
extern "C" void menu_sel_cursor_pair_load(MenuSel* self) {
    MenuSelCursor* cur = &self->cursor_0x1EC;

    menu_sel_cursor_init(cur, 1, self);
    if (menu_shared_slot_id_get(self->entries_b_0x06C[self->sel_0x1A2].slot_0x06) == 0xFFFF) {
        cur->record_0x00 = NULL;
        cur->item_a_0x04 = 0;
        cur->item_b_0x06 = 0;
    } else {
        cur->record_0x00 = (MenuComboRecipe*)lb_page_row_pair_get(menu_shared_slot_id_get(self->entries_b_0x06C[self->sel_0x1A2].slot_0x06),
                                                &cur->item_a_0x04, &cur->item_b_0x06, 0);
    }
    cur->slot_a_0x08 = menu_slot_find((MenuSlot*)self, cur->item_a_0x04);
    if (cur->slot_a_0x08 == 0xFF) {
        cur->slot_a_0x08 = 0xFFFF;
    }
    cur->slot_b_0x0A = menu_slot_find((MenuSlot*)self, cur->item_b_0x06);
    if (cur->slot_b_0x0A == 0xFF) {
        cur->slot_b_0x0A = 0xFFFF;
    }
    cur->flag_0x11 = 1;
    cur->state_0x18 = 0;
}

/* 0x8031B5D0 - reset the work area's second record */
extern "C" void menu_sel_second_reset(MenuSel* self) {
    MenuSel* second = self + 1;
    menu_combo_list_open((MenuSlot*)second);
    second->state_0x001 = 1;
    second->field_0x002 = 1;
    second->field_0x01C = 2;
    second->fade_0x23A = 0x29;
    second->field_0x23B = 5;
}

/* 0x8031B624 (0xAC): fills the three tab entries with their labels and marks tab `sel` selected. */
extern "C" void menu_sel_tabs_build(MenuSel* self, s16 sel) {
    MenuSelEntry* tab = self->entries_a_0x024;
    s32 i = 0;

    do {
        tab->selected_0x01 = 0;
        tab->available_0x02 = 1;
        tab->data_0x08 = get_menu_msg(0x28, i);
        tab->color_a_0x0C = 0;
        i++;
        tab++;
    } while (i < 3);
    if (self->cursor_0x1EC.status_0x10 >= 0) {
        self->entries_a_0x024[0].available_0x02 = 0;
    }
    if (sel <= self->field_0x016) {
        self->entries_a_0x024[sel].selected_0x01 = 1;
    }
}

/* 0x8031B6D0 (0xA4): opens the combination page: an eight-row window on the first page, the cursor on its first
 * recipe. */
extern "C" void menu_page02_open(MenuSel* self) {
    self->mode_0x014 = 2;
    self->flag_0x1B0 = 0;
    self->value_0x19E = 0;
    self->value_0x017 = 8;
    self->count_0x1A3 = 8;
    self->col_0x1A0 = 0;
    self->row_0x1A1 = menu_page_count(8, 8);
    self->sel_0x1A2 = 0;
    self->fade_0x23A = 0;
    self->state_0x001 = 0;
    menu_sel_window_build(self);
    menu_slot_entry_b_select((MenuSlot*)self, self->sel_0x1A2);
    menu_sel_cursor_init(&self->cursor_0x1EC, 1, self);
    menu_sel_cursor_pair_load(self);
}

/* 0x8031B774 (0x5DC): the combination page's frame step: picks a recipe slot, confirms it from the three-tab
 * window, runs the combination and files the result; state 10 places a recipe from the list on the second record.
 * Returns 2 when the page closes. */
extern "C" s32 menu_page02_step(MenuSel* self) {
    s32 result = 0;
    u16 keys = self->input_0x004 | (self->input_0x008 & 0xF);
    MenuSelCursor* cur = &self->cursor_0x1EC;
    MenuSel* list;
    s32 held;
    s32 go;
    s16 got;

    self->value_0x19C = keys;
    self->value_0x19E = 0;
    if (self->state_0x001 <= 1) {
        if (cur->blend_0x20 != 0) cur->blend_0x20--;
    } else {
        if (cur->blend_0x20 < 3) cur->blend_0x20++;
    }
    if (cur->mode_0x16 <= 1) {
        if (cur->blend_0x21 != 0) cur->blend_0x21--;
    } else {
        if (cur->blend_0x21 < 3) cur->blend_0x21++;
    }
    if (cur->counter_0x22 < 2) cur->counter_0x22++;
    switch (self->state_0x001) {
    case 0:
        if (self->fade_0x23A != 0) self->fade_0x23A--;
        if (keys & 0x10) {
            if (self->slots_0x210[self->entries_b_0x06C[self->sel_0x1A2].slot_0x06].id_0x00 != 0) {
                menu_sel_cursor_pair_load(self);
                if (menu_sel_cursor_status_update(cur) == 1) {
                    self->index_0x01A = 0;
                } else {
                    self->index_0x01A = 1;
                }
                self->state_0x001 = 2;
                self->field_0x016 = 3;
                menu_sel_tabs_build(self, self->index_0x01A);
            } else {
                menu_sel_second_reset(self);
                self->state_0x001 = 10;
            }
            sysSE_req(0);
        } else if (keys & 0x20) {
            result = 2;
            sysSE_req(1);
        } else {
            menu_cursor_column_step((MenuListWork*)&self->value_0x19C);
            menu_sel_cursor_pair_load(self);
            menu_sel_cursor_status_update(cur);
        }
        menu_slot_entry_b_select((MenuSlot*)self, self->sel_0x1A2);
        break;
    case 1:
        self->state_0x001 = 2;
        /* fall through */
    case 2:
        cur->mode_0x16 = 0;
        held = menu_sel_cursor_hold_tick(cur, self->input_0x006);
        menu_sel_cursor_status_update(cur);
        if (keys & 0x20) {
            self->state_0x001 = 0;
            sysSE_req(1);
            break;
        }
        if (held != 0 && self->index_0x01A == 0) {
            go = 1;
        } else {
            go = 0;
        }
        if ((keys & 0x10) || go != 0) {
            switch (self->index_0x01A) {
            case 0:
                if (cur->status_0x10 < 0) {
                    menu_sel_cursor_combine_start(cur, 0);
                    self->state_0x001 = 3;
                    self->wait_0x013 = 1;
                } else {
                    sysSE_req(2);
                }
                break;
            case 1:
                menu_sel_second_reset(self);
                self->state_0x001 = 10;
                sysSE_req(0);
                break;
            case 2:
                self->state_0x001 = 0;
                menu_sel_column_clear(self, self->sel_0x1A2);
                sysSE_req(0);
                break;
            }
        } else if (keys & 3) {
            self->index_0x01A = menu_cursor_step(self->index_0x01A, self->field_0x016, keys, 1, 2);
        }
        menu_sel_tabs_build(self, self->index_0x01A);
        break;
    case 3:
        self->wait_0x013 = 1;
        if (menu_sel_cursor_combine_step(cur, self->input_0x004, self->input_0x006) != 1) {
            break;
        }
        self->state_0x001 = 4;
        /* fall through */
    case 4:
        self->wait_0x013 = 1;
        got = item_get_with_msg(self->worker_0x190, cur->value_0x0C, self->ticket_0x1E8, 0, cur->anim_0x15);
        menu_sel_window_build(self);
        switch (got) {
        case 0:
        case 1:
            menu_sel_cursor_apply_start(cur);
            self->state_0x001 = 5;
            break;
        case 2:
        case 3:
            hud_item_msg_push(2, 0x11, cur->value_0x0C);
            menu_sel_cursor_apply_start(cur);
            self->state_0x001 = 5;
            break;
        case 5:
            self->state_0x001 = 6;
            cur->mode_0x16 = 0;
            cur->timer_0x0F = 0;
            break;
        }
        break;
    case 5:
        self->wait_0x013 = 1;
        switch (menu_sel_cursor_apply_step(cur, self->input_0x004, self->input_0x006)) {
        case 1:
            menu_sel_cursor_combine_start(cur, 1);
            self->state_0x001 = 3;
            break;
        case 2:
            self->state_0x001 = 2;
            break;
        }
        break;
    case 6:
        self->wait_0x013 = 1;
        self->state_0x001 = 2;
        if (cur->slot_a_0x08 != 0xFFFF && cur->slot_b_0x0A != 0xFFFF &&
            lb_combo_recipe_find(cur->item_a_0x04, cur->item_b_0x06, 0) == 0) {
            menu_sel_cursor_init(cur, 0, self);
            self->state_0x001 = 0;
        }
        break;
    case 10:
        list = self + 1;
        menu_sel_record_copy(&list->input_0x004, &self->input_0x004);
        list->frame_0x23C++;
        if (list->frame_0x23C > sprite_ary_last_frame((const u16*)get_menu_lsp_tbl(0x53))) {
            list->frame_0x23C = 0;
        }
        menu_combo_list_step((MenuSlot*)list);
        if (list->field_0x002 == 0) {
            if (self->slots_0x210[self->entries_b_0x06C[self->sel_0x1A2].slot_0x06].id_0x00 != 0) {
                self->state_0x001 = 2;
            } else {
                self->state_0x001 = 0;
            }
            sysSE_req(1);
        } else if (keys & 0x10) {
            if (list->entries_b_0x06C[list->list_row_0x1A7].available_0x02 != 0) {
                menu_shared_slot_id_set(self->entries_b_0x06C[self->sel_0x1A2].slot_0x06,
                                        list->entries_b_0x06C[list->list_row_0x1A7].slot_0x06);
                self->state_0x001 = 0;
                sysSE_req(0);
                menu_sel_window_build(self);
            } else {
                sysSE_req(2);
            }
        }
        break;
    }
    return result;
}

/* 0x8031BD50 - copy the five halfwords of one cursor record */
extern "C" void menu_sel_record_copy(u16* dst, u16* src) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
    dst[4] = src[4];
}

#pragma optimization_level 4
/* 0x8031BD7C (0x270): draws the combination page: the recipe slots, the pair, the result panel, the state's prompt
 * and the tab window; state 10 draws the recipe list instead. */
extern "C" void menu_page02_draw(MenuSel* self) {
    MenuSelCursor* cur = &self->cursor_0x1EC;
    s32 flags = 0x1003;
    u8 result = 0;
    s32 tabs = 0;
    s8 prompt = -1;
    s8 note = -1;
    _mh_ivec2_ pos;

    switch (self->state_0x001) {
    case 0:
        if (self->slots_0x210[self->entries_b_0x06C[self->sel_0x1A2].slot_0x06].id_0x00 != 0) {
            prompt = 7;
            if (cur->status_0x10 >= 0) {
                note = menu_page02_row_frames_a[cur->status_0x10];
            }
        } else {
            prompt = 0xC;
        }
        break;
    case 1:
    case 2:
        flags |= 0x200;
        result = 1;
        tabs = 1;
        prompt = menu_page02_row_frames_b[self->index_0x01A];
        break;
    case 3:
        prompt = 6;
        result = (cur->flag_0x11 == 0) + 1;
        break;
    case 5:
        prompt = 5;
        if (cur->value_0x0C != 0x178) {
            prompt = 4;
        }
        /* fall through */
    case 6:
        result = 3;
        break;
    case 10:
        item_page_draw((MenuSlot*)(self + 1));
        return;
    default:
        return;
    }
    get_lsp_data(0x3E0, &pos);
    menu_frame_draw_blocks((IdValue*)self->slots_0x210, (MenuListEntry*)self->entries_b_0x06C, self->col_0x1A0 + 1,
                           self->row_0x1A1, self->fade_0x23A, flags, &pos);
    menu_sel_pair_draw(self, 2);
    if (result != 0) {
        menu_sel_cursor_result_draw(cur, menu_equip_result_lsp_ids);
    }
    if (prompt >= 0) {
        get_lsp_data(0x2DC, &pos);
        draw_window_frame_preset(pos.x, pos.y, 0);
        font_set_size(0x10, 0x10);
        font_print_ex(pos.x + 0x18, pos.y + 0x10, 0, (s8*)get_menu_msg(0x26, prompt));
        if (note >= 0) {
            font_print_ex(pos.x + 0x18, pos.y + 0x10, 2, (s8*)get_menu_msg(0x26, note));
        }
    }
    if (tabs != 0) {
        get_lsp_data(0x3F0, &pos);
        menu_choice_window_draw((MenuEntry*)self->entries_a_0x024, self->field_0x016, &pos);
    }
}
#pragma optimization_level reset

/* ============================================================================================== */
/* 0x8031BFEC..0x8031CA3C - the cursor's own machine                                               */
/* ============================================================================================== */
extern "C" void menu_sel_cursor_init(MenuSelCursor* cur, u8 kind, MenuSel* owner) {
    cur->item_b_0x06 = 0;
    cur->item_a_0x04 = 0;
    cur->slot_b_0x0A = 0xFFFF;
    cur->slot_a_0x08 = 0xFFFF;
    cur->mode_0x16 = 0;
    cur->record_0x00 = NULL;
    cur->kind_0x0E = kind;
    cur->owner_0x1C = owner;
}

extern "C" void menu_sel_cursor_first_set(MenuSelCursor* cur, u16 a, u16 b) {
    cur->slot_a_0x08 = a;
    cur->item_a_0x04 = b;
}

extern "C" u32 menu_sel_cursor_second_set(MenuSelCursor* cur, u16 a, u16 b) {
    cur->slot_b_0x0A = a;
    cur->item_b_0x06 = b;
    if (a == 0xFFFF) cur->record_0x00 = NULL;
    else cur->record_0x00 = (MenuComboRecipe*)lb_combo_recipe_find(cur->item_a_0x04, b, 0);
    cur->mode_0x16 = 0;
    cur->state_0x18 = 0;
    if (cur->record_0x00 == NULL) {
        cur->slot_b_0x0A = 0xFFFF;
        cur->item_b_0x06 = 0;
        return 0;
    }
    return 1;
}

/* 0x8031C0B8 (0x280): re-resolves the pair's slots and sets the status (ready when negative, else the reason the
 * combination cannot run); returns 1 when it can. */
extern "C" u32 menu_sel_cursor_status_update(MenuSelCursor* cur) {
    MenuSel* owner;
    s32 room;

    if (lb_combo_recipe_known_ck((LbComboRecipe*)cur->record_0x00, 0) == 1) {
        cur->flag_0x11 = 1;
        cur->flag_0x12 = 0;
    } else {
        cur->flag_0x11 = 0;
        cur->flag_0x12 = 1;
    }
    if (cur->slot_a_0x08 == 0xFFFF || cur->slot_b_0x0A == 0xFFFF) {
        cur->status_0x10 = 1;
        return 0;
    }
    cur->status_0x10 = -1;
    if (cur->kind_0x0E <= 1) {
        owner = cur->owner_0x1C;
        if (menu_sel_slot_item_ck(owner, cur->slot_a_0x08, cur->item_a_0x04) == 0) {
            cur->slot_a_0x08 = menu_slot_find((MenuSlot*)owner, cur->item_a_0x04);
            if (cur->slot_a_0x08 == 0xFF) {
                cur->slot_a_0x08 = 0xFFFF;
                cur->status_0x10 = 1;
            }
        }
        if (menu_sel_slot_item_ck(owner, cur->slot_b_0x0A, cur->item_b_0x06) == 0) {
            cur->slot_b_0x0A = menu_slot_find((MenuSlot*)owner, cur->item_b_0x06);
            if (cur->slot_b_0x0A == 0xFF) {
                cur->slot_b_0x0A = 0xFFFF;
                cur->status_0x10 = 1;
            }
        }
    }
    if (cur->status_0x10 < 0) {
        if (cur->flag_0x11 != 0) {
            if (cur->kind_0x0E <= 1) {
                room = menu_item_two_page_store(cur->record_0x00->result_0x02, owner->page_a_0x194, owner->page_b_0x198);
            } else {
                room = item_slots_room_get(cur->record_0x00->result_0x02, lobby_world_block->box_0x0180,
                                           userdata_box_capacity((u8*)lobby_world_block));
            }
            if (cur->kind_0x0E <= 1 && room == 0) {
                cur->status_0x10 = 0;
            } else if (room <= 0) {
                if (cur->kind_0x0E <= 1) {
                    if ((s16)menu_slot_get(owner, cur->slot_a_0x08)[1] > 1 &&
                        (s16)menu_slot_get(owner, cur->slot_b_0x0A)[1] > 1) {
                        cur->status_0x10 = 2;
                    }
                } else if (lobby_world_block->box_0x0180[cur->slot_a_0x08].value > 1 &&
                           lobby_world_block->box_0x0180[cur->slot_b_0x0A].value > 1) {
                    cur->status_0x10 = 3;
                }
            }
        } else if (cur->kind_0x0E == 2 &&
                   (u16)item_slots_free_count(lobby_world_block->box_0x0180,
                                              userdata_box_capacity((u8*)lobby_world_block)) == 0) {
            cur->status_0x10 = 3;
        }
    }
    return cur->status_0x10 < 0;
}

/* 0x8031C338 (0x58): counts a held confirm down; returns 1 the frame it expires. */
extern "C" s32 menu_sel_cursor_hold_tick(MenuSelCursor* cur, u16 bits) {
    if (cur->state_0x18 == 2) {
        if (bits == 0x10) {
            if ((s8)(u8)(cur->timer_0x17 -= 1) <= 0) {
                cur->state_0x18 = 0;
                return 1;
            }
        } else {
            cur->state_0x18 = 0;
        }
    }
    return 0;
}

/* 0x8031C390 (0x78): the key-repeat timer; returns 1 on each repeat of a held button. */
extern "C" s32 menu_sel_cursor_repeat_tick(MenuSelCursor* cur, u16 trig, u16 hold) {
    if (trig & 0x10) {
        cur->state_0x18 = 0;
        cur->timer_0x17 = 0x10;
    } else if (hold & 0x10) {
        if ((s8)(u8)(cur->timer_0x17 -= 1) <= 0) {
            cur->state_0x18 = 1;
            cur->timer_0x17 = 5;
            return 1;
        }
    } else {
        cur->state_0x18 = 0;
    }
    return 0;
}

extern "C" s32 menu_sel_cursor_confirm_step(MenuSelCursor* cur, s32* flag, u16 a, u16 b, u16 c) {
    u32 r = (u32)menu_sel_cursor_hold_tick(cur, a);
    if (r == 1 || (b & 0x30)) {
        if ((r != 0 || (b & 0x10)) && *flag == 0) return 1;
        if ((b & 0x20) || ((b & 0x10) && *flag == 1)) {
            sysSE_req(1);
            return 2;
        }
        return 0;
    }
    if (cur->status_0x10 < 0 && (c & 3)) {
        *flag = menu_cursor_step(*flag, 2, c, 1, 2);
    }
    return 0;
}

/* 0x8031C514 - start the value count-up for the resolved pair */
extern "C" void menu_sel_cursor_combine_start(MenuSelCursor* cur, u8 which) {
    if (which == 0) {
        cur->state_0x18 = 0;
        cur->timer_0x17 = 0x10;
    } else {
        cur->state_0x18 = 1;
        cur->timer_0x17 = 5;
    }
    if (cur->flag_0x11 == 0) {
        cur->timer_0x0F = 0x1E;
        se_combo_mix_req();
    } else {
        cur->timer_0x0F = 1;
    }
    switch ((u32)cur->kind_0x0E) {
    case 0:
    case 1:
        cur->value_0x0C = lb_combo_exec(&cur->anim_0x15, cur->item_a_0x04, cur->item_b_0x06, 0,
                                        cur->slot_a_0x08, cur->slot_b_0x0A);
        break;
    case 2:
        cur->value_0x0C = lb_combo_exec(&cur->anim_0x15, cur->item_a_0x04, cur->item_b_0x06, 2,
                                        cur->slot_a_0x08, cur->slot_b_0x0A);
        break;
    }
    cur->mode_0x16 = 1;
}

/* 0x8031C5FC (0x2F0): counts the combination down; when it runs, marks the recipe seen, posts the result notices and
 * drops the slots the combination emptied.  Returns 1 the frame it finishes. */
extern "C" u32 menu_sel_cursor_combine_step(MenuSelCursor* cur, u16 trig, u16 hold) {
    _mh_ivec2_ pos_a;
    _mh_ivec2_ pos_b;
    MenuSel* owner;

    menu_sel_cursor_repeat_tick(cur, trig, hold);
    if (--cur->timer_0x0F != 0) {
        return 0;
    }
    if (cur->value_0x0C > 0) {
        lb_combo_recipe_learn((LbComboRecipe*)cur->record_0x00);
        cur->applied_0x14 = cur->value_0x0C;
        cur->flag_0x11 = 1;
        se_combo_result_req(1);
        if (cur->flag_0x12 == 0) {
            if (cur->kind_0x0E <= 1) {
                get_lsp_data(0x3D2, &pos_a);
                get_lsp_data(0x3DB, &pos_b);
                if (cur->flag_0x13 & 1) {
                    hud_notice_spawn(0, 2, pos_a.x, pos_a.y, 1, 1, 0);
                    hud_notice_spawn(0, 4, pos_b.x, pos_b.y, 1, 1, 0);
                } else {
                    hud_notice_spawn(0, 3, pos_a.x, pos_a.y, 1, 1, 0);
                    hud_notice_spawn(0, 5, pos_b.x, pos_b.y, 1, 1, 0);
                }
            } else {
                get_lsp_data(0x1B5D, &pos_a);
                get_lsp_data(0x1B5E, &pos_b);
                if (cur->flag_0x13 & 1) {
                    hud_notice_spawn(0, 2, pos_a.x, pos_a.y, 1, 1, 0);
                    hud_notice_spawn(0, 7, pos_b.x, pos_b.y, 1, 1, 0);
                } else {
                    hud_notice_spawn(0, 3, pos_a.x, pos_a.y, 1, 1, 0);
                    hud_notice_spawn(0, 8, pos_b.x, pos_b.y, 1, 1, 0);
                }
            }
            cur->flag_0x13++;
        }
        cur->counter_0x22 = 0;
    } else {
        cur->value_0x0C = 0x178;
        cur->anim_0x15 = 1;
        se_combo_result_req(0);
    }
    if (cur->kind_0x0E <= 1) {
        owner = cur->owner_0x1C;
        if (menu_slot_get(owner, cur->slot_a_0x08)[0] == 0) {
            cur->slot_a_0x08 = 0xFFFF;
        }
        if (menu_slot_get(owner, cur->slot_b_0x0A)[0] == 0) {
            cur->slot_b_0x0A = 0xFFFF;
        }
    } else {
        if (item_slots_count_sum(cur->item_a_0x04, lobby_world_block->box_0x0180,
                                 userdata_box_capacity((u8*)lobby_world_block)) <= 0) {
            cur->slot_a_0x08 = 0xFFFF;
        }
        if (item_slots_count_sum(cur->item_b_0x06, lobby_world_block->box_0x0180,
                                 userdata_box_capacity((u8*)lobby_world_block)) <= 0) {
            cur->slot_b_0x0A = 0xFFFF;
        }
    }
    return 1;
}

/* 0x8031C8EC - begin the value-apply phase */
extern "C" void menu_sel_cursor_apply_start(MenuSelCursor* cur) {
    cur->counter_0x23 = 0;
    if (cur->flag_0x12 != 0) {
        cur->timer_0x0F = 0x1E;
    } else {
        cur->timer_0x0F = 0;
        if (cur->state_0x18 == 1) cur->counter_0x23 = 0x11;
    }
    cur->mode_0x16 = 2;
}

/* 0x8031C934 (0x108): runs the apply phase; returns 1 to repeat the combination, 2 to close it, else 0. */
extern "C" s32 menu_sel_cursor_apply_step(MenuSelCursor* cur, u16 trig, u16 hold) {
    s32 repeat = menu_sel_cursor_repeat_tick(cur, trig, hold);

    if (cur->counter_0x23 < 0x11) {
        cur->counter_0x23++;
    }
    if (cur->timer_0x0F != 0) {
        cur->timer_0x0F--;
        return 0;
    }
    if (cur->flag_0x12 == 0 && cur->state_0x18 != 0 && repeat != 0) {
        menu_sel_cursor_status_update(cur);
        if (cur->status_0x10 < 0) {
            return 1;
        }
        sysSE_req(0);
        return 2;
    }
    if (trig & 0x30) {
        menu_sel_cursor_status_update(cur);
        if (trig & 0x10) {
            cur->state_0x18 = 2;
        }
        sysSE_req(0);
        return 2;
    }
    return 0;
}

/* 0x8031CA3C - the per-frame blend counters */
extern "C" void menu_sel_cursor_blend_tick(MenuSelCursor* cur) {
    if (cur->record_0x00 == NULL) {
        if (cur->blend_0x20 != 0) cur->blend_0x20--;
    } else {
        if (cur->blend_0x20 < 3) cur->blend_0x20++;
    }
    if (cur->mode_0x16 <= 1) {
        if (cur->blend_0x21 != 0) cur->blend_0x21--;
    } else {
        if (cur->blend_0x21 < 3) cur->blend_0x21++;
    }
    if (cur->counter_0x22 < 2) cur->counter_0x22++;
}

/* 0x8031CAC4 (0x17C): draws one item cell at `pos` (icon, name and count) in the colour of the three flags, with the
 * menu cursor when `focused`. */
extern "C" void menu_item_cell_draw(const _mh_ivec2_* pos, u16 item, s16 count, bool active, bool focused,
                                    bool faded) {
    _SPR_DATA_ spr;
    char text[32];
    s32 frame = faded == true ? 0 : 3;
    u16 id;

    if (item != 0) {
        spr_data_copy(&spr, get_lsp_data(0x37E, NULL));
        spr.color = GetMenuIconColor(active, focused, faded, false);
        draw_itemicon_item_id(spr, item, pos);
        if (item != 0) {
            sprite_frame_apply(&spr, 0x383, frame, NULL);
            spr.color = GetMenuFontColor(active, focused, faded, false);
            draw_font(spr, (s8*)ItemName(item), 0, pos);
            sprintf(text, menu_sel_count_fmt, count);
            id = 0x385;
            if (count < 10) id = 0x384;
            sprite_frame_apply(&spr, id, frame, NULL);
            spr.color = GetMenuFontColor(active, focused, faded, false);
            draw_font(spr, (s8*)text, 1, pos);
        }
    }
    if (focused == true) {
        put_menu_cursor((u16*)get_menu_lsp_tbl(0x39), 0, pos);
    }
}

/* 0x8031CC40 (0x1C8): draws the two item cells of the pair, `side` (0 or 1) focused. */
extern "C" void menu_sel_pair_draw(MenuSel* self, u8 side) {
    MenuSelCursor* cur = &self->cursor_0x1EC;
    u8 blend = cur->blend_0x20;
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    s16 count;
    bool focused;

    get_lsp_data(0x358, &base);
    draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x36), blend, &base);
    get_lsp_data(0x379, &pos);
    draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x37), blend, &pos);
    if (cur->item_a_0x04 != 0) {
        count = menu_item_two_page_count(cur->item_a_0x04, self->page_a_0x194, self->page_b_0x198);
    } else {
        count = 0;
    }
    focused = false;
    if (cur->kind_0x0E == 0 && side == 0) {
        focused = true;
    }
    menu_item_cell_draw(&pos, cur->item_a_0x04, count, cur->slot_a_0x08 != 0xFFFF, focused, blend == 0);
    get_lsp_data(0x386, &pos);
    draw_sprite_anim_ary(side >= 1 ? (const u16*)get_menu_lsp_tbl(0x37) : (const u16*)get_menu_lsp_tbl(0x38), blend,
                         &pos);
    if (cur->item_b_0x06 != 0) {
        count = menu_item_two_page_count(cur->item_b_0x06, self->page_a_0x194, self->page_b_0x198);
    } else {
        count = 0;
    }
    focused = false;
    if (cur->kind_0x0E == 0 && side == 1) {
        focused = true;
    }
    menu_item_cell_draw(&pos, cur->item_b_0x06, count, cur->slot_b_0x0A != 0xFFFF, focused, blend == 0);
    draw_sprite_anim_idx(0x374, blend, &base);
}

extern "C" {
/* The "new recipe" mark the result state blinks. */
u16 menu_sel_new_mark_ids[3] = { 0x03C5, 0x03C6, 0xFFFF };
}

/* 0x8031CE08 (0x40C): draws the combination panel: the rate and yield while the pair can combine (else the reason it
 * cannot), the resulting item, and in the result state the count it made. */
extern "C" void menu_sel_cursor_result_draw(MenuSelCursor* cur, const u16* lsp_ids) {
    u16 item = 0;
    u8** strs = get_str_tbl(0x40);
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    _mh_ivec2_ num;
    _SPR_DATA_ spr;
    char text[8];
    s16 count;
    u8 max;
    s32 color;

    get_lsp_data(lsp_ids[0], &base);
    draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x3A), cur->blend_0x21, &base);
    if (cur->status_0x10 < 0) {
        draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x3B), cur->blend_0x21, &base);
        get_lsp_data(lsp_ids[1], &pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x40), &pos);
        sprite_frame_apply(&spr, 0x3B6, cur->blend_0x21, NULL);
        if (cur->flag_0x11 != 0) {
            sprintf(text, (char*)strs[0], lb_combo_rate_get((LbComboRecipe*)cur->record_0x00, cur->kind_0x0E == 2 ? 2 : 0,
                                                            cur->kind_0x0E));
        } else {
            sprintf(text, (char*)strs[1]);
        }
        draw_font(spr, (s8*)text, 1, &pos);
        get_lsp_data(lsp_ids[2], &pos);
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x40), &pos);
        if (cur->flag_0x11 != 0) {
            lb_combo_rate_sprintf(text, (LbComboRecipe*)cur->record_0x00);
        } else {
            sprintf(text, (char*)strs[2]);
        }
        draw_font(spr, (s8*)text, 1, &pos);
    } else {
        draw_sprite_idx(((const u16*)get_menu_lsp_tbl(0x3C))[cur->status_0x10], &base);
    }
    get_lsp_data(lsp_ids[3], &pos);
    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x3D), &pos);
    if (cur->record_0x00 != NULL) {
        item = cur->record_0x00->result_0x02;
    }
    if (cur->mode_0x16 == 2) {
        if (cur->value_0x0C != 0x178) {
            if (cur->flag_0x12 != 0) {
                get_lsp_data(lsp_ids[4], &num);
                draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(0x41), cur->counter_0x23, &num);
                draw_sprite_anim_ary(menu_sel_new_mark_ids, 0x11, &num);
            } else {
                get_lsp_data(lsp_ids[5], &num);
                draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x42), &num);
            }
        } else {
            get_lsp_data(lsp_ids[6], &num);
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x43), &num);
        }
        sprintf(text, menu_sel_count_fmt, cur->anim_0x15);
        draw_font_idx(0x394, (s8*)text, 1, &base);
        item = cur->value_0x0C;
    }
    if (cur->flag_0x11 != 0) {
        draw_itemicon_item_id_idx(0x3B1, item, &pos);
        draw_font_idx(0x3B2, (s8*)ItemName(item), 0, &pos);
    } else {
        draw_font_idx(0x3B2, (s8*)strs[3], 0, &pos);
    }
    get_lsp_data(lsp_ids[7], &num);
    switch ((u32)cur->kind_0x0E) {
    case 0:
    case 1:
        draw_sprite_idx(0x3B9, &num);
        if (cur->flag_0x11 != 0) {
            count = menu_item_two_page_count(item, cur->owner_0x1C->page_a_0x194, cur->owner_0x1C->page_b_0x198);
            draw_number_2digit_anim_ary((const u16*)get_menu_lsp_tbl(0x3E), cur->counter_0x22, count, 0, &num);
            max = GetItemData(item)->max_num_0x003;
            draw_number_2digit_ary((const u16*)get_menu_lsp_tbl(0x3F), max, 0, &num);
        }
        break;
    case 2:
        draw_sprite_idx(0x3BA, &num);
        if (cur->flag_0x11 != 0) {
            get_lsp_data(lsp_ids[7], &num);
            color = cur->counter_0x22 == 1 ? -1 : 0;
            draw_number_digits_ary(menu_sel_box_digit_ids,
                                   item_slots_count_sum(item, lobby_world_block->box_0x0180,
                                                        userdata_box_capacity((u8*)lobby_world_block)),
                                   color, &num);
        }
        break;
    }
    font_flush();
}

/* 0x8031D214 (0x80): draws the confirm window at layout `lsp_id` with row `sel` highlighted, shifted right while the
 * pair cannot combine. */
extern "C" void menu_sel_cursor_confirm_draw(MenuSelCursor* cur, s32 sel, u16 lsp_id) {
    _mh_ivec2_ pos;
    _mh_ivec2_ offset;

    get_lsp_data(lsp_id, &pos);
    offset.x = cur->status_0x10 < 0 ? 0 : 10;
    offset.y = 0;
    menu_select_window_draw(sel, 2, (s8*)get_menu_msg_tbl(0x27), &pos, &offset);
}

/* ============================================================================================== */
/* 0x8031D294..0x8031DAA8 - eft043, the player's hand glow (effect pool +0x38 `Eft043Work`)       */
/* ============================================================================================== */

/* The eft043 effect pool.  size: 0x14 */
struct Eft043Work {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effects[2];
    /* +0x0C */ _GXColor color;
    /* +0x10 */ f32 scale;
};

/* The joint holder `_PLW::physics_0x13C` points at: the player's `MHchar` at +0x04 (a view of
 * `ef/fn_80114E34_types.h`'s `_PLW_PHYSICS`, whose header redefines `_EFT`).  size: 0x144 - lower bound */
struct Eft043PlwJoints {
    /* +0x000 */ u8 unused_0x000[0x04];
    /* +0x004 */ MHchar joint_0x004;
};

/* A vector built through `setVec3`, so a file-scope one is constructed by `__sinit`.  size: 0xC */
struct Eft043Offset : public nw4r::math::VEC3 {
    /* Builds (x, y, z). */
    Eft043Offset(f32 x, f32 y, f32 z) { setVec3(this, x, y, z); }
};

extern "C" {
/* The effect ids type 0 creates, and the pool size of each type. */
u16 eft043_effect_ids[2] = { 0x019F, 0x01A0 };
u8 eft043_effect_counts[2] = { 2, 1 };
/* The colour key strip and the scale key table type 0 runs. */
u8 eft043_color_keys[16] = { 0x00, 0x64, 0x64, 0x64, 0x19, 0x1E, 0x32, 0x96, 0xFF, 0x1E, 0x32, 0x96, 0, 0, 0, 0 };
f32 eft043_scale_keys[20] = {
    0.0f, 0.0f, 0.0f, 0.0f,
    2.0f, 3.2f, 0.3f, 1.5f,
    6.0f, 1.2f, 1.2f, 1.5f,
    25.0f, 1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f, 1.0f,
};
}

/* The joint-7 offset of each type; MWCC's `__sinit` builds them. */
Eft043Offset eft043_offsets[2] = {
    Eft043Offset(-10.0f, -17.0f, 87.0f),
    Eft043Offset(-10.0f, -17.0f, 45.0f),
};

extern "C" void eft043_release(_EFT* self);
extern "C" void eft043_dispatch(_EFT* self);
extern "C" void eft043_start(_EFT* self);
extern "C" void eft043_move(_EFT* self);
extern "C" void eft043_end(_EFT* self);
extern "C" void eft043_retire(_EFT* self);

/* 0x8031D294 (0xA8): spawns the glow effect of `type` on the player `plw`. */
extern "C" void eft043_set(_PLW* plw, u8 type) {
    _EFT* self = (_EFT*)eft_res_slot_get(0x14);

    if (self != NULL) {
        ((Eft043Work*)self->work_0x38)->count = eft043_effect_counts[type];
        self->field_0x03 = 0x2B;
        self->area_0x44 = plw->area_0x16;
        self->type_0x02 = type;
        self->source_0x30 = plw;
        self->timer_0x0C = 0;
        eft_state_flags_set(self, 1, 0);
        self->release_0x40 = eft043_release;
        self->dispatch_0x34 = eft043_dispatch;
    }
}

/* 0x8031D33C (0x3C): returns the pool's effects to the heap. */
extern "C" void eft043_release(_EFT* self) {
    Eft043Work* work = (Eft043Work*)self->work_0x38;

    push_eft_effect_heap_num(work->effects, work->count);
    work->count = 0;
}

/* 0x8031D378 (0x3C): runs the effect's state. */
extern "C" void eft043_dispatch(_EFT* self) {
    switch (self->state_0x05) {
    case 0:
        eft043_start(self);
        return;
    case 1:
        eft043_move(self);
        return;
    case 2:
        eft043_end(self);
        return;
    case 3:
        eft043_retire(self);
        return;
    }
}

/* 0x8031D3B4 (0x280): creates the pool's effects, places type 0 on the player's joint 7 and steps once. */
extern "C" void eft043_start(_EFT* self) {
    Eft043Work* work = (Eft043Work*)self->work_0x38;
    _PLW* plw = (_PLW*)self->source_0x30;
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 dir;
    s32 i;

    MTX34_ctor(&mtx);
    VEC3_ctor(&ofs);
    VEC3_ctor(&dir);
    switch (self->type_0x02) {
    case 0:
        for (i = 0; i < work->count; i++) {
            work->effects[i] = res_eft_create(eft043_effect_ids[i], 21, 0);
            if (work->effects[i] == NULL) {
                eft043_retire(self);
                return;
            }
        }
        if (eft_res_spawn_gate_ck(self, 0) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05 = 3;
            return;
        }
        mhchar_joint_mtx_get(&((Eft043PlwJoints*)plw->physics_0x13C)->joint_0x004, 7, &mtx);
        mtx34_trans_get(&mtx, &self->pos_0x18);
        dir.x = -mtx.m[0][2];
        dir.y = -mtx.m[1][2];
        dir.z = -mtx.m[2][2];
        calcVecAngXY(&dir, &self->rot_0x24.x, &self->rot_0x24.y);
        self->rot_0x24.z = 0;
        copyVec3(&ofs, &eft043_offsets[self->type_0x02]);
        rotVecX(&ofs, self->rot_0x24.x);
        rotVecY(&ofs, self->rot_0x24.y);
        addVec3To(&self->pos_0x18, &ofs);
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        mtx34_set_trans(&mtx, &self->pos_0x18);
        if (self->area_0x44 == (u8)get_now_areano()) {
            if (plw->kind_0x09 != 3) {
                pl_ryugeki_se_req(plw, &self->pos_0x18, 0);
            } else {
                pl_ryugeki_se_req(plw, &self->pos_0x18, 1);
            }
        }
        work->color.r = eft043_color_keys[1];
        work->color.g = eft043_color_keys[2];
        work->color.b = eft043_color_keys[3];
        work->color.a = 0xFF;
        break;
    case 1:
        for (i = 0; i < work->count; i++) {
            work->effects[i] = res_eft_create(0x1CE, 21, 0);
            if (work->effects[i] == NULL) {
                eft043_retire(self);
                return;
            }
        }
        break;
    }
    self->state_0x05++;
    self->field_0x06 = 0;
    work->scale = 1.0f;
    self->flag_0x01 = 1;
    eft043_move(self);
}

/* 0x8031D634 (0x35C): follows the player's joint 7, runs the colour and scale keys, and ends the effect when the
 * player leaves the glowing act. */
extern "C" void eft043_move(_EFT* self) {
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 scale;
    _PLW* plw;
    Eft043Work* work;
    nw4r::ef::EffectSystem* system;
    s32 i;

    MTX34_ctor(&mtx);
    VEC3_ctor(&ofs);
    VEC3_ctor(&dir);
    VEC3_ctor(&scale);
    plw = (_PLW*)self->source_0x30;
    work = (Eft043Work*)self->work_0x38;
    system = eft_control.effect_system_0x04;
    if (eft_res_spawn_gate_ck(self, 0) == 0) {
        self->timer_0x0C = 0;
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if (self->type_0x02 == 0) {
        if (pl_act4_phase_ck(plw, 0) == 0) {
            if (pl_act4_phase_ck(plw, 1) == 1) {
                self->timer_0x0C = 6;
            } else {
                self->timer_0x0C = 0;
            }
            self->state_0x05++;
            RetireParticleAll(work->effects[1]);
            work->effects[1]->RetireEmitterAll();
            if (system != NULL) {
                system->RetireEffect(work->effects[1]);
                work->count--;
            }
            eft043_end(self);
            return;
        }
    } else if (pl_act4_phase_ck(plw, 0) == 0) {
        self->state_0x05++;
        self->flag_0x01 = 0;
        return;
    }
    mhchar_joint_mtx_get(&((Eft043PlwJoints*)plw->physics_0x13C)->joint_0x004, 7, &mtx);
    mtx34_trans_get(&mtx, &self->pos_0x18);
    dir.x = -mtx.m[0][2];
    dir.y = -mtx.m[1][2];
    dir.z = -mtx.m[2][2];
    calcVecAngXY(&dir, &self->rot_0x24.x, &self->rot_0x24.y);
    self->rot_0x24.z = 0;
    copyVec3(&ofs, &eft043_offsets[self->type_0x02]);
    rotVecX(&ofs, self->rot_0x24.x);
    rotVecY(&ofs, self->rot_0x24.y);
    addVec3To(&self->pos_0x18, &ofs);
    cpSetRotMatrix(&self->rot_0x24, &mtx);
    mtx34_set_trans(&mtx, &self->pos_0x18);
    switch (self->type_0x02) {
    case 0:
        self->timer_0x0C++;
        eftGetKeyRGB(eft043_color_keys, self->timer_0x0C, &work->color.r, &work->color.g, &work->color.b);
        getKeyData3(eft043_scale_keys, (f32)self->timer_0x0C, &scale.x, &scale.y, &scale.z);
        for (i = 0; i < work->count; i++) {
            work->effects[i]->SetRootMtx(mtx);
            change_paramscale_eff_vec3(work->effects[0], &scale);
            effect_move(work->effects[i]);
        }
        change_color_eff(work->effects[0], &self->pos_0x18, work->color);
        break;
    case 1:
        work->effects[0]->SetRootMtx(mtx);
        change_paramscale_eff(work->effects[0], 0.8f);
        if (effect_move(work->effects[0]) == 0) {
            self->state_0x05++;
            self->flag_0x01 = 0;
            return;
        }
        break;
    }
    if (Pl_master_ck(plw) == 1) {
        eft_res_models_spawn(self, (void**)work->effects, eft_stage_spawn_flag(plw) | 1, work->count, NULL);
    } else {
        eft_res_models_spawn(self, (void**)work->effects, 1, work->count, NULL);
    }
}

/* 0x8031D990 (0xC0): fades type 0 out over its timer, then lets the record retire. */
extern "C" void eft043_end(_EFT* self) {
    Eft043Work* work = (Eft043Work*)self->work_0x38;

    switch (self->type_0x02) {
    case 0:
        if (--self->timer_0x0C >= 0) {
            effect_move(work->effects[0]);
            change_color_eff(work->effects[0], &self->pos_0x18, work->color);
            eft_res_models_spawn(self, (void**)work->effects, 1, 1, NULL);
        } else {
            self->flag_0x01 = 0;
            self->state_0x05++;
        }
        break;
    case 1:
        self->state_0x05++;
        break;
    }
}

/* 0x8031DA50 (0x4): retires the record. */
extern "C" void eft043_retire(_EFT* self) {
    eft_res_slot_release(self);
}

