/* hud/cockpit_quest - the in-game cockpit HUD: the per-player cockpit work records and their frame update, the
 *   quest gauges, the quest marker ring with its target/player/pit-trap markers, the party plates, the weapon HUD
 *   (aim view, ammo, stagger and charge gauges), the item ring/list/tag, the hunt log, the supply box and the quest
 *   menus, in the single view and the two-player split view.
 * RANGE. .text 0x802E4978-0x802F2238 (158 functions, all written); extab 0x80014FFC-0x8001547C, extabindex
 *   0x80033660-0x80033D20, .ctors 0x8056F38C-0x8056F390, .data 0x805D5B48-0x805D69A8, .bss 0x806BDCC8-0x806BE078,
 *   .sdata 0x807927C0-0x80792860, .sdata2 0x8079A8E0-0x8079A9C8 (the unit's block in config/RMHE08/splits.txt).
 * FLAGS. `cflags_hud` (cflags_main + `-opt nopeephole`) plus this object's `-pool off` (configure.py); `#pragma
 *   fp_contract off` around the functions that keep retail's unfused `fmuls`+`fadds`.
 * VIEWS. `cockpit_npc_entries_update`, `cockpit_notice_hold`, `cockpit_event_notice_spawn`,
 *   `cockpit_hud_hidden_self_ck`, `cockpit_hud_hidden_ck`, `cockpit_party_elements_draw`, `cockpit_take_menu_update`,
 *   `cockpit_supply_cell_pos_get`, `cockpit_supply_notice_spawn`, `cockpit_free_play_ck` and `quest_bar_id_keep`
 *   still compile in the namespace `view_fn_802E4978` against `menu/fn_802E4978.h`'s older record view (`CockpitWork`
 *   with `CockpitMove`/`CockpitText`, whose layout drifts by 4 bytes past +0x180); everything else uses
 *   `hud/cockpit_quest.h` and `hud/cockpit_work_types.h`.  Folding that view removes the namespace.
 * NAMES. GUESS (each read off its body and its callers): every `cockpit_*` and `quest_*` function of the unit,
 *   `pit_trap_construct`, the record types `CockpitItemTag`, `HuntLogEntry`, `CockpitSlotPair`, `CockpitWeaponIds`,
 *   `CockpitStaggerIds` and their fields, the `.data`/`.sdata` table names (`cockpit_act_mask_*`, `cockpit_split_*`,
 *   `cockpit_timer_ring_*`, ...), the inline helpers `item_count_label_id`, `quest_notice_flag_get` and
 *   `quest_mark_notice_flag_get`, the `QuestWork` supply/take fields, `SysQuestMove::area_0x0EB/pos_0x128/
 *   npc_plate_0x0EF` and the `_PLW` fields `ammo_capacity_0x26A`, `ammo_icon_kind_0x26D`, `magazine_0x270`,
 *   `daze_timer_0x468`, `quest_menu_open_0x5BD`, `take_menu_open_0x5BE`, `cockpit_menu_row_0x5BF`,
 *   `cockpit_menu_0x5C0`, and `_AINPC_W::digit_state_0x3C4`.  The registry names this unit proposed for
 *   foreign rows are GUESS too (the `pl_item_*`, `pl_*_ck`, `ai_npc_*_ck`, `draw_*`, `cockpit_*_ids` rows in
 *   config/RMHE08/name_registry.jsonl under lane hud-cockpit-quest).
 * RESIDUALS. register colouring or scheduling only (same instructions): `quest_marker_ring_draw`,
 *   `quest_marker_draw_at`, `quest_marker_icon_draw`, `quest_marker_draw_with_master`,
 *   `quest_marker_draw_all_non_master`, `quest_player_marker_draw`, `quest_target_marker_draw`, `quest_mark_draw`,
 *   `cockpit_icon_cursors_update`, `cockpit_party_list_draw` (the loop's `max`/`plw` pair),
 *   `cockpit_ammo_gauge_draw`/`cockpit_split_ammo_gauge_draw` (`icons` against `step`), `cockpit_supply_box_draw`,
 *   `cockpit_item_tag_update` (the second search loop's counters), `cockpit_split_item_ring_draw`,
 *   `cockpit_hunt_log_draw`, `cockpit_prompt_draw`, `quest_gauge_bar_draw`, `quest_marker_pie_draw`,
 *   `quest_icon_rect_draw`, `quest_map_icon_sprites_draw`, `cockpit_cursor_step`.
 *   `quest_gauge_draw`, `cockpit_split_gauge_draw`: the `health_max / 10` dividend is loaded into r4 and the magic
 *   constant into r0 in retail, the other way round in ours.
 *   `cockpit_name_tags_draw`: the first `Pl_area_flag_get` result lives in r26 in retail, r25 in ours.
 *   `quest_marker_draw`, `quest_marker_draw_sub`: the window record's colour word is loaded before the two `fmr`s
 *   where retail loads it after.
 *   `quest_marker_draw_faded`: retail rounds the ring radius with `frsp f2,f31` at the pie call (an `f64` local
 *   there; `f64 scale` loads its constants as doubles and measures worse).
 *   `cockpit_take_menu_draw`: the `li r3,0xE7B` of the count-label id is scheduled before the compare.
 *   `cockpit_item_list_entry_draw`, `quest_gauge_update`, `quest_targets_update_a`/`_b`: the `li` of the
 *   `x = A; if (c) x = B;` default is scheduled before the compare (or kept in a callee-saved register) where
 *   retail emits it between the compare and the branch.
 *   `cockpit_npc_digit_draw`: the z reload of the projected point is scheduled earlier than retail.
 *   `quest_bar_a_next_id`/`quest_bar_b_next_id`: they reference compiler-local panic strings where the target
 *   references `quest_source_file_name`/`quest_assert_fail_msg`; `quest_bar_b_next_id` keeps a `clrlwi` on the
 *   return (a `u16 id` parameter costs more).
 *   Data: `.data`/`.sdata` objects are defined where their first user is, not yet in the target's order; the
 *   `.sdata2` pool is compiler-made; `.sdata` is 8 bytes longer than the target's.
 *   `cockpit_static_init`..`cockpit_item_tag_construct` are plain functions: the target's are MWCC's `__sinit` and
 *   the record constructors (its `.ctors` word 0x8056F38C), which need `CockpitWork`/`CockpitState`/`PitTrap`/
 *   `CockpitItemTag` constructors.
 * SHAPES. Locals are declared in the reverse of their target stack offsets; a value passed by value to a callee
 *   (`quest_marker_draw(_faded)`'s window spot, `cockpit_aim_marker_draw`'s `VEC3`) is a by-value parameter, not a
 *   pointer to a copy; `bool` locals for `GetMenuFontColor` arguments; `switch` where retail compares a byte with
 *   `cmpwi`; `id = A; if (c) id = B;` where retail branches over a second `li` (a ternary turns branchless), and
 *   inside a `static inline` helper where retail keeps the default out of a callee-saved register;
 *   `(u8)(x + 166) <= 9` range checks; `for (;;) { if (*p == 0) break; ... }` for the 0-terminated mask walks;
 *   `(s32)(u8)b == 3` where retail compares a byte with `cmpwi`; `x.f += a;` where retail stores an `s16` sum
 *   without the `extsh` a plain `x.f = x.f + a` keeps; `u8` field stores of an `s8` parameter for retail's `clrlwi`;
 *   a single `res` exit for `quest_mark_fade_value`; the split gauge's sparkle anchors land in `flash_pos`.
 */

#include "types.h"
#include "pl.h"
#include "nw4r/math.h"
#include "Pl/pl_skill.h"
#include "Pl/pl_act.h"
#include "Pl/Pl_master_ck.h"
#include "hud/layout.h"
#include "hud/cockpit_quest.h"
#include "main.h"
#include "Network/network_pat_control.h"
#include "menu/menu_item.h"
#include "quest/quest_item_slot.h"   /* quest_select_ready_ck (rule 2) */
#include "menu/get_pop_dat_ptr.h"
#include "unsplit/unknown.h"
#include "ai/fn_802D44F4.h"
#include "ef/fn_800CDB2C.h"
#include "lobby/lb_quest_screen.h"
#include "enemy/fn_8012EC74.h"
#include "mh3_pad.h"
#include "fn_8004CAD8.h"
#include "enemy/ENEMY_WORK.h"
#include "copyVec2.h"
#include "hud/pl_gauge_level_get.h"
#include "hud/pl_mode8_alive_ck.h"
#include "ef/eft053_shell_pos_project.h"
#include "camera/camera_main.h"
#include "ef/sys_quest_move.h"
#include "camera/fn_802B5C58.h"
#include "stage/stg_w.h"
#include "Pl/fn_80288CEC.h"
#include "Pl/fn_8027D684.h"
#include "Pl/fn_80273B14.h"
#include "ai/ainpc.h"
#include "unsplit/menu.h"
#include "hud/cockpit.h"
#include "sound/fn_800D7F54.h"
#include "menu/cockpit_hud_hidden_ck.h"
#include "hud/cockpit_icon_data.h"
#include "menu/menu_message.h"
#include "sound/set_zmode__FbUcb.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "enemy/fn_80382310.h"
#include "unsplit/Pl.h"
#include "enemy/em_pop.h"
#include "Pl/pl_motion.h" /* quest_move_sub_flag_get (the owner header, rule 2) */
#include "fn_80047398.h"

extern "C" {

#include "MSL_C/alloc.h" /* sprintf (owner: MSL_C/alloc.cpp, rule 2) */
}

#include "Pl/pl_item_slot_item_get.h"
#include "Pl/pl_act_kind4_item_ids.h"
#include "ai/ai_npc_wait_ck.h"
#include "hud/draw_itemicon_frame_idx.h"
#include "tex_uv_copy.h"
#include "drawshape_set_corner_colors.h"
#include "screen_split_view_get.h"
#include "sound/pl_charge_se_play.h"
#include "enemy/em_prog_flag_ck.h"
#include "menu/item_kind_color_tbl.h"
#include "camera/camera_mode1_ck_player.h"
#include "enemy/em_sight_player_ck2.h"
#include "lobby/cockpit_item_ring_frame_ids.h"
#include "Pl/pl_item_page2_ck.h"
#include "Pl/pl_act_step.h"
#include "Pl/pl_target_lock_ck.h"
#include "Pl/pl_camera_reset_ck.h"
#include "camera/camera.h"
#include "hud/hud_screen_width_get.h"
#include "hud/chara_icon_pos_get.h"
#include "g3d/scn_root_get_current_camera.h"
#include "g3d/camera_world_to_screen.h"
#include "ef/pRoot.h"
#include "Pl/pl_eye_mtx_get.h"
#include "enemy/em_pl_target_ck.h"
#include "g3d/g3d_scnroot.h"
#include "hud/cockpit_quest_init.h"
#include "hud/cockpit_split_init.h"
#include "ai/ainpc_work_get.h"
#include "hud/cockpit_notice_update.h"
#include "pad_hold_ck.h"
#include "ef/math_sin_rad.h"
#include "Runtime.PPCEABI.H/CPlusLibPPC.h"
#include "quest/quest_types.h"
#include "hud/draw_window_box.h"
#include "hud/put_page_count.h"
#include "lobby/lb_server_sel_trans.h"
#include "lobby/lb_quest_work_active_ck.h"
#include "font/flfnt.h"
#include "mh3_pad/system_w.h"

/* nw4r's debug panic - the map's mangling is `Panic__Q24nw4r2dbFPCciPCce` (rule 9: the owner is
 * `nw4r::db`, so the declaration is the real one and the front-end reproduces the map name). */
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace db

/* The quest bar's tables (0x805D5C74.., this unit's own `.data`).  The step table is two rows of blend-id offsets (row 0
 * for kinds 0/1/2 of `quest_bar_a_next_id`/`quest_bar_b_next_id`, row 1 for kinds 3/4); the colour tables are read as
 * `HudBlend` (three colours plus the copied tag word), so a three-word table is followed in memory by the next
 * table's first word, which is what the tag copy picks up. */
/* The quest timer ring (0x805D5B98..): the four quads of the minute ring, the texture cells of its nine fill
 * stages, and the stage each five-minute step of the limit (0..55 minutes) draws. */
_mh_ivec2_ cockpit_timer_ring_verts[16] = {
    { 50, 49 }, { 33, 49 }, { 33, 32 }, { 50, 32 },
    { 50, 49 }, { 50, 66 }, { 33, 66 }, { 33, 49 },
    { 50, 49 }, { 67, 49 }, { 67, 66 }, { 50, 66 },
    { 50, 49 }, { 50, 32 }, { 67, 32 }, { 67, 49 },
};

_mh_tex_uv_ cockpit_timer_ring_uvs[36] = {
    { 340, 160 }, { 340, 143 }, { 357, 143 }, { 357, 160 },
    { 340, 160 }, { 340, 143 }, { 357, 143 }, { 357, 160 },
    { 340, 160 }, { 340, 143 }, { 357, 143 }, { 357, 160 },
    { 340, 160 }, { 340, 143 }, { 357, 143 }, { 357, 160 },
    { 376, 160 }, { 376, 143 }, { 393, 143 }, { 393, 160 },
    { 340, 160 }, { 340, 143 }, { 357, 143 }, { 357, 160 },
    { 340, 160 }, { 340, 143 }, { 357, 143 }, { 357, 160 },
    { 340, 160 }, { 340, 143 }, { 357, 143 }, { 357, 160 },
    { 358, 160 }, { 358, 143 }, { 375, 143 }, { 375, 160 },
};

u8 cockpit_timer_ring_cells[12] = {
    0, 20, 4, 4, 24, 8, 8, 28, 12, 12, 32, 16,
};

u16 quest_bar_id_step_table[2][5] = {
    { 0x0000, 0x0100, 0x01C0, 0x02C0, 0x0300 },
    { 0x0100, 0x01C0, 0x02C0, 0x0380, 0x03C0 },
};

u32 quest_bar_a_blend_from[4] = {
    0x13A013FF, 0x57CC33FF, 0x96E55CFF, 0xC11836FF,
};

u32 quest_bar_a_blend_to[3] = {
    0xC759BBFF, 0xCC5DEEFF, 0xCC7CFBFF,
};

u32 quest_bar_b_blend_base_from[3] = {
    0xE6BA00FF, 0xF2D200FF, 0xF7EA4CFF,
};

u32 quest_bar_b_blend_low_from[3] = {
    0xE63800FF, 0xE63800FF, 0xE63800FF,
};

u32 quest_bar_b_blend_low_to[3] = {
    0xE67373FF, 0xE67373FF, 0xE67373FF,
};

u32 quest_bar_b_blend_normal_to[4] = {
    0xCD7330FF, 0xDF8235FF, 0xE6AD6FFF, 0x00000000,
};

u32 quest_bar_b_blend_flash[6] = {
    0xF9F9C7FF, 0xFCFCE3FF, 0xFFFFFFFF, 0xF5F473FF,
    0xF7FF76FF, 0xF7FFC4FF,
};

u32 quest_bar_b_blend_dm_to[3] = {
    0x0AD7D2FF, 0x1EE1F0FF, 0x88E5F8FF,
};

u32 quest_bar_b_blend_dm_c0_to[3] = {
    0x8CD2E5FF, 0x9BD4F5FF, 0xBEE6FAFF,
};

namespace view_fn_802E4978 {

namespace nw4r = ::nw4r;  /* the view headers below reopen it */

#include "menu/fn_802E4978.h"
#include "unsplit/lobby.h"

extern "C" {
/* This view's spelling of the two player records and the sub-screen state (defined below the namespace). */
extern CockpitWork cockpit_work[2];
extern CockpitState cockpit_state;
}

extern "C" {
extern f32 lbl_8079A8FC;
extern f32 lbl_8079A90C;
extern f32 lbl_8079A918;
extern f32 lbl_8079A91C;
extern f32 lbl_8079A920;
f32 math_sin_rad(f32);
void cockpit_quest_init(void);
void cockpit_split_init(void);
void cockpit_frame_tick(void);
void cockpit_npc_digit_update(CockpitWork*);
void cockpit_frame_update(void);
void cockpit_npc_entries_update(void);
void cockpit_notice_hold(void);
void cockpit_event_notice_spawn(u8);
u32 cockpit_hud_hidden_self_ck(CockpitMove*);
u32 cockpit_hud_hidden_ck(CockpitMove*, u8);
void cockpit_draw(void);
void cockpit_party_elements_draw(void);
void cockpit_supply_box_update(void);
void cockpit_take_menu_update(void);
void cockpit_supply_cell_pos_get(_mh_ivec2_*, s8);
void cockpit_supply_notice_spawn(s8);
void cockpit_supply_box_draw(void);
void cockpit_take_menu_draw(void);
void cockpit_quest_menu_draw(void);
void cockpit_flash_gauge_draw(void);
void cockpit_timer_draw(void);
u32 cockpit_free_play_ck(void);
s32 quest_bar_id_keep(s32, s32);
void cockpit_item_tag_update(CockpitMove*, CockpitText*);
}

void cockpit_npc_entries_update(void) {
    screen_split_mode_ck();
    ainpc_entry_update_all();
    subTransSetPrio(6, (u32)&ainpc_entry_draw_all, 0, NULL);
}

void cockpit_notice_hold(void) {
    cockpit_work[0].field_0x0CF = 1;
    ai_slots_clear();
}

void cockpit_event_notice_spawn(u8 arg0) {
    _mh_ivec2_ pos;
    u16 lsp;
    s32 idx;

    switch (arg0) {
    case 4:
        lsp = 0xEC8;
        idx = 0;
        break;
    case 6:
        lsp = 0xEE5;
        idx = 1;
        break;
    case 7:
        lsp = 0xEED;
        idx = 2;
        break;
    default:
        return;
    }
    ::get_lsp_data(lsp, (::_mh_ivec2_*)&pos);
    hud_notice_spawn(2, idx, pos.x, pos.y, 1, 1, 0);
}

u32 cockpit_hud_hidden_self_ck(CockpitMove* move) {
    u32 r;

    r = get_cfg((u8)move->field_0x008, 0xD) == 0;
    if (move->field_0x308 == 1) {
        r = 1;
    }
    return r;
}

u32 cockpit_hud_hidden_ck(CockpitMove* move, u8 id) {
    u32 r;

    if (id == 0xFF) {
        return cockpit_hud_hidden_self_ck(move);
    }
    r = get_arena_cfg(id, 0xD) == 0;
    if (move->field_0x308 == 1) {
        r = 1;
    }
    return r;
}

void cockpit_party_elements_draw(void) {
    CockpitMove* move;
    u16 max;
    s32 i;

    if (game_ready_ck() != 0) {
        move = (CockpitMove*)::get_move_work_adrs(2);
        max = (u16)::get_move_work_max(2);
        for (i = 0; i < (s32)max; i++, move++) {
            if (move->field_0x000 != 0 && Pl_master_ck((struct _PLW*)move) == 0) {
                draw_lsp_element(move, 0xFF, 0xFF, 0xFFFF);
            }
        }
    }
}

void cockpit_take_menu_update(void) {
    CockpitWork* self;
    CockpitMove* move;

    self = &cockpit_work[0];
    move = self->move;
    if (self->field_0x17E == 1) {
        fn_8027B918((struct _PLW*)move);
    }
    if (move->field_0x5BD == 0xFF) {
        if (menu_item_frame_update((MenuFrameWork*)move) == 0) {
            self->field_0x17E = 1;
            return;
        }
        self->field_0x17E = 0;
        return;
    }
    self->field_0x17E = 0;
}

void cockpit_supply_cell_pos_get(_mh_ivec2_* pos, s8 idx) {
    _mh_ivec2_* src;

    ::get_lsp_data(0xE15, (::_mh_ivec2_*)pos);
    src = (_mh_ivec2_*)::get_lsp_data(cockpit_slot_row_sprite_ids[idx >> 3], NULL);
    pos->x += src->x;
    pos->y += src->y;
    src = (_mh_ivec2_*)::get_lsp_data(cockpit_slot_column_sprite_ids[idx & 7], NULL);
    pos->x += src->x;
    pos->y += src->y;
}

void cockpit_supply_notice_spawn(s8 idx) {
    _mh_ivec2_ pos;
    CockpitWork* self;

    self = &cockpit_work[0];
    cockpit_supply_cell_pos_get(&pos, idx);
    HudNotice* notice = hud_notice_spawn(0, 6, pos.x, pos.y, 1, 1, 0);
    if (notice != 0) {
        hud_notice_set_flag_ptr(notice, &self->field_0x17A);
    }
}

u32 cockpit_hud_hidden_self_ck(CockpitMove* move);



u32 cockpit_free_play_ck(void) {
    return move_work_state_ck();
}

s32 quest_bar_id_keep(s32 arg0, s32 arg1) {
    s32 r;

    r = Pl_dm_condition_ck((struct _PLW*)arg1, 0x100000);
    return (r == 1) ? (u16)(arg0 + 0x3C0) : 0;
}




/* `Screen_w` and its `ScreenGeomView` view come from `unsplit/menu.h` (rule 1: this unit
 * was the first user, `quest/quest_entry.cpp` the second). */

struct SprDataView {
    /* +0x000 */ u8 unused_0x000[0x1C];
    /* +0x01C */ u32 colour_0x01C;
}; /* size: 0x20 */

}  /* namespace view_fn_802E4978 */

/* The two player records and the sub-screen state the cockpit drives (this unit's .bss, 0x806BDCC8-0x806BE078). */
CockpitWork cockpit_work[2];
CockpitState cockpit_state;

/* 0x802E4978 (0x15C): Clears one player's cockpit work record and arms its timers, cursors and gauge state for
 * the player work `plw`. */
void cockpit_work_init(CockpitWork* work, _PLW* plw) {
    s32 r;
    s32 i;

    memset(work, 0, sizeof(CockpitWork));
    work->plw = plw;
    work->ai_npc_0x048 = (struct _AINPC_W*)ainpc_work_get();
    work->slot_timer_0x0C0[3] = -1;
    work->slot_timer_0x0C0[2] = -1;
    work->slot_timer_0x0C0[1] = -1;
    work->slot_timer_0x0C0[0] = -1;
    work->view_timer_0x0BE = -1;
    work->mark_gate_0x0CE = 0;
    work->notice_flag_0x0CF = 0;
    work->stagger_shown_0x134 = 0;
    work->stagger_flash_0x136 = 0;
    work->stagger_notice_clock_0x138 = 0;
    work->stagger_shake_idx_0x137 = 0;
    work->health_hit_clock_0x13A = 0;
    work->health_prev_0x13C = 0;
    work->notice_flag_0x139 = 0;
    work->item_tag_0x02C.kind_prev_0x14 = 0;
    work->item_tag_0x02C.frame_0x0C = 0;
    cockpit_item_ring_reset(plw, &work->item_ring_anim_0x16B[0], &work->item_list_anim_0x16F[0]);
    r = cockpit_gauge_low_ck(plw);
    work->gauge_level_0x16A = (s8)(((s32)(-r | r) >> 31) & 5);
    work->gauge_value_0x168 = plw->field_0x36C;
    work->item_icon_0x164 = 0xFF;
    work->item_frame_0x166 = 0;
    work->gauge_frame_0x167 = 0;
    work->supply_busy_0x176 = 0;
    work->box_frame_0x17C = 0;
    work->cursor_frame_0x17B = 0;
    work->pointer_time_0x178 = 0;
    work->take_busy_0x17D = 0;
    work->item_menu_busy_0x17E = 0;
    work->act_cursor_0x018[0].timer = -1;
    work->skill_cursor_0x020[0].timer = -1;
    work->act_cursor_0x018[1].timer = -1;
    work->skill_cursor_0x020[1].timer = -1;
    work->act_cursor_0x018[2].timer = -1;
    work->skill_cursor_0x020[2].timer = -1;
    work->act_cursor_0x018[3].timer = -1;
    work->skill_cursor_0x020[3].timer = -1;
    work->npc_cursor_0x028[0].timer = -1;
    work->npc_cursor_0x028[1].timer = -1;
    work->flash_frame_0x180 = 0;
    work->flash_hold_0x17F = 0;
    work->sparkle_a_frame_0x18C = 0;
    work->sparkle_b_frame_0x18E = 0;
    work->blink_clock_0x184 = 0;
    work->quest_timer_0x188 = 0;
    for (i = 0; i < 10; i++) {
        setCockpitTransferMode((u8)i, 0);
    }
}

/* 0x802E4AD4 (0xB8): Sets the cockpit up for the one-player view: the state's counters, the hunt total and this
 * player's work record. */
void cockpit_quest_init(void) {
    _PLW* plw;
    CockpitState* st;
    s8 idx;

    plw = (_PLW*)get_move_work_adrs(2);
    st = &cockpit_state;
    st->field_0x000 = 1;
    st->pit_trap_shown_0x059 = 0;
    st->pit_trap_count_0x058 = 0;
    if (move_work_state_ck() != 0) {
        st->hunt_log_timer_0x05C = 0;
        st->hunt_log_count_0x05A = 0;
        st->hunt_total_0x070 = em_set_work_state_get();
    } else {
        st->hunt_log_timer_0x05C = 0;
        st->hunt_log_count_0x05A = 0;
        st->hunt_total_0x070 = quest_work_state_get();
    }
    st->blink_timer_0x074 = 0;
    idx = (s8)my_player_no();
    plw = &plw[idx];
    cockpit_work_init(&cockpit_work[0], plw);
    cockpit_notice_init();
    ai_slots_clear();
}

/* 0x802E4B8C (0x98): Sets the cockpit up for the two-player split view: both players' work records and the quest
 * notice step. */
void cockpit_split_init(void) {
    _PLW* plw;

    plw = (_PLW*)get_move_work_adrs(2);
    cockpit_state.field_0x000 = 2;
    cockpit_state.pit_trap_shown_0x059 = 0;
    cockpit_state.pit_trap_count_0x058 = 0;
    cockpit_work_init(&cockpit_work[0], &plw[0]);
    cockpit_work_init(&cockpit_work[1], &plw[1]);
    cockpit_state.field_0x078 = 0;
    cockpit_state.quest_notice_step_0x002 = 100;
    cockpit_state.quest_notice_timer_0x004 = 0;
    ai_slots_clear();
}

/* 0x802E4C24 (0x38): Advances the cockpit state's and both work records' frame counters. */
void cockpit_frame_tick(void) {
    cockpit_state.field_0x078++;
    cockpit_work[0].frame_0x190++;
    cockpit_work[1].frame_0x190++;
}

/* 0x802E5D68 (0x128): Starts the flight of item `id` from supply box cell `cell` to the pointer: the flight time
 * grows with the distance between the cell and the pointer anchor. */
void cockpit_pointer_target_set(u16 id, s8 cell) {
    CockpitWork* work;
    _mh_ivec2_ pos;
    _mh_ivec2_ anchor;
    _mh_ivec2_* p;
    s16 dx;
    s16 dy;

    work = &cockpit_work[0];
    work->pointer_item_0x174 = id;
    work->pointer_cell_0x177 = cell;
    cockpit_supply_cell_pos_get(&pos, cell);
    p = (_mh_ivec2_*)get_lsp_data(0xE56, NULL);
    pos.x += p->x;
    pos.y += p->y;
    get_lsp_data(0xE15, &anchor);
    p = (_mh_ivec2_*)get_lsp_data(0xE33, NULL);
    pos.x -= (s16)(p->x + anchor.x);
    pos.y -= (s16)(p->y + anchor.y);
    work->pointer_time_0x178 = (u8)(view_fn_802E4978::lbl_8079A90C * sqrt_f32((f32)(pos.x * pos.x + pos.y * pos.y)));
    if (work->pointer_time_0x178 == 0) {
        work->pointer_time_0x178 = 1;
    }
    work->pointer_left_0x179 = work->pointer_time_0x178;
}

/* 0x802E704C (0xA4): Draws one hand of the quest timer, turned by `value` / 5 steps. */
void cockpit_timer_hand_draw(u16 id, s32 value, const _mh_ivec2_* pos) {
    _SPR_DATA_ spr;

    spr_data_copy(&spr, get_lsp_data(id, NULL));
    spr.tex_id = (u16)(view_fn_802E4978::lbl_8079A918 * (f32)(value / 5));
    draw_sprite(spr, pos);
}

/* 0x802E70F0 (0xD4): Draws the quest timer's minute ring for `value` minutes left (nothing past 55). */
void cockpit_timer_ring_draw(s32 value, const _mh_ivec2_* pos) {
    u8 cell;

    if (value > 0x37) {
        return;
    }
    if (value < 0) {
        value = 0;
    }
    cell = cockpit_timer_ring_cells[value / 5];
    drawshape_init(3, (u16)(((0x4A - value) / 15) * 4));
    drawshape_set_vertex_array(cockpit_timer_ring_verts);
    drawshape_set_flat_color(0xFFFFFFFF);
    drawshape_set_texture_array(0xD, &cockpit_timer_ring_uvs[cell]);
    drawshape_set_offset_ivec2((s16*)pos);
    drawshape_exec();
}

/* 0x802E71C4 (0x1F0): Draws the quest timer panel: blinking red in the last five minutes, then the minute ring and the
 * minutes left of the limit (or the idle face with no limit); the elapsed time is the base time once the sub-state
 * has ended. */
void cockpit_timer_draw(void) {
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    CockpitWork* work = cockpit_work;
    u32 color;
    s32 minute;
    s32 elapsed;
    s32 left;
    s32 limit;
    f32 t;

    set_blendmode(4, 5, 1);
    get_lsp_data(0xB90, &pos);
    color = 0x6DAAA3FF;
    if (cockpit_free_play_ck() != 0) {
        elapsed = 0;
        limit = 0;
    } else {
        minute = 60.0f * Screen_w.frame_scale;
        if (quest_sub_state_end_ck(1) == 0) {
            elapsed = quest_time_elapsed_get();
        } else {
            elapsed = quest_time_base_get();
        }
        if (elapsed <= minute * 5) {
            if (elapsed > minute) {
                t = (work->frame_0x190 & 0x1F) << 11;
            } else {
                t = (work->frame_0x190 & 0xF) << 12;
            }
            color = color_lerp(0xF50C23FF, 0xFF8C9BFF, 0.5f * (1.0f + math_sin_rad(9.58738019e-05f * t)));
        }
        left = quest_time_limit_get();
        elapsed = (left - elapsed) / minute;
        limit = left / minute;
    }
    spr_data_copy(&spr, get_lsp_data(0xB91, NULL));
    spr.color = color;
    draw_sprite(spr, &pos);
    draw_sprite_idx(0xB92, &pos);
    if (limit != 0) {
        cockpit_timer_ring_draw(limit, &pos);
        cockpit_timer_hand_draw(0xB93, limit, &pos);
        cockpit_timer_hand_draw(0xB94, elapsed, &pos);
    } else {
        cockpit_timer_ring_draw(0, &pos);
        draw_sprite_idx(0xB95, &pos);
    }
}

/* 0x802E4C5C (0x5C4): Updates the player's item tag from the act kind: the open, bob and flash frames, then the icon,
 * colour, count and usability - for the item-using kinds the first listed item the player holds (selected slot
 * first). */
void cockpit_item_tag_update(_PLW* plw, CockpitItemTag* tag) {
    u16 ids[10];
    u16* dst;
    const u16* src;
    ItemDataRecord* data;
    s32 i;
    s32 j;
    _SLOTENT* slot;

    tag->tex_idx_0x15 = 0;
    tag->count_0x19 = 0;
    tag->unusable_0x16 = 0;
    tag->open_0x17 = 0;
    tag->flag_0x18 = 0;
    tag->ofs_y_0x0F = 0;
    ids[0] = 0;
    ids[1] = 0;
    ids[2] = 0;
    ids[3] = 0;
    ids[4] = 0;
    ids[5] = 0;
    ids[6] = 0;
    ids[7] = 0;
    ids[8] = 0;
    ids[9] = 0;
    if (plw->field_0x3E9 != 0) {
        tag->open_0x17 = 1;
    }
    if (tag->kind_prev_0x14 != plw->field_0x3E4) {
        tag->kind_prev_0x14 = plw->field_0x3E4;
        tag->open_frame_0x0E = 0;
        tag->bob_frame_0x0D = 0;
        tag->frame_0x0C = 0;
    } else {
        tag->frame_0x0C++;
        if (tag->frame_0x0C >= 190) {
            tag->frame_0x0C = 40;
        } else if ((u8)(tag->frame_0x0C + 166) <= 9) {
            tag->frame_0x0C = 40;
        }
        if ((plw->field_0x309 & 0x20) != 0) {
            tag->frame_0x0C = 100;
        }
        tag->bob_frame_0x0D++;
        if (tag->bob_frame_0x0D > 90) {
            if (tag->bob_frame_0x0D >= 138) {
                tag->bob_frame_0x0D = 90;
            }
            tag->ofs_y_0x0F = 6.0f * math_sin_rad(0.130899697f * (tag->bob_frame_0x0D - 90));
        }
        tag->open_frame_0x0E++;
        if (tag->open_frame_0x0E >= 50) {
            tag->open_frame_0x0E = 30;
        }
    }
    switch (plw->field_0x3E4) {
    default:
    case 0:
    case 14:
        return;
    case 16:
        tag->tex_idx_0x15 = 0x22;
        tag->color_0x10 = 0xFF6347FF;
        return;
    case 1:
    case 10:
        tag->tex_idx_0x15 = 0x22;
        tag->color_0x10 = 0xFFFFFFFF;
        return;
    case 2:
        src = pl_act_kind2_item_ids;
        dst = ids;
        while (*src != 0xFFFF) {
            *dst++ = *src++;
        }
        break;
    case 3:
        src = pl_act_kind3_item_ids;
        dst = ids;
        while (*src != 0xFFFF) {
            *dst++ = *src++;
        }
        break;
    case 4:
        src = pl_act_kind4_item_ids;
        dst = ids;
        while (*src != 0xFFFF) {
            *dst++ = *src++;
        }
        break;
    case 5:
        tag->tex_idx_0x15 = 0x73;
        tag->color_0x10 = 0x43ABDEFF;
        return;
    case 9:
        tag->tex_idx_0x15 = 0x73;
        tag->color_0x10 = 0xCA2F59FF;
        return;
    case 8:
        tag->tex_idx_0x15 = 0x6E;
        tag->color_0x10 = 0xA6A0E6FF;
        return;
    case 11:
        tag->tex_idx_0x15 = 0x30;
        tag->color_0x10 = item_kind_color_tbl[1];
        return;
    case 12:
        tag->tex_idx_0x15 = 0x30;
        tag->color_0x10 = 0xF01010FF;
        if (Pl_item_timer_get(plw, 43) <= 0) {
            tag->unusable_0x16 = 1;
        }
        return;
    case 13:
        tag->tex_idx_0x15 = 0x22;
        tag->color_0x10 = 0xF0F010FF;
        return;
    case 6:
        tag->tex_idx_0x15 = 0x70;
        tag->color_0x10 = 0xD01C27FF;
        return;
    case 7:
        if (cockpit_item_slot_live_ck(plw, plw->field_0x304) != 0 && plw->slot_id[plw->field_0x304].item_id != 0xDF) {
            data = GetItemData(plw->slot_id[plw->field_0x304].item_id);
            tag->count_0x19 = data->max_num_0x003 != 1 ? plw->slot_id[plw->field_0x304].value : 0;
            tag->tex_idx_0x15 = data->tex_idx_0x004;
            tag->color_0x10 = item_kind_color_tbl[data->kind];
            if (pl_item_now_usable_ck(plw, plw->slot_id[plw->field_0x304].item_id) == 0) {
                tag->unusable_0x16 = 1;
            }
        }
        return;
    case 15:
        tag->tex_idx_0x15 = 0x23;
        tag->color_0x10 = 0xFFFFFFFF;
        if (pl_work_flag0_clear_ck() == 0) {
            tag->unusable_0x16 = 1;
        }
        return;
    case 17:
        tag->tex_idx_0x15 = 0x23;
        tag->color_0x10 = 0xFFFFFFFF;
        if (pl_work_flag8_clear_ck() == 0) {
            tag->unusable_0x16 = 1;
        }
        return;
    case 18:
        tag->tex_idx_0x15 = 0x23;
        tag->color_0x10 = 0xF06060FF;
        return;
    }
    for (i = 0; i < 11; i++) {
        if (ids[i] == 0) {
            break;
        }
        slot = &plw->slot_id[plw->field_0x304];
        if (slot->item_id == ids[i] && slot->value > 0) {
            data = GetItemData(ids[i]);
            tag->tex_idx_0x15 = data->tex_idx_0x004;
            tag->count_0x19 = plw->slot_id[plw->field_0x304].value;
            tag->color_0x10 = item_kind_color_tbl[data->kind];
            return;
        }
    }
    for (i = 0; i < 11; i++) {
        if (ids[i] == 0) {
            break;
        }
        for (j = 0; j < 24; j++) {
            if (plw->slot_id[j].item_id == ids[i]) {
                data = GetItemData(ids[i]);
                tag->tex_idx_0x15 = data->tex_idx_0x004;
                tag->count_0x19 = plw->slot_id[j].value;
                tag->color_0x10 = item_kind_color_tbl[data->kind];
                return;
            }
        }
    }
    if (tag->count_0x19 == 0) {
        data = GetItemData(ids[0]);
        tag->tex_idx_0x15 = data->tex_idx_0x004;
        tag->color_0x10 = item_kind_color_tbl[data->kind];
        tag->unusable_0x16 = 1;
    }
}

/* 0x802E5220 (0x64): Sets the digit over the AI NPC from its digit state: none, a green 0x22 or a red 2. */
void cockpit_npc_digit_update(CockpitWork* work) {
    switch (work->ai_npc_0x048->digit_state_0x3C4) {
    case 0:
        work->npc_digit_0x04C = 0;
        work->npc_digit_color_0x050 = 0;
        break;
    case 1:
        work->npc_digit_0x04C = 0x22;
        work->npc_digit_color_0x050 = 0x00FF00FF;
        break;
    case 2:
        work->npc_digit_0x04C = 2;
        work->npc_digit_color_0x050 = 0xFF0000FF;
        break;
    }
}

/* 0x802E5284 (0x140): The single-view cockpit's per-frame update (unless the pad is held): the clocks, notices,
 * gauges, item tag, NPC digit, item ring, target masks, icon animations, the supply box and take menu, the cursors,
 * the hunt log and the prompt; then queues the draw tasks unless an event demo runs. */
void cockpit_frame_update(void) {
    SysQuestMove* move = (SysQuestMove*)get_move_work_adrs(0);
    CockpitWork* work = cockpit_work;
    _PLW* plw = work->plw;

    if (pad_hold_ck() != 1) {
        work->field_0x0D1 = 0;
        cockpit_frame_tick();
        hud_info_toggle_step();
        cockpit_notice_update();
        quest_gauge_update(work, 0xFF);
        cockpit_item_tag_update(plw, &work->item_tag_0x02C);
        if (move->npc_plate_0x0EF != 0) {
            cockpit_npc_digit_update(work);
        }
        cockpit_item_ring_update(plw, work->item_ring_anim_0x16B, work->item_list_anim_0x16F);
        quest_targets_update_b(work);
        cockpit_gauge_icons_update(work);
        cockpit_supply_box_update();
        cockpit_take_menu_update();
        cockpit_icon_cursors_update();
        menu_slot_anim_update(0);
        if (move_work_state_ck() != 0) {
            cockpit_hunt_log_update();
        }
        cockpit_prompt_update();
        ainpc_entry_update_all();
    }
    if (event_demo_ck() != 1) {
        subTransSet((u32)cockpit_draw, 0, 0);
        if (lb_quest_work_active_ck() == 0) {
            subTransSetPrio(6, (u32)ainpc_entry_draw_all, 0, 0);
        }
        subTransSetPrio(6, (u32)menu_slot_panel_draw, 0, 0);
    }
}

/* 0x802E555C (0x158): The single-view cockpit draw task: the party elements, the weapon HUD, the gauges and party
 * list, the item tag and NPC digit, then the open menu (supply box, take menu or quest menu) or the marker window with
 * the item list and ring, the marks, the flash gauge, the note box, the hunt log and the prompt. */
void cockpit_draw(void) {
    SysQuestMove* move = (SysQuestMove*)get_move_work_adrs(0);
    CockpitWork* work = cockpit_work;
    _PLW* plw = work->plw;

    if (get_now_areano() != 0xFF) {
        set_zmode(false, 0, false);
        work->mark_count_0x0CD = 0;
        cockpit_party_elements_draw(work);
        draw_lsp_parts();
        cockpit_weapon_hud_draw();
        if (cockpit_hud_hidden_self_ck(plw) == 1) {
            cockpit_timer_draw();
            quest_gauge_draw();
            cockpit_party_list_draw();
        }
        cockpit_item_tag_draw(work);
        if (move->npc_plate_0x0EF != 0) {
            cockpit_npc_digit_draw(work);
        }
        if (menu_item_frame_update((MenuFrameWork*)plw) == 0) {
            if (plw->field_0x5BC != 0) {
                cockpit_supply_box_draw();
            } else if (plw->take_menu_open_0x5BE != 0) {
                cockpit_take_menu_draw();
            } else if (plw->quest_menu_open_0x5BD != 0) {
                cockpit_quest_menu_draw();
            } else {
                quest_marker_arm();
                if (cockpit_hud_hidden_self_ck(plw) == 1) {
                    cockpit_item_list_draw(plw, work->item_list_anim_0x16F);
                    cockpit_item_ring_draw(plw, work->item_ring_anim_0x16B);
                }
            }
        }
        quest_marks_flush(work);
        cockpit_flash_gauge_draw();
        note_box_draw();
        if (move_work_state_ck() != 0) {
            cockpit_hunt_log_draw();
        }
        cockpit_prompt_draw();
    }
}

/* 0x802E5764 (0x2B0): Draws the name tags of the other live players in the same area over their heads, through
 * player `player`'s camera and clamped to that player's half of the screen: a black shadow, then the name in the
 * player's colour. */
void cockpit_name_tags_draw(CockpitWork* work, u8 player) {
    VEC3 screen;
    u32 camera;
    s32 handle;
    _PLW* plw;
    s32 max;
    s32 i;
    f32 top;
    f32 bottom;
    s16 x;
    s16 y;
    u32 area;
    char* name;

    handle = ((nw4r::g3d::ScnRoot*)pRoot)->GetCamera(Screen_w.camera_view.player_camera_0x50[player]);
    word_copy_return_dst(&camera, &handle);
    VEC3_ctor(&screen);
    top = 18.0f;
    bottom = 480.0f;
    plw = (_PLW*)get_move_work_adrs(2);
    max = (u16)get_move_work_max(2);
    for (i = 0; i < max; i++, plw++) {
        if (plw->slot_active != 0 && plw->chunk_ofs != player && plw->field_0x001 != 0) {
            area = Pl_area_flag_get(work->plw->chunk_ofs);
            if (Pl_area_flag_get(plw->chunk_ofs) == area) {
                chara_icon_pos_get(plw, 0, &work->name_pos_0x054[plw->chunk_ofs]);
                camera_world_to_screen(&camera, &screen, &work->name_pos_0x054[plw->chunk_ofs]);
                if (screen.z < 0.0f || screen.z > 1.0f) {
                    continue;
                }
                if (player == 0) {
                    bottom = 240.0f;
                }
                if (player != 0) {
                    top = 258.0f;
                }
                screen.x *= hud_screen_width_get();
                screen.y *= 1.07142854f;
                if (screen.y < top || screen.y > bottom) {
                    continue;
                }
                x = screen.x;
                y = screen.y;
                if (get_arena_cfg(player, 12) == 0 || isServerSelectState() == 0) {
                    name = plw->hunter_0x5CA.name_0x5CA;
                } else {
                    name = plw->hunter_0x5CA.id_0x5DB;
                }
                x = menu_text_center_x(name, x, 18);
                y -= 18;
                font_set_size(18, 18);
                flfntSetColor(0xFF);
                font_locate(x + 2, y + 1);
                font_print((s8*)name);
                flfntSetColor(player_color_get(plw->chunk_ofs));
                font_locate(x, y);
                font_print((s8*)name);
            }
        }
    }
    font_flush();
}

/* 0x802E5A14 (0x1A0): Steps the supply box and take menu of the first cockpit: the pending take and supply actions,
 * the cursor and frame animations, the item flight to the pointer, and the menus' busy flags. */
void cockpit_supply_box_update(void) {
    CockpitWork* work = cockpit_work;
    _PLW* plw = work->plw;

    if (work->supply_busy_0x176 != 0) {
        fn_8027B0BC(plw);
    }
    if (work->take_busy_0x17D == 1) {
        fn_8027B358(plw);
    }
    if (plw->field_0x5BC != 0) {
        if (menu_item_frame_update((MenuFrameWork*)plw) == 0) {
            work->supply_busy_0x176 = 1;
        } else {
            work->supply_busy_0x176 = 0;
        }
        work->cursor_frame_0x17B++;
        if (sprite_anim_last_frame(0xE53) < work->cursor_frame_0x17B) {
            work->cursor_frame_0x17B = 0;
        }
        if (work->box_frame_0x17C != 0) {
            work->box_frame_0x17C++;
            if (sprite_anim_last_frame(0xE33) < work->box_frame_0x17C) {
                work->box_frame_0x17C = 0;
            }
        }
        if (work->pointer_time_0x178 != 0) {
            if (--work->pointer_left_0x179 == 0) {
                work->pointer_time_0x178 = 0;
                work->box_frame_0x17C = 1;
            }
        }
        if (menu_item_frame_update((MenuFrameWork*)plw) == 0) {
            work->supply_notice_0x17A = 0;
        } else {
            work->supply_notice_0x17A = 1;
        }
    } else {
        work->supply_busy_0x176 = 0;
        work->supply_notice_0x17A = 1;
        work->pointer_time_0x178 = 0;
        work->box_frame_0x17C = 0;
    }
    if (plw->take_menu_open_0x5BE != 0) {
        if (menu_item_frame_update((MenuFrameWork*)plw) == 0) {
            work->take_busy_0x17D = 1;
            return;
        }
        work->take_busy_0x17D = 0;
        return;
    }
    work->take_busy_0x17D = 0;
}

#pragma fp_contract off
static inline u16 item_count_label_id(s16 count) {
    u16 id = 0xCA0;
    if (count < 10) {
        id = 0xC9F;
    }
    return id;
}

static inline u8 quest_notice_flag_get(u32 hidden) {
    u8 flag = 4;
    if (hidden == 1) {
        flag = 0;
    }
    return flag;
}

static inline u8 quest_mark_notice_flag_get(s32 visible) {
    u8 flag = 4;
    if (visible != 0) {
        flag = 0;
    }
    return flag;
}

/* 0x802E5E90 (0x5DC): Draws the supply box: the 5x8 grid of the quest's supply items (taken cells empty) with the
 * cursor, the item flying to the pointer, the selected item's name, count, rarity and room warning, and the help. */
void cockpit_supply_box_draw(void) {
    _SPR_DATA_ spr;
    _mh_ivec2_ base;
    _mh_ivec2_ row_pos;
    _mh_ivec2_ cell_pos;
    char text[4];
    SysQuestMove* move;
    const u16* row;
    CockpitWork* work;
    const u16* col;
    QuestWork* quest;
    const _SPR_DATA_* rec;
    const Q_ItemPair* pair;
    ItemDataRecord* data;
    u8** help;
    s32 r;
    s32 c;
    s32 first;
    s32 idx;
    s32 warn;
    u32 color;
    s32 frame;
    f32 t;
    f32 inv;

    move = (SysQuestMove*)get_move_work_adrs(0);
    if (move != NULL) {
        quest = move->quest_0x0DC;
        if (quest != NULL) {
            work = cockpit_work;
            set_blendmode(4, 5, 1);
            get_lsp_data(0xE15, &base);
            draw_sprite_ary(cockpit_supply_box_frame_ids, &base);
            r = 0;
            row = cockpit_slot_row_sprite_ids;
            first = 0;
            do {
                rec = get_lsp_data(*row, NULL);
                row_pos.x = base.x + rec->pos.x;
                row_pos.y = base.y + rec->pos.y;
                draw_sprite_ary(cockpit_slot_column_sprite_ids, &row_pos);
                c = 0;
                col = cockpit_slot_column_sprite_ids;
                do {
                    idx = first + c;
                    rec = get_lsp_data(*col, NULL);
                    if (quest->supply_cursor_0x5E1 == idx) {
                        set_blendmode(4, 1, 1);
                        cell_pos.x = row_pos.x + rec->pos.x;
                        cell_pos.y = row_pos.y + rec->pos.y;
                        draw_sprite_anim_idx(0xE53, work->cursor_frame_0x17B, &cell_pos);
                        set_blendmode(4, 5, 1);
                    }
                    if (((1 << (idx & 0x1F)) & quest->supply_taken_0x684[idx >> 5]) == 0 &&
                        quest->supply_0x5E2[idx].id != 0) {
                        rec = get_lsp_data(*col, NULL);
                        cell_pos.x = row_pos.x + rec->pos.x;
                        cell_pos.y = row_pos.y + rec->pos.y;
                        draw_itemicon_item_id_idx(0xE56, quest->supply_0x5E2[idx].id, &cell_pos);
                    }
                    col++;
                    c++;
                } while (c < 8);
                row++;
                first += 8;
                r++;
            } while (r < 5);
            draw_sprite_anim_idx(0xE33, work->box_frame_0x17C, &base);
            if (work->pointer_time_0x178 != 0) {
                t = (f32)work->pointer_left_0x179 / (f32)work->pointer_time_0x178;
                inv = 1.0f - t;
                frame = inv * sprite_anim_last_frame(0xE56);
                cockpit_supply_cell_pos_get(&row_pos, work->pointer_cell_0x177);
                rec = get_lsp_data(0xE56, NULL);
                row_pos.x += rec->pos.x;
                row_pos.y += rec->pos.y;
                rec = get_lsp_data(0xE33, NULL);
                cell_pos.x = rec->pos.x + base.x;
                cell_pos.y = rec->pos.y + base.y;
                sprite_frame_apply(&spr, 0xE56, frame, NULL);
                spr.pos.x = row_pos.x * t + cell_pos.x * inv;
                spr.pos.y = row_pos.y * t + cell_pos.y * inv;
                draw_itemicon_item_id(spr, work->pointer_item_0x174, NULL);
            }
            pair = NULL;
            if (((1 << (quest->supply_cursor_0x5E1 & 0x1F)) & quest->supply_taken_0x684[quest->supply_cursor_0x5E1 >> 5]) == 0) {
                pair = &quest->supply_0x5E2[quest->supply_cursor_0x5E1];
                if (pair->id == 0) {
                    pair = NULL;
                }
            }
            warn = 0;
            if (pair != NULL) {
                _PLW* plw = work->plw;
                if (pl_item_room_get(plw, pair->id, (s16)pair->num) < (s16)pair->num) {
                    if (pl_item_slot_find(plw, pair->id) == 0xFFFF) {
                        draw_sprite_idx(0xE47, &base);
                        warn = 1;
                    } else {
                        draw_sprite_idx(0xE48, &base);
                        warn = 2;
                    }
                }
                draw_itemicon_item_id_idx(0xE26, pair->id, &base);
                draw_font_idx(0xE2B, (s8*)ItemName(pair->id), 1, &base);
                sprintf(text, "%d", (s16)pair->num);
                draw_font_idx(0xE2C, (s8*)text, 1, &base);
                data = GetItemData(pair->id);
                color = get_rare_color(data->level_0x01);
                spr_data_copy(&spr, get_lsp_data(0xE28, NULL));
                spr.color = color;
                draw_sprite(spr, &base);
                draw_number_idx(0xE29, data->level_0x01 + 1, color, &base);
            }
            menu_item_row_draw_by_lsp(0xE14, (u16*)pair);
            if (em_prog_flag_ck() == 0) {
                get_lsp_data(0xE13, &row_pos);
                draw_window_frame_preset(row_pos.x, row_pos.y, 2);
                help = get_str_tbl(0x58);
                font_set_size(18, 18);
                font_print_ex(row_pos.x + 24, row_pos.y + 16, 0, (s8*)help[1]);
                if (warn != 0) {
                    font_print_ex(row_pos.x + 24, row_pos.y + 52, 2, (s8*)help[2]);
                }
            }
            font_flush();
        }
    }
}
#pragma fp_contract on

/* 0x802E646C (0x640): Draws the take-item menu: the three item rows (name and count, or the empty/none text) with
 * the row cursor, then by the menu state the count page, the yes/no confirmation and the empty-row notice. */
void cockpit_take_menu_draw(void) {
    _SPR_DATA_ spr;
    char text[8];
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    CockpitSlotPair pair;
    CockpitWork* work = cockpit_work;
    SysQuestMove* move;
    u16 id;
    QuestWork* quest;
    _PLW* plw;
    u8** strs;
    s32 row;
    bool rows_state;
    u32 count;
    s16 x;

    move = (SysQuestMove*)get_move_work_adrs(0);
    if (move != NULL) {
        quest = move->quest_0x0DC;
        if (quest != NULL) {
            plw = work->plw;
            if (plw != NULL) {
                set_blendmode(4, 5, 1);
                get_lsp_data(0xE57, &base);
                draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x109), &base);
                get_lsp_data(0xE57, &base);
                row = 0;
                do {
                    get_lsp_data(((const u16*)get_menu_lsp_tbl(0x10A))[row], &pos);
                    pos.x += base.x;
                    pos.y += base.y;
                    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x10B), &pos);
                    if (row == quest->take_row_0x68C && plw->cockpit_menu_0x5C0 == 1) {
                        put_menu_cursor((u16*)get_menu_lsp_tbl(0x10D), 0, &pos);
                    }
                    draw_sprite_idx(((const u16*)get_menu_lsp_tbl(0x10C))[row], &pos);
                    rows_state = plw->cockpit_menu_0x5C0 == 1;
                    em_work_slot_pair_get(row, (s16*)&pair);
                    if (pair.id == 0) {
                        spr_data_copy(&spr, get_lsp_data(0xE79, NULL));
                        spr.color = GetMenuFontColor(0, row == quest->take_row_0x68C, rows_state, 0);
                        draw_font(spr, (s8*)get_str_tbl(28)[0], 0, &pos);
                    } else if (pair.count != 0) {
                        spr_data_copy(&spr, get_lsp_data(0xE79, NULL));
                        spr.color = GetMenuFontColor(1, row == quest->take_row_0x68C, rows_state, 0);
                        draw_font(spr, (s8*)ItemName(pair.id), 0, &pos);
                        sprintf(text, "%d", pair.count);
                        id = 0xE7B;
                        if (pair.count < 10) {
                            id = 0xE7A;
                        }
                        spr_data_copy(&spr, get_lsp_data(id, NULL));
                        spr.color = GetMenuFontColor(1, row == quest->take_row_0x68C, rows_state, 0);
                        x = menu_text_center_x(text, (s16)(pos.x + spr.pos.x), spr.width);
                        font_set_size(spr.width, spr.height);
                        font_print_ex(x, (s16)(pos.y + spr.pos.y), (s16)spr.color, (s8*)"%s", text);
                    } else {
                        spr_data_copy(&spr, get_lsp_data(0xE79, NULL));
                        spr.color = GetMenuFontColor(0, row == quest->take_row_0x68C, rows_state, 0);
                        draw_font(spr, (s8*)get_str_tbl(28)[1], 0, &pos);
                    }
                    row++;
                } while (row < 3);
                if (plw->cockpit_menu_0x5C0 >= 2) {
                    get_lsp_data(0xE8C, &base);
                    em_work_slot_pair_get(quest->take_row_0x68C, (s16*)&pair);
                    count = Pl_item_timer_get(plw, pair.id);
                    if ((u32)pair.count < count) {
                        count = pair.count;
                    }
                    put_page_count((u16*)get_menu_lsp_tbl(0x10F), quest->take_count_0x68E, count, quest->take_flags_0x696,
                                   &base);
                    if (plw->cockpit_menu_0x5C0 == 2) {
                        get_lsp_data(0xE9A, &pos);
                        pos.x += base.x;
                        pos.y += base.y;
                        put_menu_cursor((u16*)get_menu_lsp_tbl(0x110), 0, &pos);
                    }
                }
                if (plw->cockpit_menu_0x5C0 == 3) {
                    spr_data_copy(&spr, get_lsp_data(0xE9B, &base));
                    draw_window_box(base.x, base.y, spr.width, spr.height, 0x82574FFF, 0);
                    get_lsp_data(0xE9C, &base);
                    get_lsp_data(((const u16*)get_menu_lsp_tbl(0x111))[quest->take_confirm_0x68D], &pos);
                    pos.x += base.x;
                    pos.y += base.y;
                    put_menu_cursor((u16*)get_menu_lsp_tbl(0x110), 0, &pos);
                    get_lsp_data(0xE9C, &base);
                    spr_data_copy(&spr, get_lsp_data(0x6E, NULL));
                    strs = get_str_tbl(29);
                    get_lsp_data(0xE9D, &pos);
                    pos.x += base.x;
                    pos.y += base.y;
                    spr.color = GetMenuFontColor(1, quest->take_confirm_0x68D == 0, 1, 0);
                    draw_font(spr, (s8*)strs[0], 1, &pos);
                    get_lsp_data(0xE9E, &pos);
                    pos.x += base.x;
                    pos.y += base.y;
                    spr.color = GetMenuFontColor(1, quest->take_confirm_0x68D == 1, 1, 0);
                    draw_font(spr, (s8*)strs[1], 1, &pos);
                }
                if (plw->cockpit_menu_0x5C0 == 3 && quest->take_row_0x68C == 0) {
                    spr_data_copy(&spr, get_lsp_data(0xE85, &base));
                    draw_window_frame_style(base.x, base.y, spr.width, spr.height, 0);
                    get_lsp_data(0xE86, &base);
                    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x10E), &base);
                    spr_data_copy(&spr, get_lsp_data(0xE87, &pos));
                    draw_font(spr, (s8*)get_str_tbl(27)[0], 0, &base);
                }
            }
        }
    }
}

/* 0x802E6AAC (0x408): Draws the quest menu: its two rows (the second only when an item notice is pending) with the
 * row frame, then by the menu state the row's help window or the yes/no confirmation. */
void cockpit_quest_menu_draw(void) {
    _SPR_DATA_ spr;
    union {
        _mh_ivec2_ pos;
        s32 word;
    } base;
    _mh_ivec2_ pos;
    s32 anchor;
    const u16* ids;
    CockpitWork* work = cockpit_work;
    SysQuestMove* move;
    QuestWork* quest;
    _PLW* plw;
    u8** strs;
    s32 row;
    bool enabled;
    bool active;

    move = (SysQuestMove*)get_move_work_adrs(0);
    if (move != NULL) {
        quest = move->quest_0x0DC;
        if (quest != NULL) {
            plw = work->plw;
            if (plw != NULL) {
                get_lsp_data(0xEA0, &base.pos);
                draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x112), &base.pos);
                row = 0;
                do {
                    ids = (const u16*)get_menu_lsp_tbl(0x113);
                    anchor = base.word;
                    put_lsp_anchor_offset(ids[row], &pos, &anchor);
                    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x114), &pos);
                    enabled = 0;
                    if (row == 1) {
                        if (Pl_motion_input_ck(0) != 1 && quest_flag_80_ck(NULL) == 1 &&
                            (quest_item_work_notify(1) == 1 || quest_item_work_notify(2) == 1)) {
                            enabled = 1;
                        }
                    } else {
                        enabled = 1;
                    }
                    active = 0;
                    if (plw->cockpit_menu_0x5C0 > 1 && plw->cockpit_menu_row_0x5BF == row) {
                        active = 1;
                    }
                    strs = get_str_tbl(66);
                    spr_data_copy(&spr, get_lsp_data(0xEB8, NULL));
                    spr.color = GetMenuFontColor(enabled, plw->cockpit_menu_0x5C0 == row, 1, active);
                    draw_font(spr, (s8*)strs[row], 0, &pos);
                    if (plw->cockpit_menu_row_0x5BF == row) {
                        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x115), &pos);
                    }
                    row++;
                } while (row < 2);
                strs = get_str_tbl(65);
                switch (plw->cockpit_menu_0x5C0) {
                case 1:
                    spr_data_copy(&spr, get_lsp_data(0xEBD, &base.pos));
                    draw_window_frame_style(base.pos.x, base.pos.y, spr.width, spr.height, 0);
                    get_lsp_data(0xEBE, &base.pos);
                    spr_data_copy(&spr, get_lsp_data(0xEBF, NULL));
                    draw_font(spr, (s8*)strs[plw->cockpit_menu_row_0x5BF], 0, &base.pos);
                    break;
                case 2:
                    if (plw->cockpit_menu_row_0x5BF == 1) {
                        spr_data_copy(&spr, get_lsp_data(0xEC0, &base.pos));
                        draw_window_frame_style(base.pos.x, base.pos.y, spr.width, spr.height, 0);
                        get_lsp_data(0xEC1, &base.pos);
                        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0x116), &base.pos);
                        spr_data_copy(&spr, get_lsp_data(0xEC6, &pos));
                        draw_font(spr, (s8*)strs[2], 0, &base.pos);
                    }
                    break;
                }
                if (plw->cockpit_menu_0x5C0 == 2) {
                    spr_data_copy(&spr, get_lsp_data(0xEC7, &base.pos));
                    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xA), &base.pos);
                    draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xB), &base.pos);
                    if (quest->take_confirm_0x68D == 0) {
                        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xD), &base.pos);
                    }
                    if (quest->take_confirm_0x68D == 1) {
                        draw_sprite_ary((const u16*)get_menu_lsp_tbl(0xC), &base.pos);
                    }
                    strs = get_str_tbl(67);
                    spr_data_copy(&spr, get_lsp_data(0x1AE, NULL));
                    spr.color = GetMenuFontColor(1, quest->take_confirm_0x68D == 0, 1, 0);
                    draw_font(spr, (s8*)strs[0], 1, &base.pos);
                    spr_data_copy(&spr, get_lsp_data(0x1B6, NULL));
                    spr.color = GetMenuFontColor(1, quest->take_confirm_0x68D == 1, 1, 0);
                    draw_font(spr, (s8*)strs[1], 1, &base.pos);
                }
            }
        }
    }
}

/* 0x802E6EB4 (0x198): Draws the first player's flash gauge while act kind 7 charges: the frame animation and the
 * bar, filled and coloured by the charge over its limit. */
void cockpit_flash_gauge_draw(void) {
    _mh_ivec2_ pos;
    CockpitWork* work = cockpit_work;
    _PLW* plw = work->plw;
    const _SPR_DATA_* rec;
    u32 color;
    s16 width;
    f32 t;

    if (plw->field_0x00A == 7 && plw->field_0x3A4 != 0) {
        set_blendmode(4, 5, 1);
        get_lsp_data(0xBCE, &pos);
        draw_sprite_anim_ary(cockpit_flash_anim_ids, work->flash_frame_0x180, &pos);
        rec = get_lsp_data(0xBD2, NULL);
        t = plw->field_0x3A4 * (1.0f / plw->field_0x3A5);
        width = (rec->width * (f32)plw->field_0x3A4) / plw->field_0x3A5;
        color = color_lerp(0xC50A83FF, 0xD813BEFF, t);
        drawshape_init(3, 0xFFFF);
        drawshape_set_vertex_rect((s16)(rec->pos.x + pos.x), (s16)(rec->pos.y + pos.y), width, rec->height);
        drawshape_set_corner_colors(0xC50A83FF, color, color, 0xC50A83FF);
        drawshape_exec();
    }
}

/* 0x802E7408 (0x140).  The first bar's blend parameter: the player's slot 45 state picks a row of
 * the 2x5 `u16` table at 0x805D5C74 and the argument picks the column pair, and a zero result means
 * "no change" - the caller's previous value is kept by `quest_bar_id_keep`.  The `li r4,0 / cmplwi /
 * bne / li r4,1` sequence is the target's shape for the "slot is 0x7D" test, so the cancel flag is a
 * materialised local, not a folded comparison. */
u16 quest_bar_a_next_id(u16 id, u8 kind, _PLW* plw) {
    u16 off = 0;

    if (plw->field_0x464 == 0) {
        s32 row = 0;
        u8 slot = Pl_Skill_slot_item_get(plw, 45);
        s32 cancel = 0;

        if (slot == 0x7D) {
            cancel = 1;
        }
        if (cancel == 0) {
            switch (slot) {
            case 0x7C:
                off = 0;
                break;
            case 0x7E:
                off = 3;
                break;
            case 0x7F:
                off = 4;
                break;
            default:
                off = 2;
                break;
            }
            switch (kind) {
            case 0:
                row = 0;
                off = 0;
                break;
            case 1:
                row = 0;
                break;
            case 3:
                row = 1;
                break;
            default:
                nw4r::db::Panic("cockpit_quest.cpp", 0x7F8, "NW4R:Failed assertion 0");
                break;
            }
            off = quest_bar_id_step_table[row][off];
        }
    }
    if (off != 0) {
        return (u16)(id + off);
    }
    return quest_bar_id_keep(id, plw);
}

/* 0x802E7548 (0x148).  The second bar's blend parameter - `quest_bar_a_next_id`'s twin one slot family
 * over (slot 46, table values 0x80/0x82/0x83, kinds 0/2/4, line 2088), and its "off is zero" case
 * falls out as the target's mask `(-off | off) >> 31` rather than a branch (the caller then keeps
 * the previous value by adding it).  Its gate is `_PLW`'s +0x466 (`field_0x466`; `Pl/pl_act.cpp`
 * clamps the same field to 0x2328). */
u16 quest_bar_b_next_id(s32 id, u8 kind, _PLW* plw) {
    u16 off = 0;

    if (plw->field_0x466 == 0) {
        s32 row = 0;
        u8 slot = Pl_Skill_slot_item_get(plw, 46);
        s32 cancel = 0;

        if (slot == 0x81) {
            cancel = 1;
        }
        if (cancel == 0) {
            switch (slot) {
            case 0x80:
                off = 0;
                break;
            case 0x82:
                off = 3;
                break;
            case 0x83:
                off = 4;
                break;
            default:
                off = 2;
                break;
            }
            switch (kind) {
            case 0:
                row = 0;
                off = 0;
                break;
            case 2:
                row = 0;
                break;
            case 4:
                row = 1;
                break;
            default:
                nw4r::db::Panic("cockpit_quest.cpp", 0x828, "NW4R:Failed assertion 0");
                break;
            }
            off = quest_bar_id_step_table[row][off];
        }
    }
    id = off != 0 ? (u16)(id + off) : 0;
    return id;
}

/* 0x802E7690 (0xF8).  One 0x10-byte blend record: with `t == 0` the three components are copied,
 * otherwise they are interpolated between `a` and `b` by `0.5f * (1.0f - anim_tick_cos(t))`
 * clamped to [0.0f, 1.0f]; the fourth word is always copied.  `anim_tick_cos` maps the blend clock to
 * a fraction (`1.0f` is 1.0f, `0.0f` 0.0f). */
void quest_blend_lerp(HudBlend* out, const HudBlend* a, const HudBlend* b, u16 t) {
    if (t == 0) {
        out->x = a->x;
        out->y = a->y;
        out->z = a->z;
    } else {
        f32 f = 0.5f * (1.0f - anim_tick_cos(t));

        if (f < 0.0f) {
            f = 0.0f;
        }
        if (f > 1.0f) {
            f = 1.0f;
        }
        out->x = color_lerp(a->x, b->x, f);
        out->y = color_lerp(a->y, b->y, f);
        out->z = color_lerp(a->z, b->z, f);
    }
    out->tag = a->tag;
}

/* 0x802E7788.  Steps both gauge colour blends: the first follows the act kind's bar id, the second the
 * player's health status (stagger, the 0xCC/0xC0/0x88 damage conditions, low health). */
void quest_gauge_blend_update(CockpitWork* work) {
    _PLW* plw = work->plw;
    u8 kind = pl_act_kind_get(plw->area_0x16);
    s32 step;
    const u32* to;

    switch (kind) {
    case 1:
    case 3:
        work->blend_0x13E = quest_bar_a_next_id(work->blend_0x13E, kind, plw);
        break;
    default:
        work->blend_0x13E = quest_bar_id_keep(work->blend_0x13E, plw);
        break;
    }
    quest_blend_lerp(&work->bar_a_0x144, (const HudBlend*)quest_bar_a_blend_from,
                     (const HudBlend*)quest_bar_a_blend_to, work->blend_0x13E);
    if (Pl_act_state_ck(plw) == 0) {
        if (Pl_dm_condition_ck(plw, 0xCC) == 0) {
            if (plw->field_0x378 > 0x96) {
                switch (kind) {
                default:
                    work->blend_0x140 = 0;
                    break;
                case 2:
                case 4:
                    work->blend_0x140 = quest_bar_b_next_id(work->blend_0x140, kind, plw);
                    break;
                }
                quest_blend_lerp(&work->bar_b_0x154, (const HudBlend*)quest_bar_b_blend_base_from,
                                 (const HudBlend*)quest_bar_b_blend_normal_to, work->blend_0x140);
                return;
            }
            work->blend_0x140 += 0x5B0;
            quest_blend_lerp(&work->bar_b_0x154, (const HudBlend*)quest_bar_b_blend_low_from,
                             (const HudBlend*)quest_bar_b_blend_low_to, work->blend_0x140);
            return;
        }
        step = 0x200;
        to = quest_bar_b_blend_dm_to;
        if (Pl_dm_condition_ck(plw, 0xC0) != 0) {
            to = quest_bar_b_blend_dm_c0_to;
        }
        if (Pl_dm_condition_ck(plw, 0x88) != 0) {
            step = 0x300;
        }
        work->blend_0x140 += step;
        quest_blend_lerp(&work->bar_b_0x154, (const HudBlend*)quest_bar_b_blend_base_from, (const HudBlend*)to,
                         work->blend_0x140);
        return;
    }
    work->blend_0x140 += 0x400;
    quest_blend_lerp(&work->bar_b_0x154, (const HudBlend*)quest_bar_b_blend_flash,
                     (const HudBlend*)(quest_bar_b_blend_flash + 3), work->blend_0x140);
}

/* 0x802E796C.  Per-frame update of the health and stagger gauges: the shake clock, the eased stagger value, the
 * low-stagger notice and the flash effect's frame counter. */
void quest_gauge_update(CockpitWork* work, u8 mode) {
    _PLW* plw = work->plw;
    _mh_ivec2_ pos;
    HudNotice* notice;
    _SPR_DATA_* anchor;
    u32 limit_shake;
    u32 limit_notice;

    quest_gauge_blend_update(work);
    if (work->health_hit_clock_0x13A != 0) {
        work->health_hit_clock_0x13A--;
    }
    if ((s16)(plw->health + ((plw->health_max * 25) / 100)) < work->health_prev_0x13C) {
        work->health_hit_clock_0x13A = 9;
    }
    work->health_prev_0x13C = plw->health;
    if (plw->kind_0x09 != 3) {
        if (work->stagger_flash_0x136 != 0) {
            if (work->stagger_flash_0x136 > 8) {
                work->stagger_flash_0x136 -= 8;
                work->stagger_shown_0x134 = plw->field_0x37E;
            } else {
                work->stagger_flash_0x136 = 0;
                work->stagger_shown_0x134 = 0;
            }
        }
    } else if (plw->field_0x00A != 8) {
        work->stagger_flash_0x136 = 0xFF;
        work->stagger_shown_0x134 += 4;
        if (work->stagger_shown_0x134 > plw->field_0x37E) {
            work->stagger_shown_0x134 = plw->field_0x37E;
        }
    }
    if (plw->field_0x37E <= 0x19) {
        if (plw->field_0x00A != 8) {
            if (plw->field_0x37E != 0) {
                limit_shake = 0x32;
                limit_notice = 0x3C;
            } else {
                limit_shake = 0x19;
                limit_notice = 0x1E;
            }
            work->stagger_shake_idx_0x137++;
            if (work->stagger_shake_idx_0x137 > limit_shake) {
                work->stagger_shake_idx_0x137 = 1;
            }
            work->stagger_notice_clock_0x138++;
            if (work->stagger_notice_clock_0x138 > limit_notice) {
                if (mode == 0xFF) {
                    get_lsp_data(0xBAC, &pos);
                    anchor = get_lsp_data(0xBAE, NULL);
                    pos.y += (s16)((((plw->field_0x380 * 0xB4) / 150) - anchor->height) - 0x11);
                    notice = hud_notice_spawn(0, 1, pos.x, pos.y, 1, 1, 0);
                } else {
                    switch (mode) {
                    case 0:
                        get_lsp_data(0xCCA, &pos);
                        break;
                    case 1:
                        get_lsp_data(0xCCB, &pos);
                        break;
                    default:
                        pos.y = 0;
                        pos.x = 0;
                        break;
                    }
                    anchor = get_lsp_data(0xCF7, NULL);
                    pos.x += anchor->pos.x;
                    pos.y += anchor->pos.y;
                    anchor = get_lsp_data(0xCF9, NULL);
                    pos.y += (s16)((((plw->field_0x380 * 0x84) / 150) - anchor->height) - 0x11);
                    notice = hud_notice_spawn(0, 9, pos.x, pos.y, 1, 1, 0);
                }
                if (notice != 0) {
                    hud_notice_set_flag_ptr(notice, &work->notice_flag_0x139);
                    sysSE_req(0x1E);
                }
                work->stagger_notice_clock_0x138 = 0;
            }
        }
    } else {
        work->stagger_shake_idx_0x137 = 0;
        work->stagger_notice_clock_0x138 = 0x3C;
    }
    work->notice_flag_0x139 = quest_notice_flag_get(cockpit_hud_hidden_ck(plw, mode));
    if (work->flash_hold_0x17F != 0) {
        work->flash_frame_0x180++;
        if ((s32)sprite_ary_last_frame(cockpit_flash_anim_ids) < (s32)work->flash_frame_0x180) {
            work->flash_frame_0x180 = 0;
        }
    } else {
        work->flash_frame_0x180 = 0;
    }
    if (work->flash_hold_0x17F != 0) {
        work->flash_hold_0x17F--;
    }
}

/* 0x802E7CFC.  Draws one gauge bar of sprite `id`: the filled part up to `shown` (blending from the first to the
 * second colour, or on to the third past the half), and a tail quad from `shown` to `ghost` in the tag colour. */
void quest_gauge_bar_draw(u16 id, s32 shown, s32 ghost, s32 capacity, s32 units, const HudBlend* colors,
                          const _mh_ivec2_* offset, s16 width) {
    _SPR_DATA_* rec = get_lsp_data(id, NULL);
    f32 bar_width;
    _mh_ivec2_ origin;
    f32 fill;
    f32 scale[2];
    _mh_ivec2_ verts[12];
    u32 cols[12];
    s16 bottom;
    u8 count;

    if (width <= 0) {
        bar_width = (f32)rec->width;
    } else {
        bar_width = (f32)width;
    }
    uv_pair_copy(&origin, &rec->pos);
    origin.x += offset->x;
    origin.y += offset->y;
    fill = (2.0f * (f32)shown) / (f32)capacity;
    scale[0] = bar_width / (f32)units;
    scale[1] = 1.0f;
    _mh_ivec2_* tail = verts;
    u32* tail_cols = cols;
    verts[0].x = verts[3].x = origin.x;
    verts[1].x = verts[2].x = origin.x + shown;
    verts[0].y = verts[1].y = origin.y;
    bottom = origin.y + rec->height;
    verts[2].y = verts[3].y = bottom;
    cols[0] = cols[3] = colors->x;
    if (fill <= 1.0f) {
        count = 4;
        cols[1] = cols[2] = color_lerp(colors->x, colors->y, fill);
    } else {
        count = 8;
        verts[5].x = verts[6].x = verts[2].x;
        verts[1].x = verts[2].x = verts[4].x = verts[7].x = origin.x + capacity / 2;
        verts[4].y = verts[5].y = origin.y;
        verts[6].y = verts[7].y = bottom;
        cols[1] = cols[2] = cols[4] = cols[7] = colors->y;
        cols[5] = cols[6] = color_lerp(colors->y, colors->z, fill - 1.0f);
    }
    if (ghost > shown) {
        tail += count;
        tail_cols += count;
        tail[0].x = tail[3].x = origin.x + shown;
        tail[1].x = tail[2].x = origin.x + ghost;
        tail[0].y = tail[1].y = verts[0].y;
        tail[2].y = tail[3].y = verts[2].y;
        tail_cols[0] = tail_cols[1] = tail_cols[2] = tail_cols[3] = colors->tag;
        count += 4;
    }
    drawshape_init(3, count);
    drawshape_set_vertex_array(verts);
    drawshape_set_color_array(cols);
    drawshape_set_scale_ivec2(&origin.x, scale);
    drawshape_exec();
}

s16 quest_gauge_shake_scale = 2;

s16 quest_gauge_shake_table[12] = {
    0, 1, 0, 2, 0, 3, 0, 3, 0, 3, 0, 3,
};

_mh_ivec2_ quest_health_tick_verts[64] = {
    { 71, 9 }, { 71, 31 }, { 86, 31 }, { 86, 9 },
    { 86, 9 }, { 86, 31 }, { 120, 31 }, { 120, 9 },
    { 120, 9 }, { 120, 31 }, { 154, 31 }, { 154, 9 },
    { 154, 9 }, { 154, 31 }, { 188, 31 }, { 188, 9 },
    { 188, 9 }, { 188, 31 }, { 222, 31 }, { 222, 9 },
    { 222, 9 }, { 222, 31 }, { 256, 31 }, { 256, 9 },
    { 256, 9 }, { 256, 31 }, { 290, 31 }, { 290, 9 },
    { 290, 9 }, { 290, 31 }, { 324, 31 }, { 324, 9 },
    { 324, 9 }, { 324, 31 }, { 358, 31 }, { 358, 9 },
    { 358, 9 }, { 358, 31 }, { 392, 31 }, { 392, 9 },
    { 392, 9 }, { 392, 31 }, { 426, 31 }, { 426, 9 },
    { 426, 9 }, { 426, 31 }, { 460, 31 }, { 460, 9 },
    { 460, 9 }, { 460, 31 }, { 494, 31 }, { 494, 9 },
    { 494, 9 }, { 494, 31 }, { 528, 31 }, { 528, 9 },
    { 528, 9 }, { 528, 31 }, { 562, 31 }, { 562, 9 },
    { 562, 9 }, { 562, 31 }, { 596, 31 }, { 596, 9 },
};

_mh_ivec2_ quest_health_tick_verts_wide[64] = {
    { 71, 9 }, { 71, 31 }, { 86, 31 }, { 86, 9 },
    { 86, 9 }, { 86, 31 }, { 130, 31 }, { 130, 9 },
    { 130, 9 }, { 130, 31 }, { 174, 31 }, { 174, 9 },
    { 174, 9 }, { 174, 31 }, { 218, 31 }, { 218, 9 },
    { 218, 9 }, { 218, 31 }, { 262, 31 }, { 262, 9 },
    { 262, 9 }, { 262, 31 }, { 306, 31 }, { 306, 9 },
    { 306, 9 }, { 306, 31 }, { 350, 31 }, { 350, 9 },
    { 350, 9 }, { 350, 31 }, { 394, 31 }, { 394, 9 },
    { 394, 9 }, { 394, 31 }, { 438, 31 }, { 438, 9 },
    { 438, 9 }, { 438, 31 }, { 482, 31 }, { 482, 9 },
    { 482, 9 }, { 482, 31 }, { 526, 31 }, { 526, 9 },
    { 526, 9 }, { 526, 31 }, { 570, 31 }, { 570, 9 },
    { 570, 9 }, { 570, 31 }, { 614, 31 }, { 614, 9 },
    { 614, 9 }, { 614, 31 }, { 658, 31 }, { 658, 9 },
    { 658, 9 }, { 658, 31 }, { 702, 31 }, { 702, 9 },
    { 702, 9 }, { 702, 31 }, { 746, 31 }, { 746, 9 },
};

_mh_tex_uv_ quest_health_tick_uvs[64] = {
    { 84, 1 }, { 84, 23 }, { 99, 23 }, { 99, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
};

s16 quest_gauge_flicker_ofs_hit[52] = {
    0, 2, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

s16 quest_gauge_flicker_ofs_idle[28] = {
    0, 2, -2, 1, -1, 1, -1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/* 0x802E7FA0.  Draws the quest cockpit's three gauges for the first player: the stamina bar (0xB9E group), the
 * health bar with its tick marks (0xB99 group) and the stagger gauge with its flash (0xBAC group). */
void quest_gauge_draw(void) {
    CockpitWork* work = cockpit_work;
    _PLW* plw = cockpit_work[0].plw;
    s32 wide = ck_WideMode();
    _DRAW_RECT_2D_TEX rect;
    _mh_ivec2_ quad[6];
    _mh_tex_uv_ uvs[6];
    u32 colors[6];
    _mh_ivec2_ spot;
    _mh_ivec2_ flash_pos;
    _mh_ivec2_ corner;
    _mh_ivec2_ pos;
    s32 copy;
    _mh_tex_uv_ uv_a;
    s32 anchor_a;
    s32 anchor_b;
    _SPR_DATA_* rec;
    s16 cell;
    u32 cells;
    s32 tick_end;
    u16 scaled_max;
    u16 shown;
    u16 top;
    s16 shake;
    f32 fill;
    s8 count;

    set_blendmode(4, 5, 1);
    get_lsp_data(0xB9E, &spot);
    pos.y = 0;
    pos.x = 0;
    rec = get_lsp_data(0xB9F, NULL);
    if (wide == 0) {
        cell = rec->width;
    } else {
        cell = 0x6E;
    }
    cells = plw->field_0x37A / 150;
    corner.x = rec->pos.x + spot.x;
    corner.y = rec->pos.y + spot.y;
    draw_rect_2d_tex_init(&rect, &corner.x, (u16)(cell * cells), rec->height, rec->color, &rec->uv0.u, &rec->uv1.u);
    draw_rect_2d_tex_by_id(&rect, rec->tex_flag);
    pos.x = (s16)(cell * (cells - 1));
    pos.x += spot.x;
    pos.y += spot.y;
    if ((u32)wide == 1) {
        pos.x += 0x19;
    }
    draw_sprite_idx(0xBA0, &pos);
    uv_pair_copy(&copy, &pos);
    if (wide == 0) {
        quest_gauge_bar_draw(0xBA1, plw->field_0x378, 0, plw->field_0x37A, 0x384, &work->bar_b_0x154, &spot, 0);
    } else {
        quest_gauge_bar_draw(0xBA1, plw->field_0x378, 0, plw->field_0x37A, 0x384, &work->bar_b_0x154, &spot,
                             0x294);
    }
    if (plw->field_0x37A >= 0x384) {
        if (work->sparkle_b_frame_0x18E < 0x60) {
            work->sparkle_b_frame_0x18E++;
            anchor_a = copy;
            put_lsp_anchor_offset(0xBA4, &spot, &anchor_a);
            draw_sprite_anim_idx(0xBA5, work->sparkle_b_frame_0x18E, &spot);
        }
    } else {
        work->sparkle_b_frame_0x18E = 0;
    }
    get_lsp_data(0xB99, &spot);
    pos.y = 0;
    pos.x = 0;
    if (work->health_hit_clock_0x13A != 0) {
        pos.y = quest_gauge_shake_scale * quest_gauge_shake_table[work->health_hit_clock_0x13A];
    }
    pos.x += spot.x;
    pos.y += spot.y;
    tick_end = plw->health_max / 10;
    if (tick_end > 0xF) {
        tick_end = 0xF;
    }
    tick_end *= 4;
    drawshape_init(3, (u16)(tick_end + 4));
    drawshape_set_vertex_array(wide != 0 ? quest_health_tick_verts_wide : quest_health_tick_verts);
    drawshape_set_flat_color(-1);
    drawshape_set_texture_array(0xD, quest_health_tick_uvs);
    drawshape_set_offset_ivec2(&pos.x);
    drawshape_exec();
    if (wide == 0) {
        pos.x += (s16)(quest_health_tick_verts[tick_end].x - quest_health_tick_verts[4].x);
    } else {
        pos.x += (s16)(quest_health_tick_verts_wide[tick_end].x - quest_health_tick_verts_wide[4].x);
        pos.x += 0xA;
    }
    draw_sprite_idx(0xB9C, &pos);
    uv_pair_copy(&copy, &pos);
    pos.x = spot.x;
    if (wide == 0) {
        quest_gauge_bar_draw(0xB9D, plw->health, plw->field_0x376, plw->health_max, 0x96, &work->bar_a_0x144,
                             &pos, 0);
    } else {
        quest_gauge_bar_draw(0xB9D, plw->health, plw->field_0x376, plw->health_max, 0x96, &work->bar_a_0x144,
                             &pos, 0x294);
    }
    if (plw->health_max >= 0x96) {
        if (work->sparkle_a_frame_0x18C < 0x60) {
            work->sparkle_a_frame_0x18C++;
            anchor_b = copy;
            put_lsp_anchor_offset(0xBA2, &spot, &anchor_b);
            draw_sprite_anim_idx(0xBA3, work->sparkle_a_frame_0x18C, &spot);
        }
    } else {
        work->sparkle_a_frame_0x18C = 0;
    }
    get_lsp_data(0xBAC, &spot);
    scaled_max = (plw->field_0x380 * 0xB4) / 150;
    shown = (work->stagger_shown_0x134 * 0xB4) / 150;
    top = scaled_max - 0x11;
    if (plw->kind_0x09 != 3) {
        draw_sprite_idx(0xBB0, &spot);
        rec = get_lsp_data(0xBB1, NULL);
        corner.x = rec->pos.x + spot.x;
        corner.y = rec->pos.y + spot.y;
        draw_rect_2d_tex_init(&rect, &corner.x, rec->width, top, rec->color, &rec->uv0.u, &rec->uv1.u);
        draw_rect_2d_tex_by_id(&rect, rec->tex_flag);
        pos.x = spot.x;
        pos.y = spot.y + (top - rec->height);
        draw_sprite_idx(0xBB2, &pos);
    } else {
        if (plw->field_0x37E != 0) {
            shake = quest_gauge_flicker_ofs_hit[work->stagger_shake_idx_0x137];
        } else {
            shake = quest_gauge_flicker_ofs_idle[work->stagger_shake_idx_0x137];
        }
        pos.x = shake;
        pos.x += spot.x;
        pos.y = spot.y;
        draw_sprite_idx(0xBAD, &pos);
        rec = get_lsp_data(0xBAF, NULL);
        corner.x = rec->pos.x + pos.x;
        corner.y = rec->pos.y + pos.y;
        draw_rect_2d_tex_init(&rect, &corner.x, rec->width, rec->height, rec->color, &rec->uv0.u, &rec->uv1.u);
        rec = get_lsp_data(0xBAE, NULL);
        rect.y = rec->pos.y;
        rect.end_y += (s16)(top - rec->height);
        rect.v0 -= top;
        draw_rect_2d_tex_by_id(&rect, rec->tex_flag);
    }
    if (work->stagger_flash_0x136 != 0) {
        rec = get_lsp_data(0xBB3, NULL);
        flash_pos.x = rec->pos.x;
        flash_pos.y = (rec->pos.y + scaled_max) - shown;
        uv_a.u = rec->uv0.u;
        uv_a.v = rec->uv1.v - shown;
        if (work->stagger_shake_idx_0x137 != 0) {
            draw_rect_2d_tex_init(&rect, &flash_pos.x, rec->width, shown,
                                  work->stagger_flash_0x136 | 0xC1183600, &uv_a.u, &rec->uv1.u);
            rect.x += pos.x;
            rect.end_x += pos.x;
            draw_rect_2d_tex_by_id(&rect, rec->tex_flag);
            return;
        }
        flash_pos.x += spot.x;
        flash_pos.y += spot.y;
        fill = (2.0f * (f32)work->stagger_shown_0x134) / (f32)plw->field_0x380;
        quad[0].x = quad[2].x = flash_pos.x;
        quad[1].x = quad[3].x = flash_pos.x + rec->width;
        quad[0].y = quad[1].y = flash_pos.y;
        quad[2].y = quad[3].y = flash_pos.y + shown;
        uvs[0].u = uvs[2].u = rec->uv0.u;
        uvs[1].u = uvs[3].u = rec->uv1.u;
        uvs[0].v = uvs[1].v = uv_a.v;
        if (fill <= 1.0f) {
            colors[0] = colors[1] = work->stagger_flash_0x136 | color_lerp(0x93DDF500, 0x16D9D000, fill);
            colors[2] = colors[3] = work->stagger_flash_0x136 | 0x93DDF500;
            uvs[2].v = uvs[3].v = rec->uv1.v;
            count = 4;
        } else {
            s32 half;
            quad[4].x = flash_pos.x;
            quad[5].x = quad[1].x;
            quad[4].y = quad[5].y = quad[3].y;
            half = scaled_max / 2;
            quad[2].y = quad[3].y = quad[3].y - half;
            colors[0] = colors[1] = work->stagger_flash_0x136 | color_lerp(0x16D9D000, 0xB79200, fill - 1.0f);
            colors[2] = colors[3] = work->stagger_flash_0x136 | 0x16D9D000;
            colors[4] = colors[5] = work->stagger_flash_0x136 | 0x93DDF500;
            uvs[4].u = uvs[0].u;
            uvs[5].u = uvs[1].u;
            uvs[2].v = uvs[3].v = rec->uv1.v - half;
            uvs[4].v = uvs[5].v = rec->uv1.v;
            count = 6;
        }
        drawshape_init(5, count);
        drawshape_set_vertex_array(quad);
        drawshape_set_color_array(colors);
        drawshape_set_texture_array(rec->tex_flag, uvs);
        drawshape_exec();
    }
}

/* 0x802E884C (0x70).  "Is this quest target usable by the player?": `quest_target_usable_ck` is asked twice -
 * a global gate first (the same arguments with flags 0), then the per-target test when the gate
 * returns 1.  The result is the boolean, materialised as 1/0 by the branch. */
s32 quest_target_visible_ck(CockpitWork* work, _ENEMY_WORK* target) {
    u8 ok = quest_target_usable_ck(work, target, 0);

    if (ok == 1) {
        ok = quest_target_usable_ck(work, target, 1);
    }
    if (ok != 0) {
        return 1;
    }
    return 0;
}

/* A sprite record copied as nine words (the target copies the layout record with `lwz`/`stw` pairs, never with the
 * field-wise copy `_SPR_DATA_` assignment produces); `window` views the copy as the record.  size: 0x24 */
typedef struct SprRecordWords {
    /* +0x00 */ s32 word[9];
} SprRecordWords;

/* 0x802E88BC.  Spawns the quest window's notice for marker `idx` when the cockpit allows it: the window spot is taken
 * off the layout record, then the marker's screen position decides where the notice appears. */
void quest_notice_spawn(s32 unused_ctx, u8 idx) {
    CockpitWork* work;
    HudNotice* notice;
    SprRecordWords rec;
    f32 scale[2];
    _mh_ivec2_ spot;
    _mh_ivec2_ pos;
    _SPR_DATA_* window = (_SPR_DATA_*)&rec;

    if ((u32)screen_split_mode_ck() != 1 && Pl_motion_input_ck(0) != 1) {
        work = cockpit_work;
        if (get_option_cfg(14) != 1 && work->mark_gate_0x0CE != 1) {
            get_lsp_data(0xBE3, &spot);
            rec = *(const SprRecordWords*)get_lsp_data(0xBE4, NULL);
            window->pos.x += spot.x;
            work->window_x_0x084 = window->pos.x;
            work->window_y_0x086 = window->pos.y;
            work->window_scale_0x088 = (f32)window->width;
            if (quest_marker_screen_pos(work, &pos, scale, idx) != 0) {
                notice = hud_notice_spawn(0, 0, pos.x, pos.y, 1, 1, 0);
                if (notice != 0) {
                    hud_notice_set_flag_ptr(notice, &work->notice_flag_0x0CF);
                }
            }
        }
    }
}

/* 0x802E8A20.  `quest_notice_spawn` for one quest target: only when the target is visible and the player's motion
 * is not taking input. */
void quest_target_notice_spawn(_ENEMY_WORK* target, u8 idx) {
    CockpitWork* work;
    HudNotice* notice;
    SprRecordWords rec;
    f32 scale[2];
    _mh_ivec2_ spot;
    _mh_ivec2_ pos;
    _SPR_DATA_* window = (_SPR_DATA_*)&rec;

    if ((u32)screen_split_mode_ck() != 1) {
        work = cockpit_work;
        if (get_option_cfg(14) != 1 && work->mark_gate_0x0CE != 1 && quest_target_visible_ck(work, target) != 0) {
            get_lsp_data(0xBE3, &spot);
            rec = *(const SprRecordWords*)get_lsp_data(0xBE4, NULL);
            window->pos.x += spot.x;
            work->window_x_0x084 = window->pos.x;
            work->window_y_0x086 = window->pos.y;
            work->window_scale_0x088 = (f32)window->width;
            if (Pl_motion_input_ck(0) == 0 && quest_marker_screen_pos(work, &pos, scale, idx) != 0) {
                notice = hud_notice_spawn(0, 0, pos.x, pos.y, 1, 1, 0);
                if (notice != 0) {
                    hud_notice_set_flag_ptr(notice, &work->notice_flag_0x0CF);
                }
            }
        }
    }
}

/* 0x802E8BA4 (0xE8).  Fill the quest-target bitmask: every live record of the kind-3 move table
 * whose `quest_target_visible_ck` predicate holds raises the bit at its own index, and the two quest-view
 * fields are reset before the count.  The target's `loop_5` is a `for` over the table
 * (`get_move_work_max(3)` entries, stride 0xB18); the residual is in the file header. */
void quest_targets_update_a(CockpitWork* work) {
    _ENEMY_WORK* target;
    s32 count;

    work->target_rank_0x0D0 = quest_target_rank_get(work, work->plw);
    work->target_mask_0x0BC = 0;
    target = (_ENEMY_WORK*)get_move_work_adrs(3);
    count = get_move_work_max(3);
    while (count > 0) {
        if ((target->field_0x1C8 & 1) != 0 && quest_target_visible_ck(work, target) != 0) {
            work->target_mask_0x0BC |= (u16)(1 << target->area_no);
        }
        count--;
        target++;
    }
    work->notice_flag_0x0CF = quest_mark_notice_flag_get(quest_mark_visible_ck(work));
    quest_view_frame_task(work, 0);
}

/* 0x802E8C8C (0xF0).  `quest_targets_update_a`'s twin for the second cockpit view: the same target scan and
 * countdown, with the per-frame task argument 1, and the player state's pending flag latched into
 * its live one at the end. */
void quest_targets_update_b(CockpitWork* work) {
    CockpitState* state = &cockpit_state;
    _ENEMY_WORK* target;
    s32 count;

    work->target_rank_0x0D0 = quest_target_rank_get(work, work->plw);
    work->target_mask_0x0BC = 0;
    target = (_ENEMY_WORK*)get_move_work_adrs(3);
    count = get_move_work_max(3);
    while (count > 0) {
        if ((target->field_0x1C8 & 1) != 0 && quest_target_visible_ck(work, target) != 0) {
            work->target_mask_0x0BC |= (u16)(1 << target->area_no);
        }
        count--;
        target++;
    }
    work->notice_flag_0x0CF = quest_mark_notice_flag_get(quest_mark_visible_ck(work));
    quest_view_frame_task(work, 1);
    state->pit_trap_shown_0x059 = state->pit_trap_count_0x058;
    state->pit_trap_count_0x058 = 0;
}

/* 0x802E8D7C (0xB8).  "May this view's quest mark be shown?": the view gate first, then the option
 * configuration (`get_option_cfg(14)` in normal play, `get_arena_cfg(player chunk, 14)` in VS
 * mode), then the item menu's frame, and finally the player work record's own +0x5BC byte. */
s32 quest_mark_visible_ck(CockpitWork* work) {
    if (work->mark_gate_0x0CE != 0) {
        return 0;
    }
    if (system_w.vs_mode_0x8b0 == 0) {
        if (get_option_cfg(14) == 1) {
            return 0;
        }
    } else if (get_arena_cfg(work->plw->chunk_ofs, 14) == 1) {
        return 0;
    }
    /* `menu/menu_item.h` names the record `menu_item_frame_update` walks `MenuFrameWork`; it is the same
     * player work record this header calls `_PLW` (both views name +0x008 and +0x5BC), and the
     * target's call site passes the `CockpitWork::plw` pointer unchanged. */
    if (menu_item_frame_update((MenuFrameWork*)work->plw) == 1) {
        return 0;
    }
    return work->plw->field_0x5BC == 0;
}

/* 0x802E8E34 (0x28).  Toggle the view's +0xCE gate and report its new value. */
u8 quest_mark_toggle(CockpitWork* work) {
    if (work->mark_gate_0x0CE == 0) {
        work->mark_gate_0x0CE = 1;
    } else {
        work->mark_gate_0x0CE = 0;
    }
    return work->mark_gate_0x0CE;
}

/* 0x802E8E5C (0x8).  The view's +0xCE gate. */
u8 quest_mark_flag_get(CockpitWork* work) {
    return work->mark_gate_0x0CE;
}

/* 0x802E8E64.  Arms the quest window's marker draw for the first player: the window spot is slid to the framed
 * position (and the ring's scale trio armed) when the view gate is set, then the marker drawer runs. */
void quest_marker_arm(void) {
    CockpitWork* work = cockpit_work;
    _mh_ivec2_ pos;

    if (get_cfg(cockpit_work[0].plw->chunk_ofs, 14) == 1) {
        return;
    }
    set_blendmode(4, 5, 1);
    get_lsp_data(0xBE3, &pos);
    pos.y = 0;
    if (work->mark_gate_0x0CE == 0) {
        work->draw_id_0x0BA = 0xBE5;
        quest_marker_draw(work, 0xBE4, 0, pos);
    } else {
        pos.x += 0x20C;
        pos.y = 0xC6;
        work->ring_scale_0x0A0 = 2.3499999f;
        work->ring_extent_x_0x0A4 = 0.110815607f;
        work->ring_extent_y_0x0A8 = 0.166223407f;
        work->draw_id_0x0BA = 0xBE5;
        quest_marker_draw_faded(work, 0xBE4, 0, pos);
    }
}

/* 0x802E8F54 (0x68).  Project a world position into the screen space `screen_projection_get` describes:
 * x and z are divided by the two scales after the offsets are taken out. */
void quest_screen_project(f32* out, const f32* pos) {
    const QuestScreen* scr = screen_projection_get();

    out[0] = (pos[0] - scr->ofs_x) / scr->scale_x;
    out[1] = (pos[2] - scr->ofs_y) / scr->scale_y;
}

/* 0x802E8FBC (0xB4).  The squared screen distance of the marker: the offset is rotated by the
 * view's rotation clock and carried by the two scales of +0xAC/+0xB0, and the two squares are
 * added. */
#pragma fp_contract off

f32 quest_mark_dist_sq(CockpitWork* work, const f32* pos) {
    f32 rot[2];
    f32 dx = pos[0] - work->marker_ref_x_0x098;
    f32 dy = pos[1] - work->marker_ref_y_0x09C;
    f32 x;
    f32 y;

    quest_rot_pair(&rot[1], &rot[0], work->ring_angle_0x0B8);
    x = dx * rot[0] - dy * rot[1];
    y = dx * rot[1] + dy * rot[0];
    x *= work->ring_inv_x_0x0AC;
    y *= work->ring_inv_y_0x0B0;
    return x * x + y * y;
}
#pragma fp_contract on

/* 0x802E9070 (0x50).  Sine and cosine of the ring's rotation clock `t`: `anim_tick_angle` maps the ticks to an angle and
 * `math_sincos_idx` splits it (`0.00390625f` is the index gain). */
void quest_rot_pair(f32* a, f32* b, u16 t) {
    math_sincos_idx(a, b, 0.00390625f * anim_tick_angle(t));
}

/* 0x802E90C0 (0x3C).  Project and hand the result to `quest_mark_dist_sq`, which turns it into the
 * squared screen distance of the marker. */
f32 quest_mark_dist(CockpitWork* work, const f32* pos) {
    f32 sp8[2];

    quest_screen_project(sp8, pos);
    return quest_mark_dist_sq(work, sp8);
}

/* 0x802E90FC (0x34).  "Is the marker inside the screen?" - the projection is compared against the
 * 1.0f at `1.0f`, and the target's `cror eq,lt,eq` + `mfcr`/`rlwinm` is MWCC's `<=` in an
 * integer context. */
s32 quest_mark_on_screen_ck(CockpitWork* work, const f32* pos) {
    return quest_mark_dist(work, pos) <= 1.0f;
}

/* 0x802E9130 (0x6C).  Fades the marker colour's alpha byte by how far the target is: inside `work->ring_radius_0x0B4`
 * squared the colour stands, outside it the alpha is raised by the remaining fraction of 1.0f. */
u32 quest_mark_fade_value(CockpitWork* work, u32 color, f32 dist) {
    f32 fade = work->ring_radius_0x0B4;
    f32 near = fade * fade;
    f32 t;
    u32 res;

    if (dist < near) {
        res = color;
    } else {
        t = (1.0f - dist) / (1.0f - near);
        res = (color & 0xFFFFFF00) + (u8)(t * (f32)(u8)color);
    }
    return res;
}

/* 0x802E919C.  Blits one cell of the marker sheet (`index` picks the column and row of a ten-wide sheet) as a
 * 4096-unit rectangle centred on `pos`, with an optional second texture pass at `pos2`. */
void quest_marker_icon_draw(CockpitWork* work, const f32* pos, u8 index, u16 tex_no, u32 color, const f32* pos2,
                            u16 tex_no2, f32 scale, f32 scale2) {
    _SPR_DATA_* rec = get_lsp_data(work->draw_id_0x0BA, NULL);
    s16 cell_u = rec->uv1.u - rec->uv0.u;
    s16 cell_v = rec->uv1.v - rec->uv0.v;
    _mh_tex_uv_ lo;
    _mh_tex_uv_ hi;

    lo.u = rec->uv0.u + cell_u * (index % 10);
    lo.v = rec->uv0.v + cell_v * (index / 10);
    hi.u = lo.u + cell_u;
    hi.v = lo.v + cell_v;
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect(pos[0] - 2048.0f, pos[1] - 2048.0f, 4096.0f, 4096.0f);
    drawshape_set_flat_color(color);
    drawshape_set_texture_rect(rec->tex_flag, &lo, &hi);
    drawshape_set_tex_scale_uniform(pos, tex_no, 0.000244140625f * scale);
    if (pos2 != 0) {
        drawshape_set_tex_scale_uniform(pos2, tex_no2, scale2);
    }
    drawshape_exec();
}

/* 0x802E932C.  Projects the world position `pos` and draws the marker icon for it: at the window spot when `clip` is
 * 0, otherwise at the marker ring's position when it lies inside the screen; returns the squared screen distance. */
#pragma fp_contract off

f32 quest_marker_draw_at(CockpitWork* work, const f32* pos, u8 index, u16 tex_no, u32 color, s32 clip, f32 scale) {
    f32 proj[2];
    f32 dist;
    f32 dx;
    f32 dy;

    quest_screen_project(proj, pos);
    if (clip != 0) {
        dist = quest_mark_dist_sq(work, proj);
        if (dist <= 1.0f) {
            f32 out[2];
            dx = proj[0] - work->marker_ref_x_0x098;
            dy = proj[1] - work->marker_ref_y_0x09C;
            out[0] = work->marker_ofs_x_0x090 + 384.0f * dx;
            out[1] = work->marker_ofs_y_0x094 + 384.0f * dy;
            scale = scale / work->ring_scale_0x0A0;
            quest_marker_icon_draw(work, out, index, tex_no, quest_mark_fade_value(work, color, dist),
                                   &work->marker_ofs_x_0x090, work->ring_angle_0x0B8, scale, work->ring_scale_0x0A0);
        }
    } else {
        f32 out[2];
        out[0] = (f32)work->window_x_0x084 + work->window_scale_0x088 * proj[0];
        out[1] = (f32)work->window_y_0x086 + work->window_scale_0x088 * proj[1];
        quest_marker_icon_draw(work, out, index, tex_no, color, 0, 0, scale, 1.0f);
        dist = 0.0f;
    }
    return dist;
}
#pragma fp_contract on

f32 quest_marker_ring_outline_verts[20] = {
    0.0f, 0.0f, 0.0f, -0.75f,
    0.530250013f, -0.530250013f, 0.75f, 0.0f,
    0.530250013f, 0.530250013f, 0.0f, 0.75f,
    -0.530250013f, 0.530250013f, -0.75f, 0.0f,
    -0.530250013f, -0.530250013f, 0.0f, -0.75f,
};

f32 quest_marker_ring_fill_verts[36] = {
    0.0f, -1.0f, 0.0f, -0.75f,
    0.707000017f, -0.707000017f, 0.530250013f, -0.530250013f,
    1.0f, 0.0f, 0.75f, 0.0f,
    0.707000017f, 0.707000017f, 0.530250013f, 0.530250013f,
    0.0f, 1.0f, 0.0f, 0.75f,
    -0.707000017f, 0.707000017f, -0.530250013f, 0.530250013f,
    -1.0f, 0.0f, -0.75f, 0.0f,
    -0.707000017f, -0.707000017f, -0.530250013f, -0.530250013f,
    0.0f, -1.0f, 0.0f, -0.75f,
};

f32 quest_marker_pie_verts[52] = {
    0.0f, 0.0f, 1.0f, 0.0f,
    0.965924978f, 0.258819014f, 0.866024971f, 0.5f,
    0.707105994f, 0.707105994f, 0.5f, 0.866024971f,
    0.258819014f, 0.965924978f, -0.0f, 1.0f,
    -0.258819014f, 0.965924978f, -0.5f, 0.866024971f,
    -0.707105994f, 0.707105994f, -0.866024971f, 0.5f,
    -0.965924978f, 0.258819014f, -1.0f, -0.0f,
    -0.965924978f, -0.258819014f, -0.866024971f, -0.5f,
    -0.707105994f, -0.707105994f, -0.5f, -0.866024971f,
    -0.258819014f, -0.965924978f, 0.0f, -1.0f,
    0.258819014f, -0.965924978f, 0.5f, -0.866024971f,
    0.707105994f, -0.707105994f, 0.866024971f, -0.5f,
    0.965924978f, -0.258819014f, 1.0f, 0.0f,
};

/* 0x802E94D0.  Draws the marker ring: an outline pass in the flat colour, then the filled pass whose colours
 * alternate between the colour with and without its alpha byte. */
void quest_marker_ring_draw(const f32* offset, const f32* tex_offset, u32 color, f32 scale, f32 tex_scale) {
    u32 cols[18];
    f32 size;

    cols[0] = color & 0xFFFFFF00;
    cols[1] = color;
    cols[2] = color & 0xFFFFFF00;
    cols[3] = color;
    cols[4] = color & 0xFFFFFF00;
    cols[5] = color;
    cols[6] = color & 0xFFFFFF00;
    cols[7] = color;
    cols[8] = color & 0xFFFFFF00;
    cols[9] = color;
    cols[10] = color & 0xFFFFFF00;
    cols[11] = color;
    cols[12] = color & 0xFFFFFF00;
    cols[13] = color;
    cols[14] = color & 0xFFFFFF00;
    cols[15] = color;
    cols[16] = color & 0xFFFFFF00;
    cols[17] = color;
    size = scale * (384.0f * tex_scale);
    drawshape_init(6, 10);
    drawshape_set_vertex_array_f32(quest_marker_ring_outline_verts);
    drawshape_set_flat_color(color);
    drawshape_set_texture_array_f32(0x13, quest_marker_ring_outline_verts);
    drawshape_set_offset_uniform(offset, size);
    drawshape_set_tex_offset_uniform(tex_offset, tex_scale);
    drawshape_exec();
    drawshape_init(5, 18);
    drawshape_set_vertex_array_f32(quest_marker_ring_fill_verts);
    drawshape_set_color_array(cols);
    drawshape_set_texture_array_f32(0x13, quest_marker_ring_fill_verts);
    drawshape_set_offset_uniform(offset, size);
    drawshape_set_tex_offset_uniform(tex_offset, tex_scale);
    drawshape_exec();
}

/* 0x802E962C.  Draws the marker's pie: one flat-coloured fan, then the same 25 rim points scaled by the inverse of
 * `tex_scale` as a textured strip. */
void quest_marker_pie_draw(const f32* offset, const f32* tex_offset, const f32* extent, u32 color, u16 tex_no,
                           f32 scale, f32 tex_scale) {
    f32 strip[100];
    u32 cols[50];
    f32 tex_ext[2];
    f32 ofs[2];
    f32 inverse = 1.0f / tex_scale;
    const f32* src;
    f32* dst;
    u32* col;
    s32 n;
    u32 faded;

    tex_ext[0] = tex_scale * extent[0];
    tex_ext[1] = tex_scale * extent[1];
    ofs[0] = scale * (384.0f * tex_ext[0]);
    ofs[1] = scale * (384.0f * tex_ext[1]);
    faded = color & 0xFFFFFF00;
    drawshape_init(6, 26);
    drawshape_set_vertex_array_f32(quest_marker_pie_verts);
    drawshape_set_flat_color(color);
    drawshape_set_texture_array_f32(0x13, quest_marker_pie_verts);
    drawshape_set_offset_f32(ofs, offset);
    drawshape_set_tex_offset_f32(tex_ext, tex_no, tex_offset);
    drawshape_exec();
    src = quest_marker_pie_verts + 2;
    dst = strip;
    col = cols;
    n = 25;
    do {
        dst[0] = src[0] * inverse;
        dst[1] = src[1] * inverse;
        copyVec2(dst + 2, src);
        col[0] = faded;
        col[1] = color;
        n--;
        src += 2;
        dst += 4;
        col += 2;
    } while (n > 0);
    drawshape_init(5, 50);
    drawshape_set_vertex_array_f32(strip);
    drawshape_set_color_array(cols);
    drawshape_set_texture_array_f32(0x13, strip);
    drawshape_set_offset_f32(ofs, offset);
    drawshape_set_tex_offset_f32(tex_ext, tex_no, tex_offset);
    drawshape_exec();
}

f32 quest_pit_area_offsets[2][3] = {
    { -9500.0f, 0.0f, 5500.0f },
    { 5500.0f, 0.0f, 7700.0f },
};

/* 0x802E97A8.  The player's marker position: the actor position, moved by the pit area's entry offset when the
 * current map is the pit map and the player is not flagged. */
void quest_player_marker_pos(_PLW* plw, VEC3* out) {
    VEC3 to;
    VEC3 from;
    u32 extra_to;
    u32 extra_from;

    copyVec3(out, &plw->vec_0x03C);
    if (stage_map_kind_get(get_now_mapno()) == 9 && Pl_area_flag_get(plw->chunk_ofs) == 0) {
        VEC3_ctor(&to);
        VEC3_ctor(&from);
        stage_area_point_get(plw->area_0x16, 0, &from, &extra_from);
        stage_area_point_get(plw->area_0x16, 1, &to, &extra_to);
        out->x = out->x + (quest_pit_area_offsets[plw->area_0x16][0] + (to.x - from.x));
        out->z = out->z + (quest_pit_area_offsets[plw->area_0x16][2] + (to.z - from.z));
    }
}

/* 0x802E98A8.  Draws one player's marker: the world position is projected and the marker icon is drawn, and the
 * player's slot is appended to the mark table with its colour (faded by distance when `clip`). */
#pragma fp_contract off

void quest_player_marker_draw(CockpitWork* work, _PLW* plw, s32 clip) {
    VEC3 pos;
    VEC3 local;
    f32 proj[2];
    _mh_ivec2_ mark;
    u32 icon;
    f32 scale;
    f32 dist;
    u32 color;
    u32 slot_color;
    u16 slot_val;

    VEC3_ctor(&pos);
    VEC3_ctor(&local);
    quest_player_marker_pos(plw, &local);
    const VEC3& world = get_worldworld_pos(&local, plw->area_0x16);
    copyVec3(&pos, &world);
    icon = Pl_master_ck(plw) == 1 ? 0 : 10;
    if (clip != 0) {
        scale = 25.0f;
    } else {
        scale = 0.846153855f * (25.0f * work->window_icon_scale_0x08C);
    }
    slot_color = player_color_get(plw->chunk_ofs);
    dist = quest_marker_draw_at(work, &pos.x, icon, (u16)(f32)(u16)(0x18000 - plw->field_0x058), slot_color, clip, scale);
    if (work->slot_timer_0x0C0[plw->chunk_ofs] >= 0) {
        quest_screen_project(proj, &pos.x);
        slot_val = work->slot_timer_0x0C0[plw->chunk_ofs];
        if (work->slot_state_0x0C9[plw->chunk_ofs] == 0) {
            color = slot_color;
        } else {
            color = 0xF0F0F0FF;
        }
        if (clip != 0) {
            if (dist <= 1.0f) {
                mark.x = work->marker_ofs_x_0x090 + 384.0f * (proj[0] - work->marker_ref_x_0x098);
                mark.y = work->marker_ofs_y_0x094 + 384.0f * (proj[1] - work->marker_ref_y_0x09C);
                quest_mark_append(work, &mark, 1, quest_mark_fade_value(work, color, dist), slot_val);
            }
        } else {
            mark.x = (f32)work->window_x_0x084 + work->window_scale_0x088 * proj[0];
            mark.y = (f32)work->window_y_0x086 + work->window_scale_0x088 * proj[1];
            quest_mark_append(work, &mark, 1, color, slot_val);
        }
    }
}

#pragma fp_contract on

/* 0x802E9B48.  Draws the markers of every live non-master player, then the local player's. */
void quest_marker_draw_all_non_master(CockpitWork* work, s32 clip) {
    _PLW* plw = (_PLW*)get_move_work_adrs(2);
    s32 count = get_move_work_max(2);

    while (count > 0) {
        if (plw->slot_active != 0 && Pl_master_ck(plw) == 0) {
            quest_player_marker_draw(work, plw, clip);
        }
        count--;
        plw++;
    }
    plw = (_PLW*)get_move_work_adrs(2);
    quest_player_marker_draw(work, plw + (s8)my_player_no(), clip);
}

/* 0x802E9C0C.  Draws the markers of every live player but the cockpit's own, then the cockpit's own. */
void quest_marker_draw_others_then_self(CockpitWork* work, s32 clip) {
    _PLW* plw = (_PLW*)get_move_work_adrs(2);
    s32 count = get_move_work_max(2);

    while (count > 0) {
        if (plw->slot_active != 0 && plw != work->plw) {
            quest_player_marker_draw(work, plw, clip);
        }
        count--;
        plw++;
    }
    quest_player_marker_draw(work, work->plw, clip);
}

/* 0x802E9CB4.  Draws every live non-master player's marker, then the cockpit player's own (dimmed, unless the map
 * is kind 6) and the master's at fixed scales. */
void quest_marker_draw_with_master(CockpitWork* work, _PLW* plw) {
    VEC3 pos;
    _PLW* other;
    s32 count;
    f32 scale;
    _PL_ROOT* root;

    VEC3_ctor(&pos);
    other = (_PLW*)get_move_work_adrs(2);
    count = get_move_work_max(2);
    while (count > 0) {
        if (other->slot_active != 0 && Pl_master_ck(other) == 0) {
            quest_player_marker_draw(work, other, 0);
        }
        count--;
        other++;
    }
    if ((u8)stage_map_kind_get(get_now_mapno()) != 6) {
        const VEC3& world = get_worldworld_pos(&plw->vec_0x03C, plw->area_0x16);
        copyVec3(&pos, &world);
        scale = 25.0769234f;
        quest_marker_draw_at(work, &pos.x, 0, (u16)(f32)(u16)(0x18000 - plw->field_0x058),
                             color_mult(player_color_get(plw->chunk_ofs), 0x999999FF), 0, scale);
    }
    root = (_PL_ROOT*)get_move_work_adrs(0);
    {
        const VEC3& world = get_worldworld_pos(&root->pos_0x128, root->area_no_0xEB);
        copyVec3(&pos, &world);
        scale = 31.3461533f;
        quest_marker_draw_at(work, &pos.x, 0, (u16)(f32)(u16)(0x18000 - root->rot_y_0x134),
                             player_color_get(plw->chunk_ofs), 0, scale);
    }
}

u16 quest_view_hold_ids[4] = { 0x0BEA, 0x0BEB, 0xFFFF, 0 };
u16 quest_view_hold_ids_b[4] = { 0x0BED, 0x0BEE, 0xFFFF, 0 };

u16 quest_view_icon_ids[6] = {
    0x0BF0, 0x0BF1, 0x0BF2, 0x0BF3, 0xFFFF, 0x0000,
};

/* 0x802E9E9C.  Steps the four slot timers (and, for view 1, the view timer): a timer at or past the icon animation's
 * last frame is cleared, the slot's flag drops when the hold animation ends, and the timers ring the SEs at frames
 * 0, 30 and 60. */
void quest_view_frame_task(CockpitWork* work, u32 view) {
    s32 i = 0;
    s16 view_timer;

    do {
        if (work->slot_timer_0x0C0[i] >= 0) {
            if ((s32)sprite_ary_last_frame(quest_view_icon_ids) <= work->slot_timer_0x0C0[i]) {
                work->slot_timer_0x0C0[i] = -1;
            } else {
                u8 bit = 1 << i;

                if ((work->slot_flags_0x0C8 & bit) != 0 && (s32)sprite_ary_last_frame(quest_view_hold_ids) <= work->slot_timer_0x0C0[i]) {
                    work->slot_flags_0x0C8 &= (u8)~bit;
                }
                switch (work->slot_timer_0x0C0[i]) {
                case 0:
                    sysSE_req(0x11);
                    break;
                case 0x1E:
                    sysSE_req(0x12);
                    break;
                case 0x3C:
                    sysSE_req(0x13);
                    break;
                }
                work->slot_timer_0x0C0[i]++;
            }
        }
        i++;
    } while (i < 4);
    if (view == 1 && (view_timer = work->view_timer_0x0BE, view_timer >= 0)) {
        if ((s32)sprite_ary_last_frame(quest_view_icon_ids) <= view_timer) {
            work->view_timer_0x0BE = -1;
            return;
        }
        if ((s32)sprite_ary_last_frame(quest_view_hold_ids) <= view_timer) {
            work->view_timer_0x0BE = -1;
            return;
        }
        switch (view_timer) {
        case 0:
            sysSE_req(0x11);
            break;
        case 0x1E:
            sysSE_req(0x12);
            break;
        case 0x3C:
            sysSE_req(0x13);
            break;
        }
        work->view_timer_0x0BE++;
    }
}

/* 0x802EA050 (0x7C).  Every live player's `quest_slot_arm_a` arm must have taken for the caller's
 * action to pass; the result is the AND over the players `cockpit_state` counts. */
s32 quest_slot_arm_all_a(s8 value) {
    s32 ok = 1;
    CockpitState* state = &cockpit_state;
    s32 i;

    for (i = 0; i < state->field_0x000; i++) {
        if (quest_slot_arm_a(&cockpit_work[i], value) == 0) {
            ok = 0;
        }
    }
    return ok;
}

/* 0x802EA0CC (0x6C).  Arm one player's slot: an armed (negative) slot timer is cleared, the
 * player's bit goes into the shared flag byte and the slot's own byte is cleared. */
s32 quest_slot_arm_a(CockpitWork* work, s8 slot) {
    if (work->slot_timer_0x0C0[slot] >= 0) {
        return 0;
    }
    work->slot_timer_0x0C0[slot] = 0;
    work->slot_flags_0x0C8 |= (u8)(1 << slot);
    work->slot_state_0x0C9[slot] = 0;
    ai_npc_hold_item_arm();
    return 1;
}

/* 0x802EA138 (0x7C).  `quest_slot_arm_all_a`'s twin over `quest_slot_arm_b`. */
s32 quest_slot_arm_all_b(s8 value) {
    s32 ok = 1;
    CockpitState* state = &cockpit_state;
    s32 i;

    for (i = 0; i < state->field_0x000; i++) {
        if (quest_slot_arm_b(&cockpit_work[i], value) == 0) {
            ok = 0;
        }
    }
    return ok;
}

/* 0x802EA1B4 (0x50).  `quest_slot_arm_a` without the follow-up call: the slot's own byte is set
 * instead of cleared. */
s32 quest_slot_arm_b(CockpitWork* work, s8 slot) {
    if (work->slot_timer_0x0C0[slot] >= 0) {
        return 0;
    }
    work->slot_timer_0x0C0[slot] = 0;
    work->slot_flags_0x0C8 |= (u8)(1 << slot);
    work->slot_state_0x0C9[slot] = 1;
    return 1;
}

/* 0x802EA204 (0x74).  Append one mark to the player's 8-entry table: the texture coordinate pair
 * is copied and the three values ride behind it, and the count only advances while there is room. */
void quest_mark_append(CockpitWork* work, const _mh_ivec2_* pos, u8 tex_idx, u32 coord, u16 size) {
    QuestMark* mark;

    if (work->mark_count_0x0CD >= 8) {
        return;
    }
    mark = &work->marks_0x0D4[work->mark_count_0x0CD];
    uv_pair_copy(&mark->pos, pos);
    mark->field_0x04 = coord;
    mark->field_0x08 = size;
    mark->field_0x0A = tex_idx;
    work->mark_count_0x0CD++;
}

/* 0x802EA278.  Draws the mark's sprites: every id of the icon list (the hold list when `mode` is 0) at `pos`,
 * animated to `frame`, with the colour's alpha scaled by the sprite's own. */
void quest_mark_draw(const _mh_ivec2_* pos, u8 mode, u32 color, u16 frame) {
    const u16* ids;
    _SPR_DATA_ rec;

    if (mode == 0) {
        ids = quest_view_hold_ids;
    } else {
        ids = quest_view_icon_ids;
    }
    while (*ids != 0xFFFF) {
        spr_data_copy(&rec, get_lsp_data(*ids, 0));
        sprite_frame_apply(&rec, *ids, frame, 0);
        rec.color = (color & 0xFFFFFF00) + (u8)(((u32)(u8)rec.color * (u8)color) / 255);
        draw_sprite(rec, pos);
        ids++;
    }
}

/* 0x802EA33C.  Draws every mark of the table and empties it. */
void quest_marks_flush(CockpitWork* work) {
    QuestMark* mark;
    s32 count = work->mark_count_0x0CD;

    if (count != 0) {
        mark = work->marks_0x0D4;
        while (count > 0) {
            quest_mark_draw(&mark->pos, mark->field_0x0A, mark->field_0x04, mark->field_0x08);
            count--;
            mark++;
        }
        work->mark_count_0x0CD = 0;
    }
}

#pragma fp_contract off

/* Draws the AI NPC's marker at `world` and appends its mark-table entry. */
static inline void quest_npc_marker_draw_at(const VEC3& world, CockpitWork* work, _AINPC_W* npc, s32 clip) {
    f32 proj[2];
    _mh_ivec2_ mark;
    f32 scale;
    u32 color;
    f32 dist;

    if (clip != 0) {
        scale = 25.0f;
    } else {
        scale = 0.846153855f * (25.0f * work->window_icon_scale_0x08C);
    }
    if ((u32)ai_npc_arrived_ck(npc) == 1) {
        color = 0xF1A86DFF;
    } else {
        color = 0xC8E52EFF;
    }
    dist = quest_marker_draw_at(work, &world.x, 0xB, (u16)(0x18000 - npc->field_0x194), color, clip, scale);
    if (work->view_timer_0x0BE >= 0) {
        quest_screen_project(proj, &world.x);
        if (clip != 0) {
            if (dist <= 1.0f) {
                f32 fx = 384.0f * (proj[0] - work->marker_ref_x_0x098);
                f32 fy = 384.0f * (proj[1] - work->marker_ref_y_0x09C);

                mark.x = work->marker_ofs_x_0x090 + work->ring_scale_0x0A0 * fx;
                mark.y = work->marker_ofs_y_0x094 + work->ring_scale_0x0A0 * fy;
                quest_mark_append(work, &mark, 1, quest_mark_fade_value(work, 0xC8E52EFF, dist),
                                  work->view_timer_0x0BE);
            }
        } else {
            mark.x = (f32)work->window_x_0x084 + work->window_scale_0x088 * proj[0];
            mark.y = (f32)work->window_y_0x086 + work->window_scale_0x088 * proj[1];
            quest_mark_append(work, &mark, 1, 0xC8E52EFF, work->view_timer_0x0BE);
        }
    }
}

/* 0x802EA3B4.  Draws the AI NPC's marker (and its mark-table entry) while the player root says the NPC is present. */
void quest_npc_marker_draw(s32 clip) {
    CockpitWork* work;
    _AINPC_W* npc;

    if (((_PL_ROOT*)get_move_work_adrs(0))->npc_present_0xEF != 0) {
        work = cockpit_work;
        npc = work->ai_npc_0x048;
        quest_npc_marker_draw_at(get_worldworld_pos(&npc->vec_0x178, npc->field_0x1A4), work, npc, clip);
    }
}
#pragma fp_contract on

/* 0x802EA5F4.  The cockpit's quest-target rank bits: 4 while the AI NPC holds (or skill 0x97), 2 while skill 0x96 is
 * active - which also runs the two blink clocks - and 1 while the player's +0x460 timer runs or skill 0x31's quest has
 * under thirty seconds left. */
u8 quest_target_rank_get(CockpitWork* work, _PLW* plw) {
    u8 rank = 0;

    if (Pl_Skill_ck(plw, 0x97) == 1) {
        rank = 4;
    } else if ((u32)ai_npc_hold_ck(work->ai_npc_0x048) == 1) {
        rank = 4;
    } else if (Pl_Skill_ck(plw, 0x96) == 1) {
        rank = 2;
        if (work->quest_timer_0x188 > 0) {
            work->quest_timer_0x188--;
        }
        if (work->blink_clock_0x184 <= 0) {
            work->blink_clock_0x184 = (s32)(5.0f * (60.0f * Screen_w.frame_scale));
        } else {
            work->blink_clock_0x184--;
            if (work->blink_clock_0x184 <= 0) {
                work->blink_clock_0x184 = (s32)(5.0f * (60.0f * Screen_w.frame_scale));
                if (rand() % 3 != 0) {
                    work->quest_timer_0x188 = (s32)(30.0f * Screen_w.frame_scale);
                }
            }
        }
    }
    if ((u32)Pl_timer_0x460_ck(plw) == 1) {
        rank |= 1;
    } else if (Pl_cat_skill_ck(plw, 0x31) == 1) {
        s32 limit = quest_time_limit_get();

        if ((f32)(limit - quest_time_elapsed_get()) < 30.0f * Screen_w.frame_scale) {
            rank |= 1;
        }
    }
    return rank;
}

/* 0x802EA7D4 (0x8).  `quest_target_usable_ck` with no flags. */
u8 quest_target_usable0_ck(CockpitWork* work, _ENEMY_WORK* target) {
    return quest_target_usable_ck(work, target, 0);
}

/* 0x802EA7DC.  How a quest target may be shown: 0 hidden, 1 not markable, 2 plain, 3 highlighted - from the target's
 * live flag, the cockpit's rank bits (+0xD0) and the target's mark timer. */
u8 quest_target_usable_ck(CockpitWork* work, _ENEMY_WORK* target, s32 flags) {
    u8 rank;

    if (target->active == 0) {
        return 0;
    }
    if ((target->field_0x1C8 & 1) != 0) {
        if (flags == 0 && em_markable_ck(target) == 0) {
            return 1;
        }
        rank = work->target_rank_0x0D0;
        if ((rank & 4) != 0) {
            return 3;
        }
        if ((rank & 2) != 0) {
            if (work->quest_timer_0x188 > 0) {
                return 3;
            }
            if ((rank & 1) != 0) {
                return 3;
            }
            return em_mark_timer_ck(target) == 1 ? 3 : 0;
        }
        if ((rank & 1) != 0) {
            return 2;
        }
        if (em_mark_timer_ck(target) == 1) {
            return 2;
        }
    } else if (em_mark_timer_ck(target) != 0) {
        return 2;
    }
    return 0;
}

/* 0x802EA908.  The target's marker colour: grey for a dead action, per-kind for a live one (blinking towards light grey
 * while the skill 0xBE window is open), plain for an idle one; the alpha byte is full while the target is shown. */
#pragma fp_contract off

u32 quest_target_color(CockpitWork* work, _ENEMY_WORK* target, u8 kind) {
    _PLW* plw = work->plw;
    u32 color;
    f32 fraction;
    s32 step;
    s32 alpha;

    if (target->action == 0xB) {
        color = 0x80808000;
    } else if ((target->field_0x1C8 & 1) != 0) {
        if (kind == 3) {
            u8 variant = target->field_0x43D;

            if (variant == 1) {
                color = 0xED1A4000;
            } else if (variant == 2) {
                color = 0xFAE40800;
            } else {
                color = 0x4CE3E300;
            }
        } else {
            color = 0xEF679700;
        }
        if (Pl_Skill_ck(plw, 0xBE) == 1 && em_motion_window_ck(target) == 1) {
            step = work->frame_0x190 % 24;
            if (step < 0xC) {
                fraction = (f32)step / 12.0f;
                color = color_lerp(color, 0xE0E0E000, fraction);
            } else {
                fraction = (f32)(step - 0xC) / 12.0f;
                color = color_lerp(0xE0E0E000, color, fraction);
            }
        }
    } else {
        color = 0xE4C05B00;
    }
    if (target->visible_0x001 == 0 || target->field_0x1D4 <= 0.0f) {
        alpha = 0;
    } else {
        alpha = 0xFF;
    }
    return color | alpha;
}
#pragma fp_contract on

/* 0x802EAAB4.  Picks the target's marker icon, scale and colour; returns 1 when it is to be drawn. */
s32 quest_target_icon_get(CockpitWork* work, _ENEMY_WORK* target, u8* icon, f32* scale, u32* color) {
    u8 kind;

    *icon = 1;
    if ((target->field_0x1C8 & 1) != 0) {
        *scale = 26.0f;
    } else {
        *scale = 18.0f;
    }
    kind = quest_target_usable0_ck(work, target);
    *color = quest_target_color(work, target, kind);
    if ((u8)*color == 0) {
        return 0;
    }
    if (kind == 2) {
        return 1;
    }
    if (kind != 3) {
        return 0;
    }
    if ((target->field_0x1C8 & 1) != 0) {
        switch (target->team) {
        case 7:
        case 8:
        case 9:
        case 12:
        case 14:
            *icon = 5;
            break;
        default:
            if ((s32)target->field_0x1E2 == 1 || (s32)target->field_0x1E2 == 3) {
                *icon = 3;
            } else {
                *icon = 4;
            }
            break;
        case 15:
        case 16:
        case 18:
        case 19:
            *icon = 6;
            *scale = 30.0f;
            break;
        case 20:
            *icon = 7;
            break;
        case 25:
            if ((s32)target->field_0x1E2 == 1 || (s32)target->field_0x1E2 == 3) {
                *icon = 9;
            } else {
                *icon = 8;
            }
            break;
        }
    }
    return 1;
}

/* 0x802EAC38.  Draws one quest target's marker (when it is on screen, for the clipped view). */
#pragma fp_contract off

void quest_target_marker_draw(CockpitWork* work, _ENEMY_WORK* target, s32 clip) {
    VEC3 pos;
    u8 icon;
    f32 scale;
    u32 color;

    VEC3_ctor(&pos);
    const VEC3& world = get_worldworld_pos(&target->pos, target->area_no);
    copyVec3(&pos, &world);
    if ((clip == 0 || quest_mark_on_screen_ck(work, &pos.x) != 0) &&
        quest_target_icon_get(work, target, &icon, &scale, &color) != 0) {
        if (clip == 0) {
            scale *= 0.846153855f * work->window_icon_scale_0x08C;
        }
        quest_marker_draw_at(work, &pos.x, icon, (u16)(0x18000 - target->field_0x1C0), color, clip, scale);
    }
}
#pragma fp_contract on

/* 0x802EAD28.  Draws the markers of every quest target that may be shown as plain or highlighted. */
void quest_target_markers_draw(CockpitWork* work, s32 clip) {
    _ENEMY_WORK* target = (_ENEMY_WORK*)get_move_work_adrs(3);
    s32 count = get_move_work_max(3);

    while (count > 0) {
        if ((u8)(quest_target_usable0_ck(work, target) + 0xFE) <= 1) {
            quest_target_marker_draw(work, target, clip);
        }
        count--;
        target++;
    }
}

/* 0x802EADC4.  Draws the unclipped markers of the live quest targets that may be shown. */
void quest_target_markers_draw_live(CockpitWork* work) {
    _ENEMY_WORK* target = (_ENEMY_WORK*)get_move_work_adrs(3);
    s32 count = get_move_work_max(3);

    while (count > 0) {
        if ((target->field_0x1C8 & 1) != 0 && (u8)(quest_target_usable0_ck(work, target) + 0xFE) <= 1) {
            quest_target_marker_draw(work, target, 0);
        }
        count--;
        target++;
    }
}

/* 0x802EAE64.  Draws the hold-animation sprites of quest target `idx` at its marker position, their texture scales
 * stretched by the window scale and the marker's own. */
void quest_target_icon_anim_draw(CockpitWork* work, u8 idx, u16 frame) {
    _SPR_DATA_ rec;
    f32 scale[2];
    _mh_ivec2_ pos;
    f32 factor;
    const u16* ids;

    if (quest_marker_screen_pos(work, &pos, scale, idx) != 0) {
        factor = 0.00260416674f * work->window_scale_0x088;
        scale[0] *= factor;
        scale[1] *= factor;
        ids = quest_view_hold_ids_b;
        while (*ids != 0xFFFF) {
            sprite_frame_apply(&rec, *ids, frame, 0);
            rec.u_scale = (u16)((f32)rec.u_scale * scale[0]);
            rec.v_scale = (u16)((f32)rec.v_scale * scale[1]);
            draw_sprite(rec, &pos);
            ids++;
        }
    }
}

/* 0x802EAF88.  Runs the hold animation for every quest target of the bitmask. */
void quest_target_icons_anim_draw(void) {
    CockpitWork* work = cockpit_work;
    u16 frame;
    s32 i;

    if (work->target_mask_0x0BC != 0) {
        frame = work->frame_0x190 % (sprite_ary_last_frame(quest_view_hold_ids_b) + 1);
        i = 0;
        do {
            if ((work->target_mask_0x0BC & (1 << i)) != 0) {
                quest_target_icon_anim_draw(work, i, frame);
            }
            i++;
        } while (i < 13);
    }
}

/* 0x802EB034.  Draws one square icon of sprite `id` at the layout record's position: in the window frame when
 * `clip` is 0, otherwise on the marker ring (rotated by the ring's clock, and dropped when it lies off the ring). */
#pragma fp_contract off

void quest_icon_rect_draw(CockpitWork* work, u16 id, u16 size, u32 color, s32 clip) {
    _SPR_DATA_* rec = get_lsp_data(id, NULL);
    f32 pos[2];
    f32 x;
    f32 y;
    f32 rot[2];
    f32 dx;
    f32 dy;
    f32 u;
    f32 v;
    f32 scaled_u;
    f32 scaled_v;
    f32 dist;
    f32 side;
    f32 half;
    f32 ring;

    pos[0] = 0.00260416674f * (f32)rec->pos.x;
    pos[1] = 0.00260416674f * (f32)rec->pos.y;
    if (clip != 0) {
        dx = pos[0] - work->marker_ref_x_0x098;
        dy = pos[1] - work->marker_ref_y_0x09C;
        quest_rot_pair(&rot[1], &rot[0], work->ring_angle_0x0B8);
        u = dx * rot[0] - dy * rot[1];
        v = dx * rot[1] + dy * rot[0];
        scaled_u = u * work->ring_inv_x_0x0AC;
        scaled_v = v * work->ring_inv_y_0x0B0;
        dist = scaled_u * scaled_u + scaled_v * scaled_v;
        if (dist > 1.0f) {
            return;
        }
        ring = 384.0f * work->ring_scale_0x0A0;
        x = work->marker_ofs_x_0x090 + ring * u;
        y = work->marker_ofs_y_0x094 + ring * v;
        color = quest_mark_fade_value(work, color, dist);
    } else {
        x = (f32)work->window_x_0x084 + pos[0] * work->window_scale_0x088;
        y = (f32)work->window_y_0x086 + pos[1] * work->window_scale_0x088;
    }
    side = (f32)size;
    half = 0.5f * side;
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect(x - half, y - half, side, side);
    drawshape_set_flat_color(color);
    drawshape_set_texture_rect(rec->tex_flag, &rec->uv0, &rec->uv1);
    drawshape_exec();
}
#pragma fp_contract on

/* 0x802EB268.  Draws the icons of the current area's list at the full size - the selected quest's list table replaces
 * the map's while a quest is selected. */
void quest_map_area_icons_draw(CockpitWork* work) {
    u16** lists;
    u16* ids;

    lists = quest_area_icon_lists_by_map[get_now_mapno()];
    if (lists != NULL) {
        if (quest_select_ready_ck() != 0) {
            switch ((u16)quest_id_get()) {
            case 1:
                lists = quest_area_icon_lists_by_quest[0];
                break;
            case 2:
                lists = quest_area_icon_lists_by_quest[1];
                break;
            case 3:
                lists = quest_area_icon_lists_by_quest[2];
                break;
            }
        }
        ids = lists[get_now_areano()];
        if (ids != NULL) {
            for (;;) {
                if (*ids == 0xFFFF) {
                    break;
                }
                quest_icon_rect_draw(work, *ids, 0x18, 0xACACACAA, 1);
                ids++;
            }
        }
    }
}

/* 0x802EB35C.  The icon id list of map `mapno`: the selected quest's list replaces the map's while a quest is selected. */
const u16* quest_map_icon_list_get(u8 mapno) {
    u16* list = quest_map_icon_list_by_map[mapno];

    if (list == NULL) {
        return NULL;
    }
    if (quest_select_ready_ck() != 0) {
        switch ((u16)quest_id_get()) {
        case 1:
            list = quest_map_icon_list_by_quest[0];
            break;
        case 2:
            list = quest_map_icon_list_by_quest[1];
            break;
        case 3:
            list = quest_map_icon_list_by_quest[2];
            break;
        }
    }
    return list;
}

/* 0x802EB400.  Draws the current map's icon id list (the pit map's area list on map kind 9) at `size`, skipping the
 * first entry on the second level of a kind 6 map, then the area's own icons. */
void quest_map_icons_draw(CockpitWork* work, u16 size) {
    u8 mapno;
    const u16* ids;
    u32 skip_first;
    s32 i;
    u32 count;

    mapno = get_now_mapno();
    if ((u8)stage_map_kind_get(get_now_mapno()) != 9) {
        ids = quest_map_icon_list_get(mapno);
    } else {
        ids = quest_pit_area_icon_lists[get_now_areano()];
    }
    if (ids != NULL) {
        skip_first = 0;
        if ((u8)stage_map_kind_get(get_now_mapno()) == 6 && (u8)get_now_areano() != 0) {
            skip_first = 1;
        }
        count = stage_map_area_count_get(mapno);
        for (i = 0; i < (u8)count; i++, ids++) {
            if (*ids == 0xFFFF) {
                break;
            }
            if (*ids != 0) {
                if (i != 0 || skip_first != 1) {
                    quest_icon_rect_draw(work, *ids, size, 0xF0F0F0AA, 1);
                }
            }
        }
        quest_map_area_icons_draw(work);
    }
}

/* 0x802EB51C.  On the pit map, draws the area's two icons at `size`. */
void quest_pit_area_icons_draw(CockpitWork* work, u16 size) {
    const u16* ids;

    if ((u8)stage_map_kind_get(get_now_mapno()) == 9) {
        ids = quest_pit_area_icon_lists[get_now_areano()];
        quest_icon_rect_draw(work, ids[0], size, 0xF0F0F0AA, 1);
        quest_icon_rect_draw(work, ids[1], size, 0xF0F0F0AA, 1);
    }
}

/* 0x802EB5C4.  Draws the map's icon id list at `size` with the marked ones (the quest-target bitmask) highlighted. */
void quest_map_icon_marks_draw(CockpitWork* work, u16 size, s32 clip) {
    u8 mapno;
    const u16* ids;
    s32 skip_first;
    s32 i;
    u32 count;
    u32 color;

    mapno = get_now_mapno();
    if ((u8)stage_map_kind_get(get_now_mapno()) != 9) {
        ids = quest_map_icon_list_get(mapno);
    } else {
        ids = quest_pit_area_icon_lists[get_now_areano()];
    }
    if (ids != NULL) {
        skip_first = 0;
        if ((u8)stage_map_kind_get(get_now_mapno()) == 6 && (u8)get_now_areano() != 0) {
            skip_first = 1;
        }
        count = stage_map_area_count_get(mapno);
        for (i = 0; i < (u8)count; i++, ids++) {
            if (*ids == 0xFFFF) {
                break;
            }
            if (*ids != 0) {
                if (i == 0) {
                    if (skip_first == 0) {
                        quest_icon_rect_draw(work, *ids, size, 0xF0F0F0C8, 0);
                    }
                } else {
                    color = 0xF0F0F0C8;
                    if ((work->target_mask_0x0BC & (1 << i)) != 0) {
                        color = 0xF1CC3CAA;
                    }
                    quest_icon_rect_draw(work, *ids, size, color, clip);
                }
            }
        }
    }
}

/* 0x802EB714.  On the pit map, draws the area's two icons at `size`, the second highlighted. */
void quest_pit_area_icon_marks_draw(CockpitWork* work, u16 size, s32 clip) {
    const u16* ids;

    if ((u8)stage_map_kind_get(get_now_mapno()) == 9) {
        ids = quest_pit_area_icon_lists[get_now_areano()];
        quest_icon_rect_draw(work, ids[0], size, 0xF0F0F0C8, 0);
        quest_icon_rect_draw(work, ids[1], size, 0xF1CC3CAA, clip);
    }
}

/* 0x802EB7C4.  Draws the map's icon id list at `size`, the icon of the player's own area in the colour of sprite 0x11D2
 * frame `frame`. */
void quest_map_icon_sprites_draw(CockpitWork* work, u16 size, u8 frame, s32 clip) {
    u8 mapno;
    const u16* ids;
    _PL_ROOT* root;
    _SPR_DATA_ rec;
    u32 own_color;
    u8 area;
    s32 skip_first;
    s32 i;
    u32 count;
    u32 color;

    mapno = get_now_mapno();
    ids = quest_map_icon_list_get(mapno);
    if (ids != NULL) {
        root = (_PL_ROOT*)get_move_work_adrs(0);
        area = root->area_no_0xEB;
        sprite_frame_apply(&rec, 0x11D2, frame, 0);
        own_color = rec.color;
        skip_first = 0;
        if ((u8)stage_map_kind_get(get_now_mapno()) == 6 && area != 0) {
            skip_first = 1;
        }
        count = stage_map_area_count_get(mapno);
        for (i = 0; i < (u8)count; i++, ids++) {
            if (*ids == 0xFFFF) {
                break;
            }
            if (*ids != 0) {
                color = 0xF0F0F0C8;
                if (i == area) {
                    color = own_color;
                }
                if (i == 0) {
                    if (skip_first == 0) {
                        quest_icon_rect_draw(work, *ids, size, color, clip);
                    }
                } else {
                    quest_icon_rect_draw(work, *ids, size, color, clip);
                }
            }
        }
    }
}

/* 0x802EB8E8.  The window position and texture scales of the current area's marker sprite of quest target `idx`;
 * 0 (with everything zeroed) when the map has none. */
#pragma fp_contract off

s32 quest_marker_screen_pos(CockpitWork* work, _mh_ivec2_* pos, f32* scale, u8 idx) {
    u16* ids = quest_area_sprite_list_by_map[get_now_mapno()];
    _SPR_DATA_* rec;
    f32 v[2];

    if (ids == NULL) {
        pos->y = 0;
        pos->x = 0;
        scale[1] = 0.0f;
        scale[0] = 0.0f;
        return 0;
    }
    rec = get_lsp_data(ids[idx], NULL);
    v[0] = 0.00260416674f * (f32)rec->pos.x;
    v[1] = 0.00260416674f * (f32)rec->pos.y;
    pos->x = (s16)((f32)work->window_x_0x084 + v[0] * work->window_scale_0x088);
    pos->y = (s16)((f32)work->window_y_0x086 + v[1] * work->window_scale_0x088);
    scale[0] = (f32)rec->u_scale / 100.0f;
    scale[1] = (f32)rec->v_scale / 100.0f;
    return 1;
}
#pragma fp_contract on

/* 0x802EBA74.  Draws one pit trap's marker: the clipped view rotates with the marker ring's clock. */
#pragma fp_contract off

f32 quest_pit_trap_marker_draw(CockpitWork* work, PitTrap* trap, s32 clip) {
    f32 scale;
    u32 color;
    s32 tex_no;

    if (clip != 0) {
        scale = 18.0f;
    } else {
        scale = 0.846153855f * (18.0f * work->window_icon_scale_0x08C);
    }
    if (trap->kind_0x0C == 0) {
        color = 0x53E37EFF;
    } else {
        color = 0xE069FFFF;
    }
    if (clip != 0) {
        tex_no = 0x10000 - work->ring_angle_0x0B8;
    } else {
        tex_no = 0;
    }
    return quest_marker_draw_at(work, &trap->pos.x, 0xE, tex_no, color, clip, scale);
}
#pragma fp_contract on

/* 0x802EBAE0.  Draws every registered pit trap's marker. */
void quest_pit_traps_draw(CockpitWork* work, s32 clip) {
    CockpitState* state = &cockpit_state;
    PitTrap* trap = state->pit_traps_0x018;
    s32 count = state->pit_trap_shown_0x059;

    while (count > 0) {
        quest_pit_trap_marker_draw(work, trap, clip);
        count--;
        trap++;
    }
}

/* 0x802EBB58.  Registers a pit trap (position and kind) for this frame's markers, up to four. */
void PitTrapSet(const VEC3* pos, u8 kind) {
    CockpitState* state = &cockpit_state;
    PitTrap* trap;

    if (state->pit_trap_count_0x058 < 4) {
        trap = &state->pit_traps_0x018[state->pit_trap_count_0x058];
        copyVec3(&trap->pos, pos);
        trap->kind_0x0C = kind;
        state->pit_trap_count_0x058++;
    }
}

/* 0x802EBBD0.  Draws the quest cockpit's marker ring and everything on it: the ring's pie at the window spot (rotated
 * by the camera when the option is on), then the map icons, the quest targets, the players and the pit traps. */
void quest_marker_draw_faded(CockpitWork* work, u16 id, u32 mode, _mh_ivec2_ pos) {
    _PLW* plw = work->plw;
    u32 angle_on;
    u16 angle;
    _SPR_DATA_* rec;
    VEC3 local;
    f32 proj[2];
    f32 scale;
    u32 prev_player;

    work->window_x_0x084 = pos.x;
    work->window_y_0x086 = pos.y;
    angle_on = 0;
    if (get_cfg(plw->chunk_ofs, 0xF) == 1) {
        angle_on = 1;
    }
    if (angle_on == 1) {
        if (mode == 1) {
            prev_player = my_player_no();
            my_player_no_set(plw->chunk_ofs);
            angle = camera_angle_y_get();
            my_player_no_set(prev_player);
        } else {
            angle = camera_angle_y_get();
        }
    } else {
        angle = 0;
    }
    rec = get_lsp_data(id, NULL);
    VEC3_ctor(&local);
    quest_player_marker_pos(plw, &local);
    const VEC3& world = get_worldworld_pos(&local, plw->area_0x16);
    quest_screen_project(proj, &world.x);
    if (stage_map_kind_get(get_now_mapno()) == 6) {
        if (get_now_areano() == 1) {
            scale = 0.800000012f;
        } else {
            work->ring_extent_x_0x0A4 = 0.0831117034f;
            work->ring_extent_y_0x0A8 = 0.124667555f;
            scale = 0.649999976f;
        }
    } else {
        scale = 0.800000012f;
    }
    work->marker_ofs_x_0x090 = (f32)work->window_x_0x084;
    work->marker_ofs_y_0x094 = (f32)work->window_y_0x086;
    drawshape_copy_vec2(&work->marker_ref_x_0x098, proj);
    work->ring_inv_x_0x0AC = 1.0f / work->ring_extent_x_0x0A4;
    work->ring_inv_y_0x0B0 = 1.0f / work->ring_extent_y_0x0A8;
    work->ring_angle_0x0B8 = angle;
    work->ring_radius_0x0B4 = scale;
    quest_marker_pie_draw(&work->marker_ofs_x_0x090, &work->marker_ref_x_0x098, &work->ring_extent_x_0x0A4, rec->color,
                          0x10000 - angle, work->ring_scale_0x0A0, scale);
    if (mode == 0) {
        quest_map_icons_draw(work, 0x28);
    } else {
        quest_pit_area_icons_draw(work, 0x28);
    }
    quest_target_markers_draw(work, 1);
    if (mode == 0) {
        quest_npc_marker_draw(1);
    }
    if (mode == 0) {
        quest_marker_draw_all_non_master(work, 1);
    } else {
        quest_marker_draw_others_then_self(work, 1);
    }
    quest_pit_traps_draw(work, 1);
}

/* 0x802EBE2C.  Whether the quest marker may show for the player: never on map kind 6, always while the move work
 * state holds or skill 0x86 is on, never with skill 0x87, else by the item timer of id 0x2A. */
u32 quest_mark_allowed_ck(_PLW* plw) {
    if (stage_map_kind_get(get_now_mapno()) == 6) {
        return 0;
    }
    if (move_work_state_ck() != 0) {
        return 1;
    }
    if (Pl_Skill_ck(plw, 0x86) == 1) {
        return 1;
    }
    if (Pl_Skill_ck(plw, 0x87) == 1) {
        return 0;
    }
    return Pl_item_timer_get(plw, 0x2A) != 0;
}

/* 0x802EBED8 (0x328): Draws the quest cockpit's marker window unfaded: the window record at `pos`, the map icons, and -
 * when the marker may not show - the ring around the player's projected position, then the targets, players and traps. */
#pragma fp_contract off

void quest_marker_draw(CockpitWork* work, u16 id, u32 mode, _mh_ivec2_ pos) {
    union {
        SprRecordWords words;
        _SPR_DATA_ spr;
    } rec;
    VEC3 local;
    f32 spot[2];
    f32 proj[2];
    _PLW* plw = work->plw;
    f32 tex_scale;
    f32 scale;
    f32 inv;

    rec.words = *(const SprRecordWords*)get_lsp_data(id, NULL);
    rec.spr.pos.x += pos.x;
    rec.spr.pos.y += pos.y;
    work->window_x_0x084 = rec.spr.pos.x;
    work->window_y_0x086 = rec.spr.pos.y;
    work->window_scale_0x088 = (f32)rec.spr.width;
    work->window_icon_scale_0x08C = 1.0f;
    if (mode == 0) {
        quest_target_icons_anim_draw();
    }
    if (quest_mark_allowed_ck(plw) == 1) {
        draw_sprite(rec.spr, NULL);
        if (mode == 0) {
            quest_map_icon_marks_draw(work, 0x12, 0);
        } else {
            quest_pit_area_icon_marks_draw(work, 0x12, 0);
        }
    } else {
        VEC3_ctor(&local);
        quest_player_marker_pos(plw, &local);
        const VEC3& world = get_worldworld_pos(&local, plw->area_0x16);
        quest_screen_project(proj, &world.x);
        spot[0] = (f32)work->window_x_0x084 + work->window_scale_0x088 * proj[0];
        spot[1] = (f32)work->window_y_0x086 + work->window_scale_0x088 * proj[1];
        if (stage_map_kind_get(get_now_mapno()) == 6) {
            if (get_now_areano() == 1) {
                tex_scale = 0.219999999f;
            } else {
                tex_scale = 0.119999997f;
            }
        } else {
            tex_scale = 0.219999999f;
        }
        scale = 0.00260416674f * work->window_scale_0x088;
        quest_marker_ring_draw(spot, proj, rec.spr.color, scale, tex_scale);
        work->marker_ofs_x_0x090 = spot[0];
        work->marker_ofs_y_0x094 = spot[1];
        drawshape_copy_vec2(&work->marker_ref_x_0x098, proj);
        work->ring_scale_0x0A0 = scale;
        work->ring_extent_x_0x0A4 = tex_scale;
        work->ring_extent_y_0x0A8 = tex_scale;
        inv = 1.0f / tex_scale;
        work->ring_inv_x_0x0AC = inv;
        work->ring_inv_y_0x0B0 = inv;
        work->ring_angle_0x0B8 = 0;
        work->ring_radius_0x0B4 = 0.75f;
        if (mode == 0) {
            quest_map_icon_marks_draw(work, 0x12, 1);
        } else {
            quest_pit_area_icon_marks_draw(work, 0x12, 1);
        }
    }
    quest_target_markers_draw(work, 0);
    if (mode == 0) {
        quest_npc_marker_draw(0);
    }
    if (mode == 0) {
        quest_marker_draw_all_non_master(work, 0);
    } else {
        quest_marker_draw_others_then_self(work, 0);
    }
    quest_pit_traps_draw(work, 0);
}

/* 0x802EC200 (0x2F0): Draws the quest-window variant of the marker window for the first cockpit: the window frame,
 * the map icon sprites (the ring around the quest move's position when the marker may not show), the live targets,
 * the players and the pit traps, then raises the loading display. */
void quest_marker_draw_sub(void) {
    union {
        SprRecordWords words;
        _SPR_DATA_ spr;
    } rec;
    f32 spot[2];
    f32 proj[2];
    _mh_ivec2_ base;
    CockpitWork* work = cockpit_work;
    _PLW* plw = work->plw;
    SysQuestMove* quest;
    f32 tex_scale;
    f32 scale;
    f32 inv;

    set_blendmode(4, 5, 1);
    get_lsp_data(0x11D3, &base);
    draw_sprite_idx(0x11D4, &base);
    get_lsp_data(0x11D0, &base);
    spr_data_copy(&rec.spr, get_lsp_data(0x11D1, NULL));
    work->window_x_0x084 = rec.spr.pos.x + base.x;
    work->window_y_0x086 = rec.spr.pos.y + base.y;
    work->window_scale_0x088 = (f32)rec.spr.width;
    work->window_icon_scale_0x08C = 1.48181820f;
    work->draw_id_0x0BA = 0xBE5;
    quest_target_icons_anim_draw();
    if (quest_mark_allowed_ck(plw) == 1) {
        draw_sprite(rec.spr, &base);
        quest_map_icon_sprites_draw(work, 0x1A, work->frame_0x190 % 14, 0);
    } else {
        quest = (SysQuestMove*)get_move_work_adrs(0);
        const VEC3& world = get_worldworld_pos(&quest->pos_0x128, quest->area_0x0EB);
        quest_screen_project(proj, &world.x);
        spot[0] = (f32)work->window_x_0x084 + work->window_scale_0x088 * proj[0];
        spot[1] = (f32)work->window_y_0x086 + work->window_scale_0x088 * proj[1];
        if (stage_map_kind_get(get_now_mapno()) == 6) {
            tex_scale = 0.119999997f;
        } else {
            tex_scale = 0.219999999f;
        }
        scale = 0.00260416674f * work->window_scale_0x088;
        quest_marker_ring_draw(spot, proj, rec.spr.color, scale, tex_scale);
        work->marker_ofs_x_0x090 = spot[0];
        work->marker_ofs_y_0x094 = spot[1];
        drawshape_copy_vec2(&work->marker_ref_x_0x098, proj);
        work->ring_scale_0x0A0 = scale;
        work->ring_extent_x_0x0A4 = tex_scale;
        work->ring_extent_y_0x0A8 = tex_scale;
        inv = 1.0f / tex_scale;
        work->ring_inv_x_0x0AC = inv;
        work->ring_inv_y_0x0B0 = inv;
        work->ring_angle_0x0B8 = 0;
        work->ring_radius_0x0B4 = 0.75f;
        quest_map_icon_sprites_draw(work, 0x1A, work->frame_0x190 % 14, 1);
    }
    quest_target_markers_draw_live(work);
    quest_npc_marker_draw(0);
    quest_marker_draw_with_master(work, work->plw);
    quest_pit_traps_draw(work, 0);
    loading_disp_set(1, 6);
}
#pragma fp_contract on

/* 0x802EC4F0 (0x1D4): Steps the cockpit's item and gauge icon animations from the player work: the gauge value and
 * level on the first item page, the item icon frame and the gauge icon frame (with its sound cue) on the second. */
void cockpit_gauge_icons_update(CockpitWork* work) {
    _PLW* plw = work->plw;
    u32 cam;
    u8 icon;

    if (cockpit_state.field_0x000 == 2) {
        cam = camera_mode1_ck_player(plw->chunk_ofs);
    } else {
        cam = camera_mode1_ck();
    }
    if (pl_item_page2_ck(plw) == 0) {
        switch (plw->field_0x002) {
        case 8:
            if (work->gauge_value_0x168 >= plw->field_0x36C) {
                work->gauge_value_0x168 = plw->field_0x36C;
            } else {
                work->gauge_value_0x168++;
            }
            if (cockpit_gauge_low_ck(plw) != 0) {
                if (work->gauge_level_0x16A < 5) {
                    work->gauge_level_0x16A++;
                }
            } else if (work->gauge_level_0x16A != 0) {
                work->gauge_level_0x16A--;
            }
            break;
        }
    } else {
        icon = pl_item_icon_kind_get(pl_item_slot_item_get(plw, plw->item_slot_sel));
        if (work->item_icon_0x164 != icon) {
            work->item_icon_0x164 = icon;
            work->item_frame_0x166 = 0;
        } else if (work->item_frame_0x166 != 0) {
            if (sprite_ary_last_frame(cockpit_ammo_fire_ids) <= work->item_frame_0x166) {
                work->item_frame_0x166 = 0;
            } else {
                work->item_frame_0x166++;
            }
        } else if (work->item_count_prev_0x165 > plw->field_0x269) {
            work->item_frame_0x166++;
        }
        work->item_count_prev_0x165 = plw->field_0x269;
        if (pl_gauge_icon_ck(plw) != 0) {
            if (sprite_ary_last_frame(cockpit_scope_anim_ids) > work->gauge_frame_0x167) {
                work->gauge_frame_0x167++;
                if (cam != 0 && work->gauge_frame_0x167 == 7) {
                    pl_charge_se_play(plw);
                }
            }
        } else {
            work->gauge_frame_0x167 = 0;
        }
    }
}

/* 0x802EC700 (0x1AC): Draws the first cockpit's weapon HUD: the aim view, the lock-on and camera markers and the
 * gauge icons on the second item page; the aim view, the camera marker and the weapon gauge on the first. */
void cockpit_weapon_hud_draw(void) {
    _mh_ivec2_ pos;
    CockpitWork* work = cockpit_work;
    _PLW* plw = work->plw;

    set_blendmode(4, 5, 1);
    if (pl_item_page2_ck(plw) == 1) {
        if (camera_mode1_ck() == 1) {
            cockpit_aim_view_draw();
            return;
        }
        if (camera_work_ck() == 0) {
            if ((u32)pl_target_lock_ck(plw) == 1 && pl_aim_act_ck(plw) == 1) {
                cockpit_eye_marker_draw(plw);
            }
            if (pl_camera_reset_ck(plw, 1) == 1) {
                cockpit_shell_marker_draw(plw);
            }
        }
        if (cockpit_hud_hidden_self_ck(plw) == 1) {
            get_lsp_data(ck_WideMode() != 0 ? 0xCC7 : 0xC75, &pos);
            cockpit_ammo_gauge_draw(plw, pos.x, pos.y, 0);
        }
    } else {
        if (camera_mode1_ck() == 1) {
            cockpit_aim_view_draw();
        }
        if (camera_work_ck() == 0 && pl_camera_reset_ck(plw, 1) == 1) {
            cockpit_shell_marker_draw(plw);
        }
        if (cockpit_hud_hidden_self_ck(plw) == 1) {
            get_lsp_data(0xBA6, &pos);
            cockpit_weapon_frame_draw(work, &pos, 0);
            switch (plw->field_0x002) {
            case 7:
                cockpit_stagger_gauge_draw(work, 0, 0);
                break;
            case 8:
                get_lsp_data(0xC13, &pos);
                cockpit_charge_gauge_draw(work, &pos, 0);
                break;
            }
        }
    }
}

/* 0x802EC8AC (0x174): Draws the aim view: the scope frame and its gauge animation while the camera aims, the barrier
 * gauge when no barrier holds, and the zoom ring scaled by the zoom ratio. */
void cockpit_aim_view_draw(void) {
    _mh_ivec2_ pos;
    _mh_ivec2_ gauge;
    f32 zoom;
    u8 aiming;
    u8 zooming;
    CockpitWork* work = cockpit_work;
    _PLW* plw = work->plw;
    s32 wide = ck_WideMode();

    camera_aim_zoom_get(plw, &aiming, &zooming, &zoom);
    get_lsp_data(wide != 0 ? 0xCA1 : 0xC48, &pos);
    if (aiming != 0) {
        draw_sprite_ary(wide != 0 ? cockpit_scope_frame_ids_wide : cockpit_scope_frame_ids, &pos);
        draw_sprite_anim_ary(wide != 0 ? cockpit_scope_anim_ids_wide : cockpit_scope_anim_ids, work->gauge_frame_0x167, &pos);
        if (Pl_bari_ck(plw, 1) == 0) {
            get_lsp_data(wide != 0 ? 0xCC6 : 0xC6E, &gauge);
            cockpit_ammo_gauge_draw(plw, gauge.x, gauge.y, 1);
        }
    }
    if (zooming != 0) {
        zoom *= (f32)sprite_anim_last_frame(0xC5C);
        draw_sprite_anim_ary(wide != 0 ? cockpit_zoom_ring_ids_wide : cockpit_zoom_ring_ids, (u16)zoom, &pos);
    }
}

/* The status icon cells per cursor index (players' two rows), the act-field mask tables the player cursors walk
 * (kind 0 and 1, the +0x3D8 and +0x3DC words), and the AI NPC's two mask tables - 0-terminated. */
u8 cockpit_act_icon_cells[20] = {
    0x04, 0x05, 0x3C, 0x02, 0x03, 0x3D, 0x0E, 0x0F, 0x10, 0x32,
    0x33, 0x34, 0x35, 0x36, 0x0C, 0x41, 0x42, 0x43, 0x44, 0x00,
};
u8 cockpit_skill_icon_cells[20] = {
    0x07, 0x06, 0x09, 0x08, 0x0B, 0x37, 0x38, 0x3B, 0x11, 0x12,
    0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x00, 0x00,
};
u32 cockpit_act_mask_a0[10] = {
    0x00000010, 0x00000020, 0x00010000, 0x00000040, 0x00000080,
    0x00002000, 0x00000800, 0x00000400, 0x00004000, 0x00000000,
};
u32 cockpit_act_mask_b0[11] = {
    0x00008000, 0x00010000, 0x00040000, 0x00020000, 0x00080000, 0x00000400,
    0x00000800, 0x00002000, 0x00001000, 0x00004000, 0x00000000,
};
u32 cockpit_act_mask_a1[9] = {
    0x00000008, 0x00000004, 0x00000002, 0x00000001, 0x00000100,
    0x00000200, 0x00020000, 0x00040000, 0x00000000,
};
u32 cockpit_act_mask_b1[12] = {
    0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000040, 0x00000080,
    0x00000010, 0x00000020, 0x00000100, 0x00000200, 0x00000000, 0x00000000,
};
u32 cockpit_npc_mask0[4] = {
    0x00000010, 0x00000040, 0x00002000, 0x00000000,
};
u32 cockpit_npc_mask1[5] = {
    0x00000008, 0x00000004, 0x00000002, 0x00000001, 0x00000000,
};

/* The ammo gauge's three state icons (empty, reloading, idle) for the plain and the aim view. */
u16 cockpit_ammo_icons[3] = { 0x0C76, 0x0C77, 0x0C78 };
u16 cockpit_ammo_icons_aim[3] = { 0x0C71, 0x0C72, 0x0C73 };

/* 0x802ECA20 (0x208): Draws the ammo gauge at (`x`, `y`): one cell per loaded round (the firing cell animated), the
 * condition marker, then the gauge's state icon; with no magazine only the empty icon while the weapon is out. */
void cockpit_ammo_gauge_draw(_PLW* plw, s16 x, s16 y, u8 aim) {
    _mh_ivec2_ pos;
    _mh_ivec2_ start;
    const u16* icons;
    s16 step;
    CockpitWork* work = cockpit_work;
    u8 capacity;
    u8 loaded;
    s32 i;

    icons = aim == 0 ? cockpit_ammo_icons : cockpit_ammo_icons_aim;
    pos.x = x;
    pos.y = y;
    if (plw->magazine_0x270 == 0) {
        if (plw->field_0x018 == 1 && pl_item_slot_next_get(plw, plw->item_slot_sel, 1) != 0xFF) {
            draw_sprite_anim_idx(icons[2], work->frame_0x190, &pos);
        }
    } else {
        capacity = plw->ammo_capacity_0x26A;
        loaded = plw->field_0x269;
        step = get_lsp_data(0xC6F, NULL)->width;
        if (aim != 0) {
            pos.x = (s32)(320.0f * hud_screen_width_get()) - (capacity * step) / 2;
        }
        uv_pair_copy(&start, &pos);
        for (i = 0; i < capacity; i++) {
            if (work->item_frame_0x166 != 0 && i == loaded) {
                draw_sprite_anim_ary(cockpit_ammo_fire_ids, work->item_frame_0x166, &pos);
            } else if (i < loaded) {
                draw_sprite_anim_idx(0xC6F, 0, &pos);
            } else {
                draw_sprite_anim_idx(0xC70, 100, &pos);
            }
            pos.x += step;
        }
        if (Pl_dm_condition_ck(plw, 0x300) != 0) {
            draw_sprite_anim_idx(0xC74, work->frame_0x190, &start);
        }
        pos.x = x;
        pos.y = y;
        if (pl_reload_motion_ck(plw) != 0) {
            draw_sprite_anim_idx(icons[1], work->frame_0x190, &pos);
        } else if (loaded == 0 && plw->field_0x018 == 1) {
            draw_sprite_anim_idx(icons[0], work->frame_0x190, &pos);
        }
    }
}

/* 0x802ECC28 (0x200): Draws one player's name plate at `base`: the plate in the player's colour, the class icon
 * (blinking the warning icon while `cockpit_lockon_level_get` reports one), the hunter name and the plate's status icons. */
void cockpit_split_plate_draw(_PLW* plw, const _mh_ivec2_* base) {
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    CockpitWork* work = &cockpit_work[plw->chunk_ofs];
    const _SPR_DATA_* rec;
    u32 color;
    u8 warn;

    rec = get_lsp_data(0xD00, NULL);
    pos.x = rec->pos.x + base->x;
    pos.y = rec->pos.y + base->y;
    rec = get_lsp_data(0xBB6, NULL);
    color = player_color_get(plw->chunk_ofs);
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect((s16)(rec->pos.x + pos.x), (s16)(rec->pos.y + pos.y), rec->width, rec->height);
    drawshape_set_corner_colors(-1, color, color, -1);
    drawshape_set_texture_rect(rec->tex_flag, &rec->uv0, &rec->uv1);
    drawshape_exec();
    spr_data_copy(&spr, get_lsp_data(0xBB7, NULL));
    spr.color = color;
    draw_sprite(spr, &pos);
    warn = cockpit_lockon_level_get(plw);
    if (warn != 0 && ((work->frame_0x190 / 20) & 1) != 0) {
        draw_sprite_uv_idx(0xBBB, warn == 2 ? 0 : 1, &pos);
    } else {
        draw_sprite_uv_color_idx(0xBB8, plw->field_0x002, color, &pos);
    }
    rec = get_lsp_data(0xBB9, NULL);
    font_set_size(rec->width, rec->height);
    font_print_ex((s16)(pos.x + rec->pos.x), (s16)(pos.y + rec->pos.y), 0, (s8*)"%s", plw->hunter_0x5CA.name_0x5CA);
    cockpit_plate_icons_draw(&pos, plw);
}

/* 0x802ECE28 (0x3E4): Draws one player's plate in the cockpit's party list: the master's own plate on top, the
 * others stacked by `index`; the plate, class icon, mark-table entry, name (or id), transfer icon, status icons and
 * the health bar of a non-master. */
void cockpit_party_plate_draw(_PLW* plw, u16 index) {
    _mh_ivec2_ pos;
    _mh_ivec2_ mark;
    _SPR_DATA_ spr;
    CockpitWork* work = cockpit_work;
    const _SPR_DATA_* rec;
    u32 color;
    u16 text_id;
    u8 warn;
    s16 width;

    if (Pl_master_ck(plw) == 1) {
        get_lsp_data(0xBB5, &pos);
        rec = get_lsp_data(0xBB6, NULL);
        text_id = 0xBB9;
    } else {
        get_lsp_data(0xBBE, &pos);
        pos.y += (s16)(index * 25);
        rec = get_lsp_data(0xBC0, NULL);
        text_id = 0xBC2;
    }
    color = player_color_get(plw->chunk_ofs);
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect((s16)(rec->pos.x + pos.x), (s16)(rec->pos.y + pos.y), rec->width, rec->height);
    drawshape_set_corner_colors(-1, color, color, -1);
    drawshape_set_texture_rect(rec->tex_flag, &rec->uv0, &rec->uv1);
    drawshape_exec();
    spr_data_copy(&spr, get_lsp_data(0xBB7, NULL));
    spr.color = color;
    draw_sprite(spr, &pos);
    warn = cockpit_lockon_level_get(plw);
    if (warn != 0 && ((work->frame_0x190 / 20) & 1) != 0) {
        draw_sprite_uv_idx(0xBBB, warn == 2 ? 0 : 1, &pos);
    } else {
        draw_sprite_uv_color_idx(0xBB8, plw->field_0x002, color, &pos);
    }
    if ((work->slot_flags_0x0C8 & (1 << plw->chunk_ofs)) != 0) {
        rec = get_lsp_data(0xBBD, NULL);
        mark.x = rec->pos.x + pos.x;
        mark.y = rec->pos.y + pos.y;
        quest_mark_append(work, &mark, 0, work->slot_state_0x0C9[plw->chunk_ofs] == 0 ? color : 0xF0F0F0FF,
                          work->slot_timer_0x0C0[plw->chunk_ofs]);
    }
    rec = get_lsp_data(text_id, NULL);
    font_set_size(rec->width, rec->height);
    if (isServerSelectState() == 0 || get_option_cfg(12) == 0) {
        font_print_ex((s16)(pos.x + rec->pos.x), (s16)(pos.y + rec->pos.y), 0, (s8*)"%s", plw->hunter_0x5CA.name_0x5CA);
    } else {
        font_print_ex((s16)(pos.x + rec->pos.x), (s16)(pos.y + rec->pos.y), 0, (s8*)"%s", plw->hunter_0x5CA.id_0x5DB);
    }
    if (game_ready_ck() == 1 && (s32)(u8)cockpit_state.transfer_mode_0x07C[plw->chunk_ofs] == 3 && (system_w.field_0x0c & 0x10) == 0) {
        draw_tex_icon_square((s16)(pos.x + rec->pos.x + 46), (s16)(pos.y + rec->pos.y), rec->width, -1, 34);
    }
    cockpit_plate_icons_draw(&pos, plw);
    if (Pl_master_ck(plw) == 0) {
        rec = get_lsp_data(0xBC1, NULL);
        width = rec->width * plw->health / plw->health_max;
        drawshape_init(3, 0xFFFF);
        drawshape_set_vertex_rect((s16)(pos.x + rec->pos.x), (s16)(pos.y + rec->pos.y), width, rec->height);
        drawshape_set_flat_color(0x57CC33FF);
        drawshape_exec();
    }
}

/* 0x802ED20C (0x274): Draws the AI NPC's plate in the party list: the plate and icon in the NPC colour, its name
 * line, its health bar, its status icons and its mark-table entry while its view timer runs. */
void cockpit_npc_plate_draw(void) {
    _mh_ivec2_ pos;
    _mh_ivec2_ mark;
    _SPR_DATA_ spr;
    CockpitWork* work = cockpit_work;
    _AINPC_W* npc = work->ai_npc_0x048;
    const _SPR_DATA_* rec;
    u8** names;
    s32 single;
    s16 width;

    get_lsp_data(0xBBE, &pos);
    rec = get_lsp_data(0xBC0, NULL);
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect((s16)(rec->pos.x + pos.x), (s16)(rec->pos.y + pos.y), rec->width, rec->height);
    drawshape_set_corner_colors(-1, 0xC8E52EFF, 0xC8E52EFF, -1);
    drawshape_set_texture_rect(rec->tex_flag, &rec->uv0, &rec->uv1);
    drawshape_exec();
    spr_data_copy(&spr, get_lsp_data(0xBBF, NULL));
    spr.color = 0xC8E52EFF;
    draw_sprite(spr, &pos);
    draw_sprite_cell_color_idx(0xBC5, 0x3A, 0xC8E52EFF, &pos);
    names = get_str_tbl(0x55);
    single = ai_npc_mode1_ck(npc) == 1;
    rec = get_lsp_data(0xBC2, NULL);
    font_set_size(rec->width, rec->height);
    font_print_ex((s16)(pos.x + rec->pos.x), (s16)(pos.y + rec->pos.y), 0, (s8*)names[(u8)single]);
    rec = get_lsp_data(0xBC1, NULL);
    width = rec->width * npc->field_0x1F0 / npc->field_0x1F4;
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect((s16)(pos.x + rec->pos.x), (s16)(pos.y + rec->pos.y), width, rec->height);
    drawshape_set_flat_color(0x57CC33FF);
    drawshape_exec();
    cockpit_npc_plate_icons_draw(&pos);
    if (ai_npc_call_ck(npc) == 1 && work->view_timer_0x0BE < 0) {
        work->view_timer_0x0BE = 0;
    }
    if (work->view_timer_0x0BE >= 0) {
        rec = get_lsp_data(0xBC7, NULL);
        mark.x = rec->pos.x + pos.x;
        mark.y = rec->pos.y + pos.y;
        quest_mark_append(work, &mark, 0, 0xC8E52EFF, work->view_timer_0x0BE);
    }
}

/* 0x802ED480 (0x108): Draws the cockpit's party list: the local player's plate, then every other live non-master
 * player's plate once the game is ready - before that, the AI NPC's plate while the quest move shows one. */
void cockpit_party_list_draw(void) {
    _PLW* plw;
    u16 max;
    u16 index;
    s32 i;

    set_blendmode(4, 5, 1);
    plw = (_PLW*)get_move_work_adrs(2);
    cockpit_party_plate_draw(&plw[(s8)my_player_no()], 0);
    if (game_ready_ck() == 1) {
        plw = (_PLW*)get_move_work_adrs(2);
        max = get_move_work_max(2);
        index = 0;
        for (i = 0; i < max; i++, plw++) {
            if (plw->slot_active != 0 && Pl_master_ck(plw) == 0) {
                cockpit_party_plate_draw(plw, index);
                index++;
            }
        }
    } else if (((SysQuestMove*)get_move_work_adrs(0))->npc_plate_0x0EF != 0 && quest_move_state_valid_ck() == 1) {
        cockpit_npc_plate_draw();
    }
    font_flush();
}

/* 0x802ED620 (0xD4): The AI NPC's status icon bits: one bit per set mask word of the kind's table, then (kind 0) the
 * NPC's follow, wait or arrived state as bit 3, 4 or 5. */
s32 cockpit_npc_icon_mask_get(_AINPC_W* npc, u8 kind) {
    s32 result = 0;
    s32 bit = 1;
    const u32* mask;
    u32 flags;

    mask = kind == 0 ? cockpit_npc_mask0 : cockpit_npc_mask1;
    flags = npc->field_0x20C;
    for (;;) {
        if (*mask == 0) {
            break;
        }
        if ((flags & *mask) != 0) {
            result |= bit;
        }
        bit <<= 1;
        mask++;
    }
    if (kind == 0) {
        if (ai_npc_wait_ck(npc) == 1) {
            result |= 8;
        } else if (ai_npc_motion5_ck(npc) == 1) {
            result |= 0x10;
        } else if ((u32)ai_npc_arrived_ck(npc) == 1) {
            result |= 0x20;
        }
    }
    return result;
}

/* 0x802ED88C (0xE4): Steps the status-icon cursors: both of every live player's, then the AI NPC's two. */
void cockpit_icon_cursors_update(void) {
    _PLW* plw;
    CockpitWork* work = cockpit_work;
    u16 max;
    s32 i;
    _AINPC_W* npc;

    plw = (_PLW*)get_move_work_adrs(2);
    if (plw != NULL) {
        max = get_move_work_max(2);
        for (i = 0; i < max; i++, plw++) {
            if (plw->slot_active != 0) {
                cockpit_act_cursor_step(plw, &work->act_cursor_0x018[plw->chunk_ofs], 0);
                cockpit_act_cursor_step(plw, &work->skill_cursor_0x020[plw->chunk_ofs], 1);
            }
        }
        npc = work->ai_npc_0x048;
        cockpit_npc_cursor_step(npc, &work->npc_cursor_0x028[0], 0);
        cockpit_npc_cursor_step(npc, &work->npc_cursor_0x028[1], 1);
    }
}

/* 0x802ED970 (0xB4): Draws the player's two status icons on the plate at `pos`, each while its cursor is armed. */
void cockpit_plate_icons_draw(const _mh_ivec2_* pos, _PLW* plw) {
    CockpitWork* work = cockpit_work;
    Cursor2* cur;

    cur = &work->act_cursor_0x018[plw->chunk_ofs];
    if (cur->timer >= 0) {
        draw_sprite_uv_idx(0xBBA, cockpit_act_icon_cells[cur->index], pos);
    }
    cur = &work->skill_cursor_0x020[plw->chunk_ofs];
    if (cur->timer >= 0) {
        draw_sprite_uv_idx(0xBBC, cockpit_skill_icon_cells[cur->index], pos);
    }
}

/* The AI NPC's two status icon tables (sprite cells of 0xBC4 and 0xBC6), indexed by its cursors. */
u8 cockpit_npc_act_cells[8] = { 0x04, 0x02, 0x3D, 0x40, 0x39, 0x3A, 0x00, 0x00 };
u8 cockpit_npc_skill_cells[4] = { 0x07, 0x06, 0x09, 0x08 };

/* 0x802EDA24 (0xCC): Draws the AI NPC's two status icons on its plate at `pos`; icons 0x40 and 0x3A get their own
 * colours. */
void cockpit_npc_plate_icons_draw(const _mh_ivec2_* pos) {
    CockpitWork* work = cockpit_work;

    u8 icon;

    if (work->npc_cursor_0x028[0].timer >= 0) {
        icon = cockpit_npc_act_cells[work->npc_cursor_0x028[0].index];
        switch (icon) {
        case 0x40:
            draw_sprite_cell_color_idx(0xBC4, icon, 0xFCE45EFF, pos);
            break;
        case 0x3A:
            draw_sprite_cell_color_idx(0xBC4, icon, 0xF1A86DFF, pos);
            break;
        default:
            draw_sprite_uv_idx(0xBC4, icon, pos);
            break;
        }
    }
    if (work->npc_cursor_0x028[1].timer >= 0) {
        draw_sprite_uv_idx(0xBC6, cockpit_npc_skill_cells[work->npc_cursor_0x028[1].index], pos);
    }
}

/* 0x802EDB0C (0x1D8): Steps the special-shell icon state while the selected slot holds shell 0xDF: the hold and
 * blink counters and the level while the weapon is charged, the fade alpha up or down; latches the shell flag. */
void cockpit_item_ring_update(_PLW* plw, u8* anim, u8* fade) {
    u8 on = plw->slot_id[plw->field_0x304].item_id == 0xDF;

    if (plw->field_0x308 == 0) {
        if (anim[1] != 0) {
            anim[1]--;
        }
        anim[2] = 0;
        if (on != anim[0]) {
            anim[3] = on != 0 ? 16 : 0;
        } else if (on == 0) {
            if (anim[3] != 0) {
                anim[3]--;
            }
            if (anim[3] > 3) {
                anim[3] = 3;
            }
        } else {
            if (anim[3] < 16) {
                anim[3]++;
            }
            if (anim[3] > 16) {
                anim[3] = 16;
            }
        }
        if (fade[1] > 16) {
            fade[1] -= 16;
        } else {
            fade[1] = 0;
        }
    } else {
        if (anim[1] < 10) {
            anim[1]++;
        }
        anim[2]++;
        if (anim[2] >= 190) {
            anim[2] = 40;
        } else if ((u8)(anim[2] + 166) <= 9) {
            anim[2] = 40;
        }
        if ((plw->field_0x309 & 0x20) != 0) {
            anim[2] = 100;
        }
        if (on == 0) {
            if (anim[3] != 0) {
                anim[3]--;
            }
            if (anim[3] > 3) {
                anim[3] = 3;
            }
        } else {
            if (anim[3] < 3) {
                anim[3]++;
            }
            if (anim[3] > 3) {
                anim[3]--;
            }
        }
        fade[1] += 10;
        if (fade[1] > 150) {
            fade[1] = 150;
        }
    }
    anim[0] = on;
}

/* 0x802EDCE4 (0x5A4): Draws the item-slot ring: the frame, the selected item and up to four neighbours (the shell
 * slot's frame cells swapped in), the act icons, the selected item's name and count, and the use prompt. */
void cockpit_item_ring_draw(_PLW* plw, u8* anim) {
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    u16 next2;
    u16 prev2;
    u16 prev;
    u16 next;
    u16 cur;
    s16 count;
    u32 dead;
    const u16* alt;
    const u16* ids;
    const _SPR_DATA_* rec;
    u16 item;
    s16 num;

    set_blendmode(4, 5, 1);
    get_lsp_data(0xC20, &pos);
    draw_sprite_anim_ary(cockpit_item_ring_open_ids, anim[1], &pos);
    draw_sprite_anim_ary(cockpit_item_ring_level_ids, anim[3], &pos);
    if (anim[1] != 0) {
        next2 = 0xFF;
        prev2 = 0xFF;
        prev = 0xFF;
        next = 0xFF;
        cur = 0xFF;
        count = cockpit_item_slot_live_count(plw);
        dead = cockpit_item_slot_live_ck(plw, plw->field_0x304) == 0;
        if (dead != 0) {
            count++;
        }
        if (dead == 0) {
            cur = plw->field_0x304;
        }
        switch (count) {
        case 0:
        case 1:
            break;
        default:
            next = pl_item_slot_cycle(plw, plw->field_0x304, 1);
            prev = pl_item_slot_cycle(plw, plw->field_0x304, 0);
            prev2 = pl_item_slot_cycle(plw, next, 1);
            next2 = pl_item_slot_cycle(plw, prev, 0);
            break;
        case 4:
            next = pl_item_slot_cycle(plw, plw->field_0x304, 1);
            prev = pl_item_slot_cycle(plw, plw->field_0x304, 0);
            prev2 = pl_item_slot_cycle(plw, next, 1);
            break;
        case 3:
            prev = pl_item_slot_cycle(plw, plw->field_0x304, 0);
        case 2:
            next = pl_item_slot_cycle(plw, plw->field_0x304, 1);
            break;
        }
        alt = NULL;
        if (plw->slot_id[next].item_id == 0xDF) {
            alt = cockpit_item_ring_shell_next_ids;
        } else if (plw->slot_id[prev].item_id == 0xDF) {
            alt = cockpit_item_ring_shell_prev_ids;
        } else if (plw->slot_id[prev2].item_id == 0xDF) {
            alt = cockpit_item_ring_shell_prev2_ids;
        } else if (plw->slot_id[next2].item_id == 0xDF) {
            alt = cockpit_item_ring_shell_next2_ids;
        }
        if (alt == NULL) {
            draw_sprite_anim_ary(cockpit_item_ring_frame_ids, anim[1], &pos);
        } else {
            for (ids = cockpit_item_ring_frame_ids; *ids != 0xFFFF; ids++, alt++) {
                sprite_frame_apply(&spr, *ids, anim[1], NULL);
                if (*ids != *alt) {
                    rec = get_lsp_data(*alt, NULL);
                    tex_uv_copy(&spr.uv0, &rec->uv0);
                    tex_uv_copy(&spr.uv1, &rec->uv1);
                }
                draw_sprite(spr, &pos);
            }
        }
        cockpit_item_slot_icon_draw(plw, 0xC23, anim[1], cur, &pos);
        cockpit_item_slot_icon_draw(plw, 0xC2D, anim[1], next, &pos);
        cockpit_item_slot_icon_draw(plw, 0xC2E, anim[1], prev, &pos);
        cockpit_item_slot_icon_draw(plw, 0xC2F, anim[1], prev2, &pos);
        cockpit_item_slot_icon_draw(plw, 0xC30, anim[1], next2, &pos);
    } else {
        cockpit_item_slot_icon_draw(plw, 0xC23, anim[1], plw->field_0x304, &pos);
    }
    cockpit_act_icon_draw(plw, 0xC3F, anim[3], 4, &pos);
    cockpit_act_icon_draw(plw, 0xC40, anim[3], 2, &pos);
    draw_sprite_anim_ary(cockpit_item_ring_act_ids, anim[3], &pos);
    item = plw->slot_id[plw->field_0x304].item_id;
    if (item != 0 && (item != 0xDF || anim[3] == 3)) {
        draw_font_idx(0xC2C, item == 0xDF ? (s8*)get_str_tbl(0x58)[3] : (s8*)ItemName(item), 5, &pos);
        if (GetItemData(item)->max_num_0x003 != 1) {
            draw_sprite_anim_idx(0xC24, anim[1], &pos);
            num = plw->slot_id[plw->field_0x304].value;
            if (num == GetItemData(item)->max_num_0x003) {
                draw_number_2digit_anim_ary(cockpit_item_ring_digit_ids, anim[1], num, 0xF53737FF, &pos);
            } else {
                draw_number_2digit_anim_ary(cockpit_item_ring_digit_ids, anim[1], num, 0, &pos);
            }
        }
    }
    if (get_cfg(plw->chunk_ofs, 7) == 0) {
        draw_sprite_anim_idx(0xC37, anim[1], &pos);
    } else {
        draw_sprite_anim_idx(0xC38, anim[1], &pos);
    }
    if (plw->field_0x3E4 == 7 && plw->field_0x3E8 == 0 && item != 0 && item != 0xDF && get_cfg(plw->chunk_ofs, 7) == 0) {
        draw_sprite_anim_idx(0xC39, anim[2], &pos);
        draw_sprite_anim_idx(0xC36, anim[2], &pos);
        if (pl_item_now_usable_ck(plw, item) != 0) {
            set_blendmode(4, 1, 1);
            draw_sprite_anim_idx(0xC35, anim[2], &pos);
            set_blendmode(4, 5, 1);
        }
    }
}

/* The item list's name colours: selected/other slot, matching/other kind. */
u32 cockpit_item_list_colors[4] = { 0xF3D73EFF, 0xD1BA3FFF, 0xF5F5F5FF, 0xB4B4B4FF };

/* 0x802EE3AC (0x2B0): Draws one entry of the item list at `id` offset by (`x`, `y`): the item name and its count
 * (or the stack text), shadowed for the selected slot and coloured by slot and kind, then the item icon. */
void cockpit_item_list_entry_draw(_PLW* plw, u16 id, s16 x, s16 y, u8 slot) {
    char text[12];
    _mh_ivec2_ pos;
    const _SPR_DATA_* rec;
    const _SPR_DATA_* line;
    u16 item;
    u32 selected;
    s16 name_x;
    s16 name_y;
    s16 idx;
    u32 color;
    s16 text_x;
    u8 kind;
    u16 num_id;

    if (slot != 0xFF && pl_item_slot_count_get(plw, slot) != 0) {
        item = pl_item_slot_item_get(plw, slot);
        if (GetItemData(item)->kind_0x00 == 1) {
            selected = slot == plw->item_slot_sel;
            rec = get_lsp_data(id, NULL);
            x += rec->pos.x;
            y += rec->pos.y;
            line = get_lsp_data(0xC9E, NULL);
            name_x = x + line->pos.x;
            name_y = y + line->pos.y;
            kind = pl_item_icon_kind_get(item);
            idx = 0;
            if (kind != plw->ammo_icon_kind_0x26D) {
                idx = 2;
            }
            if (plw->item_slot_sel != slot) {
                idx++;
            }
            color = cockpit_item_list_colors[idx];
            font_set_size(line->width, line->height);
            name_x = menu_text_center_x((char*)ItemName(item), name_x, 16);
            if (selected != 0) {
                font_print_ex(name_x + 2, name_y + 1, 1, (s8*)"%s", ItemName(item));
            }
            font_locate(name_x, name_y);
            flfntSetColor(color);
            font_print((s8*)"%s", ItemName(item));
            if (item_is_53_ck(item) == 1) {
                sprintf(text, (const char*)get_str_tbl(31)[2]);
                num_id = 0xC9F;
            } else {
                sprintf(text, "%d", pl_item_slot_count_get(plw, slot));
                num_id = item_count_label_id(pl_item_slot_count_get(plw, slot));
            }
            rec = get_lsp_data(num_id, NULL);
            text_x = menu_text_center_x(text, (s16)(x + rec->pos.x), rec->width);
            if (selected != 0) {
                font_print_ex(text_x + 2, name_y + 1, 1, (s8*)text);
            }
            font_locate(text_x, name_y);
            font_set_size(rec->width, rec->height);
            flfntSetColor(color);
            font_print((s8*)text);
            pos.x = x;
            pos.y = y;
            draw_itemicon_item_id_idx(0xC9D, item, &pos);
        }
    }
}

/* The item list frame's opening animation and its second-page overlay (sprite ids, 0xFFFF-terminated). */
u16 cockpit_item_list_frame_ids[16] = {
    0x0C94, 0x0C88, 0x0C89, 0x0C8A, 0x0C8B, 0x0C8C, 0x0C8D, 0x0C8E,
    0x0C8F, 0x0C90, 0x0C91, 0x0C92, 0x0C93, 0x0C9A, 0xFFFF, 0x0000,
};
u16 cockpit_item_list_page2_ids[18] = {
    0x0C7A, 0x0C7B, 0x0C7C, 0x0C7D, 0x0C7E, 0x0C7F, 0x0C80, 0x0C81, 0x0C82,
    0x0C83, 0x0C84, 0x0C85, 0x0C98, 0x0C99, 0x0C95, 0x0C96, 0x0C97, 0xFFFF,
};

/* 0x802EE65C (0x1D0): Draws the item list of the second item page: the frame (opening with the hold frame), the act
 * icons, then the selected item and - as the frame opens - its two neighbours. */
void cockpit_item_list_draw(_PLW* plw, u8* anim) {
    _mh_ivec2_ pos;
    u16 next;
    u16 prev;
    u8 frame;

    if (pl_item_page2_ck(plw) != 0) {
        set_blendmode(4, 5, 1);
        frame = anim[1] / 10;
        get_lsp_data(0xC79, &pos);
        draw_sprite_anim_ary(cockpit_item_list_frame_ids, frame, &pos);
        if (frame != 0) {
            draw_sprite_anim_ary(cockpit_item_list_page2_ids, frame, &pos);
            cockpit_act_icon_draw(plw, 0xC86, frame, 0x10, &pos);
            cockpit_act_icon_draw(plw, 0xC87, frame, 8, &pos);
        }
        if (frame <= 3) {
            cockpit_item_list_entry_draw(plw, 0xC9A, pos.x, pos.y, plw->item_slot_sel);
        } else {
            cockpit_item_list_entry_draw(plw, 0xC99, pos.x, pos.y, plw->item_slot_sel);
            next = pl_item_slot_next_get(plw, plw->item_slot_sel, 1);
            if (next != 0xFF && next != plw->item_slot_sel) {
                cockpit_item_list_entry_draw(plw, 0xC9A, pos.x, pos.y, next);
            }
            if (frame > 7) {
                prev = pl_item_slot_next_get(plw, plw->item_slot_sel, 0);
                if (prev != 0xFF && prev != plw->item_slot_sel && next != prev) {
                    cockpit_item_list_entry_draw(plw, 0xC98, pos.x, pos.y, prev);
                }
            }
        }
        font_flush();
    }
}

/* 0x802EE82C (0x150): Draws the cockpit's floating item tag over the player: projected through the current camera,
 * kept below the screen top, offset by the tag's own y. */
void cockpit_item_tag_draw(CockpitWork* work) {
    VEC3 screen;
    u32 camera;
    _mh_ivec2_ pos;
    s32 handle;
    _PLW* plw = work->plw;
    CockpitItemTag* tag;

    if (plw->field_0x3E4 != 0) {
        tag = &work->item_tag_0x02C;
        if (tag->tex_idx_0x15 != 0) {
            set_blendmode(4, 5, 1);
            VEC3_ctor(&screen);
            chara_icon_pos_get(plw, 1, &tag->pos);
            handle = scn_root_get_current_camera(pRoot);
            word_copy_return_dst(&camera, &handle);
            camera_world_to_screen(&camera, &screen, &tag->pos);
            if (screen.z < 0.0f || screen.z > 1.0f) {
                return;
            }
            screen.x *= hud_screen_width_get();
            screen.y *= 1.07142854f;
            if (screen.y < 100.0f) {
                screen.y = 100.0f;
            }
            pos.x = screen.x;
            pos.y = screen.y + tag->ofs_y_0x0F;
            cockpit_item_tag_draw_at(plw, tag, &pos, 0xFF);
        }
    }
}

/* The item tag's opening animation (sprite ids, 0xFFFF-terminated). */
u16 cockpit_item_tag_open_ids[4] = { 0x0BDD, 0x0BDE, 0xFFFF, 0x0000 };

/* 0x802EE97C (0x2A8): Draws one item tag at `pos`: the frame, the icon (crossed out when unusable) and its count; on
 * the second item page with the prompt on, the use prompt; with the menu open, the opening animation. */
void cockpit_item_tag_draw_at(_PLW* plw, const CockpitItemTag* tag, const _mh_ivec2_* pos, u8 alpha) {
    u32 prompt;
    u16 id;
    u8 frame;

    set_blendmode(4, 5, 1);
    prompt = 0;
    if (get_cfg(plw->chunk_ofs, 7) == 0) {
        prompt = 1;
    }
    if (pl_item_menu_open_ck(plw) == 0 || plw->field_0x3E4 == 7) {
        if (plw->field_0x3E8 == 0 && prompt == 1) {
            id = 0xBD6;
        } else {
            id = 0xBD7;
        }
        draw_sprite_anim_idx(id, tag->frame_0x0C, pos);
        draw_itemicon_frame_idx(0xBDC, tag->frame_0x0C, tag->tex_idx_0x15, tag->color_0x10, pos);
        if (tag->unusable_0x16 != 0) {
            draw_itemicon_frame_idx(0xBDC, tag->frame_0x0C, 0x71, 0xFF0000FF, pos);
        }
        if (tag->count_0x19 != 0) {
            draw_number_2digit_anim_ary(cockpit_item_tag_digit_ids, tag->frame_0x0C, tag->count_0x19, 0, pos);
        }
        if (plw->field_0x3E8 == 0 && prompt == 1) {
            draw_sprite_anim_idx(0xBDA, tag->frame_0x0C, pos);
            draw_sprite_anim_idx(0xBD9, tag->frame_0x0C, pos);
            if (tag->open_0x17 != 0 && tag->unusable_0x16 == 0) {
                draw_sprite_anim_idx(0xBDB, tag->frame_0x0C, pos);
            }
            if (tag->unusable_0x16 == 0) {
                set_blendmode(4, 1, 1);
                draw_sprite_anim_idx(0xBD8, tag->frame_0x0C, pos);
            }
        }
    } else {
        draw_sprite_anim_idx(0xBD7, tag->frame_0x0C, pos);
        frame = tag->open_frame_0x0E;
        if (frame > 30) {
            frame = 30;
        }
        if (tag->unusable_0x16 == 0) {
            if (prompt == 1) {
                draw_sprite_anim_ary(cockpit_item_tag_open_ids, tag->open_frame_0x0E, pos);
            } else {
                draw_itemicon_frame_idx(0xBDC, tag->frame_0x0C, tag->tex_idx_0x15, tag->color_0x10, pos);
            }
        } else if (prompt == 1) {
            draw_sprite_anim_idx(0xBDD, frame, pos);
            draw_sprite_anim_idx(0xBDF, frame, pos);
        } else {
            draw_itemicon_frame_idx(0xBDC, tag->frame_0x0C, tag->tex_idx_0x15, tag->color_0x10, pos);
            draw_itemicon_frame_idx(0xBDC, tag->frame_0x0C, 0x71, 0xFF0000FF, pos);
        }
    }
}

/* 0x802EED0C (0xD8): The player's lock-on warning level: 1 when a live, alert quest target that sees the player
 * targets it with state 1, else 2 for state 2, else 0. */
u8 cockpit_lockon_level_get(_PLW* plw) {
    s32 level = 0;
    u16 max = get_move_work_max(3);
    _ENEMY_WORK* enemy = (_ENEMY_WORK*)get_move_work_adrs(3);
    u16 i;

    for (i = 0; i < max; i++, enemy++) {
        if (enemy->active != 0 && (enemy->field_0x1C8 & 1) != 0 && em_sight_player_ck2(enemy, plw->chunk_ofs) == 1 &&
            em_pl_target_ck(enemy, plw) != 0) {
            if (enemy->field_0x43D == 1) {
                level = 1;
            }
            if (enemy->field_0x43D == 2 && level == 0) {
                level = 2;
            }
        }
    }
    return level;
}

/* 0x802EEDE4 (0xBC): Draws the aim marker at the world position `pos`, projected through the current camera. */
void cockpit_aim_marker_draw(VEC3 pos) {
    VEC3 screen;
    u32 camera;
    _mh_ivec2_ spot;
    s32 handle;

    VEC3_ctor(&screen);
    handle = scn_root_get_current_camera(pRoot);
    word_copy_return_dst(&camera, &handle);
    camera_world_to_screen(&camera, &screen, &pos);
    if (screen.z < 0.0f || screen.z > 1.0f) {
        return;
    }
    spot.x = screen.x * hud_screen_width_get();
    spot.y = 1.07142854f * screen.y;
    draw_sprite_idx(0xC6D, &spot);
}

/* 0x802EEEA0 (0xDC): `cockpit_aim_marker_draw` through player `player`'s own scene camera. */
void cockpit_aim_marker_draw_split(VEC3 pos, u8 player) {
    VEC3 screen;
    u32 camera;
    _mh_ivec2_ spot;
    s32 handle;

    VEC3_ctor(&screen);
    handle = ((nw4r::g3d::ScnRoot*)pRoot)->GetCamera(Screen_w.camera_view.player_camera_0x50[player]);
    word_copy_return_dst(&camera, &handle);
    camera_world_to_screen(&camera, &screen, &pos);
    if (screen.z < 0.0f || screen.z > 1.0f) {
        return;
    }
    spot.x = screen.x * hud_screen_width_get();
    spot.y = 1.07142854f * screen.y;
    draw_sprite_idx(0xC6D, &spot);
}

/* 0x802EEF7C (0x90): Draws the aim marker 1000 units ahead of the player's eye. */
void cockpit_eye_marker_draw(_PLW* plw) {
    MTX34 eye;
    VEC3 pos;
    VEC3 ofs;

    VEC3_ctor(&pos);
    MTX34_ctor(&eye);
    pl_eye_mtx_get(plw, &eye);
    copyVec3(&pos, setVec3(&ofs, 0.0f, 0.0f, 1000.0f));
    mulVecMatAddTrans(&pos, &eye);
    cockpit_aim_marker_draw(pos);
}

/* 0x802EF00C (0x94): `cockpit_eye_marker_draw` through the player's own scene camera. */
void cockpit_eye_marker_draw_split(_PLW* plw) {
    MTX34 eye;
    VEC3 pos;
    VEC3 ofs;

    VEC3_ctor(&pos);
    MTX34_ctor(&eye);
    pl_eye_mtx_get(plw, &eye);
    copyVec3(&pos, setVec3(&ofs, 0.0f, 0.0f, 1000.0f));
    mulVecMatAddTrans(&pos, &eye);
    cockpit_aim_marker_draw_split(pos, plw->chunk_ofs);
}

/* 0x802EF0FC (0x134): Draws the digit over the AI NPC's head (when it carries one), projected through the current
 * camera. */
#pragma fp_contract off
void cockpit_npc_digit_draw(CockpitWork* work) {
    VEC3 screen;
    VEC3 world;
    u32 camera;
    s32 handle;
    _AINPC_W* npc = work->ai_npc_0x048;

    VEC3_ctor(&screen);
    assignVec3((Vec*)&world, (Vec*)&npc->vec_0x178);
    handle = scn_root_get_current_camera(pRoot);
    word_copy_return_dst(&camera, &handle);
    if (work->npc_digit_0x04C != 0) {
        world.y += 150.0f;
        camera_world_to_screen(&camera, &screen, &world);
        if (screen.z < 0.0f || screen.z > 1.0f) {
            return;
        }
        screen.x *= hud_screen_width_get();
        screen.y *= 1.07142854f;
        HudSize2 size(32.0f, 32.0f);
        HudPos3 pos(screen.x - 0.5f * size.w, screen.y, screen.z);
        draw_digit_rect(pos, size, work->npc_digit_0x04C, work->npc_digit_color_0x050);
    }
}
#pragma fp_contract on

/* 0x802EF230 (0x110): Ticks the hunt log: counts its timer down (clearing the log at 0), steps the lines' and the
 * total's flash frames, and flashes the total when the hunt count grows. */
void cockpit_hunt_log_update(void) {
    CockpitState* state = &cockpit_state;
    HuntLogEntry* entry;
    s32 i;
    u32 total;

    if (state->hunt_log_timer_0x05C != 0) {
        if (--state->hunt_log_timer_0x05C == 0) {
            state->hunt_log_count_0x05A = 0;
            return;
        }
        entry = state->hunt_log_0x05E;
        for (i = 0; i < state->hunt_log_count_0x05A; i++, entry++) {
            if (entry->frame != 0) {
                entry->frame++;
                if (entry->frame > 6) {
                    entry->frame = 0;
                }
            }
        }
        if (state->hunt_total_frame_0x05B != 0) {
            if (++state->hunt_total_frame_0x05B > 6) {
                state->hunt_total_frame_0x05B = 0;
            }
        }
        if (move_work_state_ck() != 0) {
            total = em_set_work_state_get();
            if (state->hunt_total_0x070 < total) {
                state->hunt_total_0x070 = total;
                state->hunt_total_frame_0x05B = 1;
            }
        } else {
            total = quest_work_state_get();
            if (state->hunt_total_0x070 < total) {
                state->hunt_total_0x070 = total;
                state->hunt_total_frame_0x05B = 1;
            }
        }
    }
}

/* 0x802EF340 (0xC0): Pushes a hunted monster onto the hunt log (the oldest of three drops off) and shows the log for
 * five seconds. */
void cockpit_hunt_log_push(s32 name_id, s32 count, s32 crown) {
    CockpitState* state = &cockpit_state;

    cockpit_hunt_log_entry_copy(&state->hunt_log_0x05E[2], &state->hunt_log_0x05E[1]);
    cockpit_hunt_log_entry_copy(&state->hunt_log_0x05E[1], &state->hunt_log_0x05E[0]);
    if (state->hunt_log_count_0x05A < 3) {
        state->hunt_log_count_0x05A++;
    }
    state->hunt_log_0x05E[0].name_id = name_id;
    state->hunt_log_0x05E[0].count = count;
    state->hunt_log_0x05E[0].crown = crown;
    state->hunt_log_0x05E[0].frame = 1;
    state->hunt_log_timer_0x05C = 5.0f * Screen_w.frame_scale;
}

/* The hunt log's frame, its row count, its line anchors and the crown animation (sprite ids). */
u16 cockpit_hunt_log_frame_ids[12] = {
    0x0BF8, 0x0BF9, 0x0BFA, 0x0BFB, 0x0BFC, 0x0BF5, 0x0BF6, 0x0BF7, 0xFFFF, 0x0000, 0x0000, 0x0000,
};
u16 cockpit_hunt_log_row_ids[4] = { 0x0BFD, 0x0BFE, 0x0BFF, 0xFFFF };
u16 cockpit_hunt_log_line_ids[4] = { 0x0C01, 0x0C02, 0x0C03, 0x0000 };
u16 cockpit_hunt_log_crown_ids[4] = { 0x0C06, 0x0C07, 0x0C05, 0xFFFF };

/* 0x802EF424 (0x208): Draws the hunt log: the frame, the total (red at 999999), then one line per entry - the
 * monster's name, its count (red at 99) and its crown. */
void cockpit_hunt_log_draw(void) {
    char text[8];
    _mh_ivec2_ base;
    _SPR_DATA_ spr;
    _mh_ivec2_ pos;
    CockpitState* state = &cockpit_state;
    HuntLogEntry* entry;
    s32 i;
    const _SPR_DATA_* rec;
    u16 id;

    set_blendmode(4, 5, 1);
    if (state->hunt_log_timer_0x05C != 0 && state->hunt_log_count_0x05A != 0) {
        get_lsp_data(0xBF4, &base);
        draw_sprite_ary(cockpit_hunt_log_frame_ids, &base);
        sprite_frame_apply(&spr, 0xC00, state->hunt_total_frame_0x05B, NULL);
        if (state->hunt_total_0x070 >= 999999) {
            spr.color = 0xF53737FF;
        }
        sprintf(text, "%d", state->hunt_total_0x070);
        draw_font(spr, (s8*)text, 2, &base);
        draw_sprite_anim_ary(cockpit_hunt_log_row_ids, state->hunt_log_count_0x05A - 1, &base);
        entry = state->hunt_log_0x05E;
        for (i = 0; i < state->hunt_log_count_0x05A; i++, entry++) {
            rec = get_lsp_data(cockpit_hunt_log_line_ids[state->hunt_log_count_0x05A - 1 - i], NULL);
            pos.x = base.x + rec->pos.x;
            pos.y = base.y + rec->pos.y;
            draw_font_idx(0xC09, str_tbl_33_get(entry->name_id), 0, &pos);
            draw_sprite_idx(0xC08, &pos);
            sprintf(text, "%d", entry->count);
            id = 0xC0B;
            if (entry->count > 99) {
                id = 0xC0A;
            }
            sprite_frame_apply(&spr, id, entry->frame, NULL);
            if (entry->count >= 99) {
                spr.color = 0xF53737FF;
            }
            draw_font(spr, (s8*)text, 2, &pos);
            if (entry->crown != 0) {
                draw_sprite_anim_ary(cockpit_hunt_log_crown_ids, entry->crown - 1, &pos);
            }
        }
        font_flush();
    }
}

/* 0x802EF730 (0x14C): Draws the quest HUD's move prompt while its blink runs: the frame, the button (by the control
 * option), the prompt line (the second when the latched flag is 2) and, in act state 0xE, the flashing arrow. */
void cockpit_prompt_draw(void) {
    _mh_ivec2_ pos;
    CockpitState* state = &cockpit_state;
    CockpitWork* work;
    u32 second;
    const _SPR_DATA_* rec;

    if (state->blink_timer_0x074 != 0) {
        set_blendmode(4, 5, 1);
        work = cockpit_work;
        rec = get_lsp_data(0xC0C, &pos);
        draw_prompt_frame(pos.x, pos.y, rec->width, 0, 3);
        get_lsp_data(0xC0D, &pos);
        if (get_option_cfg(7) == 0) {
            draw_sprite_idx(0xC0E, &pos);
        } else {
            draw_sprite_idx(0xC0F, &pos);
        }
        second = state->blink_flag_0x075 == 2;
        draw_font_idx(0xC10, (s8*)get_str_tbl(42)[second], 0, &pos);
        if (work->plw->field_0x3E4 == 0xE) {
            set_blendmode(4, 1, 1);
            draw_sprite_anim_idx(0xC11, state->field_0x078, &pos);
            set_blendmode(4, 5, 1);
        } else {
            draw_sprite_idx(0xC12, &pos);
        }
    }
}

/* 0x802EF87C (0x1B4): The quest cockpit's per-frame update (unless the pad is held): the quest timer notice, both
 * gauges, item tags, shell icons and target masks, the pit-trap latch, the icon animations and the AI NPC entries;
 * then queues the draw tasks. */
void cockpit_split_frame_update(void) {
    CockpitState* state;
    CockpitWork* work;
    CockpitWork* work2;
    _PLW* plw;
    s32 limit;
    s32 elapsed;

    if (pad_hold_ck() != 1) {
        state = &cockpit_state;
        work = cockpit_work;
        work2 = &cockpit_work[1];
        work->field_0x0D1 = 0;
        cockpit_frame_tick();
        if (state->quest_notice_timer_0x004 != 0) {
            state->quest_notice_timer_0x004--;
        }
        limit = 5.0f * (60.0f * Screen_w.frame_scale);
        elapsed = quest_time_elapsed_get();
        if (elapsed <= limit) {
            state->quest_notice_timer_0x004 = 0x7FFF;
        } else if (elapsed < limit * state->quest_notice_step_0x002) {
            state->quest_notice_timer_0x004 = 10.0f * Screen_w.frame_scale;
            state->quest_notice_step_0x002 = elapsed / limit;
        }
        quest_gauge_update(work, 0);
        quest_gauge_update(work2, 1);
        plw = (_PLW*)get_move_work_adrs(2);
        cockpit_item_tag_update(plw, &work->item_tag_0x02C);
        cockpit_item_tag_update(&plw[1], &work2->item_tag_0x02C);
        cockpit_item_ring_update(work->plw, work->item_ring_anim_0x16B, work->item_list_anim_0x16F);
        cockpit_item_ring_update(work2->plw, work2->item_ring_anim_0x16B, work2->item_list_anim_0x16F);
        quest_targets_update_a(work);
        quest_targets_update_a(work2);
        state->pit_trap_shown_0x059 = state->pit_trap_count_0x058;
        state->pit_trap_count_0x058 = 0;
        cockpit_gauge_icons_update(work);
        cockpit_gauge_icons_update(work2);
        cockpit_supply_box_update();
        cockpit_icon_cursors_update();
        menu_slot_anim_update(0);
        menu_slot_anim_update(1);
        ainpc_entry_update_all();
    }
    subTransSet((u32)cockpit_split_draw, 0, 0);
    subTransSetPrio(6, (u32)ainpc_entry_draw_all, 0, 0);
}

/* The two players' marker window anchors (layout ids). */
u16 cockpit_split_marker_anchors[4] = { 0x0CD2, 0x0CD3, 0x0000, 0x0000 };

/* 0x802EFA30 (0x150): Draws player `player`'s split-screen marker window: plain at its anchor, or - with the view
 * gate set - the faded ring with the split view's scale trio. */
void cockpit_split_marker_draw(u8 player) {
    _mh_ivec2_ pos;
    _mh_ivec2_ base;
    CockpitWork* work;
    const _SPR_DATA_* rec;

    if (get_arena_cfg(player, 14) != 1) {
        work = &cockpit_work[player];
        set_blendmode(4, 5, 1);
        get_lsp_data(cockpit_split_marker_anchors[player], &base);
        rec = get_lsp_data(0xCD2, NULL);
        base.x -= rec->pos.x;
        base.y -= rec->pos.y;
        if (work->mark_gate_0x0CE == 0) {
            work->draw_id_0x0BA = 0xD08;
            quest_marker_draw(work, 0xD06, 1, base);
            return;
        }
        work->ring_scale_0x0A0 = 2.0f;
        work->ring_extent_x_0x0A4 = 0.0651041642f;
        work->ring_extent_y_0x0A8 = 0.0651041642f;
        work->draw_id_0x0BA = 0xD08;
        rec = get_lsp_data(0xD07, NULL);
        pos.x = rec->pos.x + base.x;
        pos.y = rec->pos.y + base.y;
        quest_marker_draw_faded(work, 0xD07, 1, pos);
    }
}

/* 0x802EFB80 (0x218): The split-screen cockpit draw task: per player the gauge, the target ring, the hidden-HUD
 * panels (plate, weapon and ammo), the marker window and the mark table, the quick panel; then the quest timer and
 * both item tags. */
void cockpit_split_draw(void) {
    _mh_ivec2_ pos;
    CockpitState* state = &cockpit_state;
    CockpitWork* work = cockpit_work;
    CockpitWork* work2 = &cockpit_work[1];
    _PLW* plw = work->plw;
    _PLW* plw2 = work2->plw;

    work->mark_count_0x0CD = 0;
    work2->mark_count_0x0CD = 0;
    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    if (screen_split_view_get() == 0) {
        cockpit_name_tags_draw(work, 0);
        cockpit_split_weapon_hud_draw(work, 0);
        if (cockpit_hud_hidden_ck(plw, 0) == 1) {
            cockpit_split_gauge_draw(work, 0);
            get_lsp_data(0xCCA, &pos);
            cockpit_split_plate_draw(plw, &pos);
        }
        if (menu_slot_busy_ck(0) == 0) {
            cockpit_split_marker_draw(0);
            if (cockpit_hud_hidden_ck(plw, 0) == 1) {
                cockpit_split_item_list_draw(plw, work->item_list_anim_0x16F, 0);
                cockpit_split_item_ring_draw(plw, work->item_ring_anim_0x16B, 0);
            }
        }
        quest_marks_flush(work);
        cockpit_split_flash_gauge_draw(work, 0);
        menu_split_panel_draw(0);
        cockpit_name_tags_draw(work2, 1);
        cockpit_split_weapon_hud_draw(work2, 1);
        if (cockpit_hud_hidden_ck(plw2, 1) == 1) {
            cockpit_split_gauge_draw(work2, 1);
            get_lsp_data(0xCCB, &pos);
            cockpit_split_plate_draw(plw2, &pos);
        }
        if (menu_slot_busy_ck(1) == 0) {
            cockpit_split_marker_draw(1);
            if (cockpit_hud_hidden_ck(plw2, 1) == 1) {
                cockpit_split_item_list_draw(plw2, work2->item_list_anim_0x16F, 1);
                cockpit_split_item_ring_draw(plw2, work2->item_ring_anim_0x16B, 1);
            }
        }
        quest_marks_flush(work2);
        cockpit_split_flash_gauge_draw(work2, 1);
        menu_split_panel_draw(1);
    }
    if (state->quest_notice_timer_0x004 != 0) {
        cockpit_split_timer_draw(1);
    }
    cockpit_split_item_tag_draw(work);
    cockpit_split_item_tag_draw(work2);
}

/* The quest timer's ring: its 16 vertices, the 36 cell texture corners and the cell index per 5-second step. */
_mh_ivec2_ cockpit_split_timer_ring_verts[16] = {
    { 0, 0 }, { -17, 0 }, { -17, -17 }, { 0, -17 }, { 0, 0 }, { 0, 17 }, { -17, 17 }, { -17, 0 },
    { 0, 0 }, { 17, 0 }, { 17, 17 }, { 0, 17 }, { 0, 0 }, { 0, -17 }, { 17, -17 }, { 17, 0 },
};
_mh_tex_uv_ cockpit_split_timer_ring_uvs[36] = {
    { 0x154, 0xA0 }, { 0x154, 0x8F }, { 0x165, 0x8F }, { 0x165, 0xA0 }, { 0x154, 0xA0 }, { 0x154, 0x8F },
    { 0x165, 0x8F }, { 0x165, 0xA0 }, { 0x154, 0xA0 }, { 0x154, 0x8F }, { 0x165, 0x8F }, { 0x165, 0xA0 },
    { 0x154, 0xA0 }, { 0x154, 0x8F }, { 0x165, 0x8F }, { 0x165, 0xA0 }, { 0x178, 0xA0 }, { 0x178, 0x8F },
    { 0x189, 0x8F }, { 0x189, 0xA0 }, { 0x154, 0xA0 }, { 0x154, 0x8F }, { 0x165, 0x8F }, { 0x165, 0xA0 },
    { 0x154, 0xA0 }, { 0x154, 0x8F }, { 0x165, 0x8F }, { 0x165, 0xA0 }, { 0x154, 0xA0 }, { 0x154, 0x8F },
    { 0x165, 0x8F }, { 0x165, 0xA0 }, { 0x166, 0xA0 }, { 0x166, 0x8F }, { 0x177, 0x8F }, { 0x177, 0xA0 },
};
u8 cockpit_split_timer_ring_cells[16] = {
    0x00, 0x14, 0x04, 0x04, 0x18, 0x08, 0x08, 0x1C, 0x0C, 0x0C, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00,
};

/* 0x802EFD98 (0x188): Draws the quest timer's ring at layout `id` for `seconds` (0..55): the ring fills in 15-second
 * quarters and its cell art steps every 5 seconds. */
void cockpit_timer_ring_draw_at(u16 id, s32 seconds, const _mh_ivec2_* pos) {
    f32 ofs[2];
    f32 scale[2];
    const _SPR_DATA_* rec;
    u8 cell;

    if (seconds <= 55) {
        if (seconds < 0) {
            seconds = 0;
        }
        cell = cockpit_split_timer_ring_cells[seconds / 5];
        rec = get_lsp_data(id, NULL);
        ofs[0] = rec->pos.x + pos->x;
        ofs[1] = rec->pos.y + pos->y;
        scale[0] = rec->u_scale / 100.0f;
        scale[1] = rec->v_scale / 100.0f;
        drawshape_init(3, ((74 - seconds) / 15) * 4);
        drawshape_set_vertex_array(cockpit_split_timer_ring_verts);
        drawshape_set_flat_color(rec->color);
        drawshape_set_texture_array(rec->tex_flag, &cockpit_split_timer_ring_uvs[cell]);
        drawshape_set_offset_f32(scale, ofs);
        drawshape_exec();
    }
}

/* 0x802EFF20 (0x204): Draws the quest timer panel at the anchor `kind` picks: blinking red in the last five minutes,
 * then the minute ring and the minutes left of the limit (or the idle face with no limit). */
void cockpit_split_timer_draw(u8 kind) {
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    CockpitWork* work = cockpit_work;
    u32 color;
    s32 minute;
    s32 elapsed;
    s32 left;
    s32 limit;
    u16 id;
    f32 t;

    set_blendmode(4, 5, 1);
    switch (kind) {
    default:
        id = 0xB90;
        break;
    case 1:
        id = 0xCC8;
        break;
    case 2:
        id = 0xCC9;
        break;
    }
    get_lsp_data(id, &pos);
    color = 0x6DAAA3FF;
    if (cockpit_free_play_ck() != 0) {
        elapsed = 0;
        limit = 0;
    } else {
        minute = 60.0f * Screen_w.frame_scale;
        elapsed = quest_time_elapsed_get();
        if (elapsed <= minute * 5) {
            if (elapsed > minute) {
                t = (work->frame_0x190 & 0x1F) << 11;
            } else {
                t = (work->frame_0x190 & 0xF) << 12;
            }
            color = color_lerp(0xF50C23FF, 0xFF8C9BFF, 0.5f * (1.0f + math_sin_rad(9.58738019e-05f * t)));
        }
        left = quest_time_limit_get();
        elapsed = (left - elapsed) / minute;
        limit = left / minute;
    }
    spr_data_copy(&spr, get_lsp_data(0xCDB, NULL));
    spr.color = color;
    draw_sprite(spr, &pos);
    draw_sprite_idx(0xCDC, &pos);
    if (limit != 0) {
        cockpit_timer_ring_draw_at(0xCE0, limit, &pos);
        cockpit_timer_hand_draw(0xCDD, limit, &pos);
        cockpit_timer_hand_draw(0xCDE, elapsed, &pos);
    } else {
        cockpit_timer_ring_draw_at(0xCE0, 0, &pos);
        draw_sprite_idx(0xCDF, &pos);
    }
}

/* 0x802F0124 (0x1C8): Draws the work record's item tag over its player in the split view, through that player's own
 * camera and clamped to the player's half of the screen. */
void cockpit_split_item_tag_draw(CockpitWork* work) {
    VEC3 screen;
    u32 camera;
    _mh_ivec2_ pos;
    s32 handle;
    _PLW* plw = work->plw;
    CockpitItemTag* tag;
    f32 top;
    f32 bottom;

    if (plw->field_0x3E4 != 0) {
        tag = &work->item_tag_0x02C;
        if (tag->tex_idx_0x15 != 0) {
            VEC3_ctor(&screen);
            top = 64.0f;
            bottom = 480.0f;
            handle = ((nw4r::g3d::ScnRoot*)pRoot)->GetCamera(Screen_w.camera_view.player_camera_0x50[plw->chunk_ofs]);
            word_copy_return_dst(&camera, &handle);
            chara_icon_pos_get(plw, 1, &tag->pos);
            camera_world_to_screen(&camera, &screen, &tag->pos);
            if (screen.z < 0.0f || screen.z > 1.0f) {
                return;
            }
            if (plw->chunk_ofs == 0) {
                if (screen_split_view_get() == 0) {
                    bottom = 240.0f;
                }
            } else if (screen_split_view_get() == 0) {
                top = 304.0f;
            }
            screen.x *= hud_screen_width_get();
            screen.y *= 1.07142854f;
            if (screen.y < top) {
                screen.y = top;
            }
            if (screen.y > bottom) {
                screen.y = bottom;
            }
            pos.x = screen.x;
            pos.y = screen.y + tag->ofs_y_0x0F;
            cockpit_item_tag_draw_at(plw, tag, &pos, plw->chunk_ofs);
        }
    }
}

/* The two players' item list anchors (layout ids). */
u16 cockpit_split_item_list_anchors[4] = { 0x0DDC, 0x0DE1, 0x0D82, 0x0D86 };

/* The two players' item ring anchors (layout ids). */
u16 cockpit_split_item_ring_anchors[4] = { 0x0CCE, 0x0CCF, 0x0CD0, 0x0CD1 };

/* 0x802F02EC (0x4F8): `cockpit_item_ring_draw` for player `player`'s split view: the item ring at the player's anchor with the
 * selected item and its two neighbours, the act icons, the name and count, and the use prompt. */
void cockpit_split_item_ring_draw(_PLW* plw, u8* anim, u8 player) {
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    u16 prev;
    u16 next;
    u16 cur;
    s16 count;
    u32 dead;
    const u16* alt;
    const u16* ids;
    const _SPR_DATA_* rec;
    u16 item;
    s16 num;

    set_blendmode(4, 5, 1);
    get_lsp_data(cockpit_split_item_ring_anchors[player], &pos);
    draw_sprite_anim_ary(cockpit_split_item_ring_open_ids, anim[1], &pos);
    draw_sprite_anim_ary(cockpit_split_item_ring_level_ids, anim[3], &pos);
    if (anim[1] != 0) {
        prev = 0xFF;
        next = 0xFF;
        cur = 0xFF;
        count = cockpit_item_slot_live_count(plw);
        dead = cockpit_item_slot_live_ck(plw, plw->field_0x304) == 0;
        if (dead != 0) {
            count++;
        }
        if (dead == 0) {
            cur = plw->field_0x304;
        }
        switch (count) {
        case 0:
        case 1:
            break;
        default:
            next = pl_item_slot_cycle(plw, plw->field_0x304, 1);
            prev = pl_item_slot_cycle(plw, plw->field_0x304, 0);
            break;
        case 4:
            next = pl_item_slot_cycle(plw, plw->field_0x304, 1);
            prev = pl_item_slot_cycle(plw, plw->field_0x304, 0);
            break;
        case 3:
            prev = pl_item_slot_cycle(plw, plw->field_0x304, 0);
        case 2:
            next = pl_item_slot_cycle(plw, plw->field_0x304, 1);
            break;
        }
        alt = NULL;
        if (plw->slot_id[next].item_id == 0xDF) {
            alt = cockpit_split_item_ring_shell_next_ids;
        } else if (plw->slot_id[prev].item_id == 0xDF) {
            alt = cockpit_split_item_ring_shell_prev_ids;
        }
        if (alt == NULL) {
            draw_sprite_anim_ary(cockpit_split_item_ring_frame_ids, anim[1], &pos);
        } else {
            for (ids = cockpit_split_item_ring_frame_ids; *ids != 0xFFFF; ids++, alt++) {
                sprite_frame_apply(&spr, *ids, anim[1], NULL);
                if (*ids != *alt) {
                    rec = get_lsp_data(*alt, NULL);
                    tex_uv_copy(&spr.uv0, &rec->uv0);
                    tex_uv_copy(&spr.uv1, &rec->uv1);
                }
                draw_sprite(spr, &pos);
            }
        }
        cockpit_item_slot_icon_draw(plw, 0xD3C, anim[1], cur, &pos);
        cockpit_item_slot_icon_draw(plw, 0xD44, anim[1], next, &pos);
        cockpit_item_slot_icon_draw(plw, 0xD45, anim[1], prev, &pos);
    } else {
        cockpit_item_slot_icon_draw(plw, 0xD3C, anim[1], plw->field_0x304, &pos);
    }
    cockpit_act_icon_draw(plw, 0xD52, anim[3], 4, &pos);
    cockpit_act_icon_draw(plw, 0xD53, anim[3], 2, &pos);
    draw_sprite_anim_ary(cockpit_split_item_ring_act_ids, anim[3], &pos);
    item = plw->slot_id[plw->field_0x304].item_id;
    if (item != 0 && (item != 0xDF || anim[3] == 3)) {
        draw_font_idx(0xD43, item == 0xDF ? (s8*)get_str_tbl(0x58)[3] : (s8*)ItemName(item), 5, &pos);
        if (GetItemData(item)->max_num_0x003 != 1) {
            draw_sprite_anim_idx(0xD3D, anim[1], &pos);
            num = plw->slot_id[plw->field_0x304].value;
            if (num == GetItemData(item)->max_num_0x003) {
                draw_number_2digit_anim_ary(cockpit_split_item_ring_digit_ids, anim[1], num, 0xF53737FF, &pos);
            } else {
                draw_number_2digit_anim_ary(cockpit_split_item_ring_digit_ids, anim[1], num, 0, &pos);
            }
        }
    }
    if (get_arena_cfg(player, 7) == 0) {
        draw_sprite_anim_idx(0xC37, anim[1], &pos);
    } else {
        draw_sprite_anim_idx(0xC38, anim[1], &pos);
    }
    if (plw->field_0x3E4 == 7 && plw->field_0x3E8 == 0 && item != 0 && item != 0xDF && get_arena_cfg(player, 7) == 0) {
        draw_sprite_anim_idx(0xC39, anim[2], &pos);
        draw_sprite_anim_idx(0xC36, anim[2], &pos);
        if (pl_item_now_usable_ck(plw, item) != 0) {
            set_blendmode(4, 1, 1);
            draw_sprite_anim_idx(0xC35, anim[2], &pos);
            set_blendmode(4, 5, 1);
        }
    }
}

/* 0x802F07E4 (0x1E0): `cockpit_item_list_draw` for player `player`'s split view: the item list at the player's anchor. */
void cockpit_split_item_list_draw(_PLW* plw, u8* anim, u8 player) {
    _mh_ivec2_ pos;
    u16 next;
    u16 prev;
    u8 frame;

    if (pl_item_page2_ck(plw) != 0) {
        set_blendmode(4, 5, 1);
        frame = anim[1] / 10;
        get_lsp_data(cockpit_split_item_list_anchors[player], &pos);
        draw_sprite_anim_ary(cockpit_item_list_frame_ids, frame, &pos);
        if (frame != 0) {
            draw_sprite_anim_ary(cockpit_item_list_page2_ids, frame, &pos);
            cockpit_act_icon_draw(plw, 0xC86, frame, 0x10, &pos);
            cockpit_act_icon_draw(plw, 0xC87, frame, 8, &pos);
        }
        if (frame <= 3) {
            cockpit_item_list_entry_draw(plw, 0xC9A, pos.x, pos.y, plw->item_slot_sel);
        } else {
            cockpit_item_list_entry_draw(plw, 0xC99, pos.x, pos.y, plw->item_slot_sel);
            next = pl_item_slot_next_get(plw, plw->item_slot_sel, 1);
            if (next != 0xFF && next != plw->item_slot_sel) {
                cockpit_item_list_entry_draw(plw, 0xC9A, pos.x, pos.y, next);
            }
            if (frame > 7) {
                prev = pl_item_slot_next_get(plw, plw->item_slot_sel, 0);
                if (prev != 0xFF && prev != plw->item_slot_sel && next != prev) {
                    cockpit_item_list_entry_draw(plw, 0xC98, pos.x, pos.y, prev);
                }
            }
        }
        font_flush();
    }
}

/* The split view's gauge tables: the health shake, the health tick quads (plain and wide) and their texture corners,
 * the stagger flicker offsets; the gauge anchors and the shake scale. */
u16 cockpit_split_gauge_anchors[4] = { 0x0CCA, 0x0CCB, 0x0CCC, 0x0CCD };
s16 cockpit_split_shake_scale = 2;
s16 cockpit_split_shake_table[12] = {
    0, 1, 0, 2, 0, 3, 0, 3, 0, 3, 0, 3,
};
_mh_ivec2_ cockpit_split_health_tick_verts[64] = {
    { 26, 9 }, { 26, 31 }, { 41, 31 }, { 41, 9 },
    { 41, 9 }, { 41, 31 }, { 75, 31 }, { 75, 9 },
    { 75, 9 }, { 75, 31 }, { 109, 31 }, { 109, 9 },
    { 109, 9 }, { 109, 31 }, { 143, 31 }, { 143, 9 },
    { 143, 9 }, { 143, 31 }, { 177, 31 }, { 177, 9 },
    { 177, 9 }, { 177, 31 }, { 211, 31 }, { 211, 9 },
    { 211, 9 }, { 211, 31 }, { 245, 31 }, { 245, 9 },
    { 245, 9 }, { 245, 31 }, { 279, 31 }, { 279, 9 },
    { 279, 9 }, { 279, 31 }, { 313, 31 }, { 313, 9 },
    { 313, 9 }, { 313, 31 }, { 347, 31 }, { 347, 9 },
    { 347, 9 }, { 347, 31 }, { 381, 31 }, { 381, 9 },
    { 381, 9 }, { 381, 31 }, { 415, 31 }, { 415, 9 },
    { 415, 9 }, { 415, 31 }, { 449, 31 }, { 449, 9 },
    { 449, 9 }, { 449, 31 }, { 483, 31 }, { 483, 9 },
    { 483, 9 }, { 483, 31 }, { 517, 31 }, { 517, 9 },
    { 517, 9 }, { 517, 31 }, { 551, 31 }, { 551, 9 },
};
_mh_ivec2_ cockpit_split_health_tick_verts_wide[64] = {
    { 26, 9 }, { 26, 31 }, { 41, 31 }, { 41, 9 },
    { 41, 9 }, { 41, 31 }, { 85, 31 }, { 85, 9 },
    { 85, 9 }, { 85, 31 }, { 129, 31 }, { 129, 9 },
    { 129, 9 }, { 129, 31 }, { 173, 31 }, { 173, 9 },
    { 173, 9 }, { 173, 31 }, { 217, 31 }, { 217, 9 },
    { 217, 9 }, { 217, 31 }, { 261, 31 }, { 261, 9 },
    { 261, 9 }, { 261, 31 }, { 305, 31 }, { 305, 9 },
    { 305, 9 }, { 305, 31 }, { 349, 31 }, { 349, 9 },
    { 349, 9 }, { 349, 31 }, { 393, 31 }, { 393, 9 },
    { 393, 9 }, { 393, 31 }, { 437, 31 }, { 437, 9 },
    { 437, 9 }, { 437, 31 }, { 481, 31 }, { 481, 9 },
    { 481, 9 }, { 481, 31 }, { 525, 31 }, { 525, 9 },
    { 525, 9 }, { 525, 31 }, { 569, 31 }, { 569, 9 },
    { 569, 9 }, { 569, 31 }, { 613, 31 }, { 613, 9 },
    { 613, 9 }, { 613, 31 }, { 657, 31 }, { 657, 9 },
    { 657, 9 }, { 657, 31 }, { 701, 31 }, { 701, 9 },
};
_mh_tex_uv_ cockpit_split_health_tick_uvs[64] = {
    { 84, 1 }, { 84, 23 }, { 99, 23 }, { 99, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
};
s16 cockpit_split_flicker_ofs_hit[52] = {
    0, 2, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
s16 cockpit_split_flicker_ofs_idle[26] = {
    0, 2, -2, 1, -1, 1, -1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/* 0x802F09C4 (0x8E0): `quest_gauge_draw` for player `player`'s split view: the stamina, health and stagger gauges
 * at the player's anchor, with the split view's own tick and flicker tables. */
void cockpit_split_gauge_draw(CockpitWork* work, u8 player) {
    _PLW* plw = work->plw;
    s32 wide = ck_WideMode();
    _DRAW_RECT_2D_TEX rect;
    _mh_ivec2_ quad[6];
    _mh_tex_uv_ uvs[6];
    u32 colors[6];
    _mh_ivec2_ spot;
    _mh_ivec2_ flash_pos;
    _mh_ivec2_ corner;
    _mh_ivec2_ pos;
    s32 copy;
    _mh_tex_uv_ uv_a;
    s32 anchor_a;
    s32 anchor_b;
    _SPR_DATA_* rec;
    s16 cell;
    u32 cells;
    s32 tick_end;
    u16 scaled_max;
    u16 shown;
    u16 top;
    s16 shake;
    f32 fill;
    s8 count;

    set_blendmode(4, 5, 1);
    get_lsp_data(cockpit_split_gauge_anchors[player], &spot);
    draw_sprite_idx(0xCEC, &spot);
    pos.y = 0;
    pos.x = 0;
    rec = get_lsp_data(0xCE9, NULL);
    if (wide == 0) {
        cell = rec->width;
    } else {
        cell = 0x6E;
    }
    cells = plw->field_0x37A / 150;
    corner.x = rec->pos.x + spot.x;
    corner.y = rec->pos.y + spot.y;
    draw_rect_2d_tex_init(&rect, &corner.x, (u16)(cell * cells), rec->height, rec->color, &rec->uv0.u, &rec->uv1.u);
    draw_rect_2d_tex_by_id(&rect, rec->tex_flag);
    pos.x = (s16)(cell * (cells - 1));
    pos.x += spot.x;
    pos.y += spot.y;
    if ((u32)wide == 1) {
        pos.x += 0x19;
    }
    draw_sprite_idx(0xCEA, &pos);
    uv_pair_copy(&copy, &pos);
    if (wide == 0) {
        quest_gauge_bar_draw(0xCEB, plw->field_0x378, 0, plw->field_0x37A, 0x384, &work->bar_b_0x154, &spot, 0);
    } else {
        quest_gauge_bar_draw(0xCEB, plw->field_0x378, 0, plw->field_0x37A, 0x384, &work->bar_b_0x154, &spot,
                             0x294);
    }
    if (plw->field_0x37A >= 0x384) {
        if (work->sparkle_b_frame_0x18E < 0x60) {
            work->sparkle_b_frame_0x18E++;
            anchor_a = copy;
            put_lsp_anchor_offset(0xCEF, &flash_pos, &anchor_a);
            draw_sprite_anim_idx(0xCF0, work->sparkle_b_frame_0x18E, &flash_pos);
        }
    } else {
        work->sparkle_b_frame_0x18E = 0;
    }
    pos.y = 0;
    pos.x = 0;
    if (work->health_hit_clock_0x13A != 0) {
        pos.y = cockpit_split_shake_scale * cockpit_split_shake_table[work->health_hit_clock_0x13A];
    }
    pos.x += spot.x;
    pos.y += spot.y;
    tick_end = plw->health_max / 10;
    if (tick_end > 0xF) {
        tick_end = 0xF;
    }
    tick_end *= 4;
    drawshape_init(3, (u16)(tick_end + 4));
    drawshape_set_vertex_array(wide != 0 ? cockpit_split_health_tick_verts_wide : cockpit_split_health_tick_verts);
    drawshape_set_flat_color(-1);
    drawshape_set_texture_array(0xD, cockpit_split_health_tick_uvs);
    drawshape_set_offset_ivec2(&pos.x);
    drawshape_exec();
    if (wide == 0) {
        pos.x += (s16)(cockpit_split_health_tick_verts[tick_end].x - cockpit_split_health_tick_verts[4].x);
    } else {
        pos.x += (s16)(cockpit_split_health_tick_verts_wide[tick_end].x - cockpit_split_health_tick_verts_wide[4].x);
        pos.x += 0xA;
    }
    draw_sprite_idx(0xCE6, &pos);
    uv_pair_copy(&copy, &pos);
    pos.x = spot.x;
    if (wide == 0) {
        quest_gauge_bar_draw(0xCE7, plw->health, plw->field_0x376, plw->health_max, 0x96, &work->bar_a_0x144,
                             &pos, 0);
    } else {
        quest_gauge_bar_draw(0xCE7, plw->health, plw->field_0x376, plw->health_max, 0x96, &work->bar_a_0x144,
                             &pos, 0x294);
    }
    if (plw->health_max >= 0x96) {
        if (work->sparkle_a_frame_0x18C < 0x60) {
            work->sparkle_a_frame_0x18C++;
            anchor_b = copy;
            put_lsp_anchor_offset(0xCED, &flash_pos, &anchor_b);
            draw_sprite_anim_idx(0xCEE, work->sparkle_a_frame_0x18C, &flash_pos);
        }
    } else {
        work->sparkle_a_frame_0x18C = 0;
    }
    rec = get_lsp_data(0xCF7, NULL);
    spot.x += rec->pos.x;
    spot.y += rec->pos.y;
    scaled_max = (plw->field_0x380 * 0x84) / 150;
    shown = (work->stagger_shown_0x134 * 0x84) / 150;
    top = scaled_max - 0x11;
    if (plw->kind_0x09 != 3) {
        draw_sprite_idx(0xCFB, &spot);
        rec = get_lsp_data(0xCFC, NULL);
        corner.x = rec->pos.x + spot.x;
        corner.y = rec->pos.y + spot.y;
        draw_rect_2d_tex_init(&rect, &corner.x, rec->width, top, rec->color, &rec->uv0.u, &rec->uv1.u);
        draw_rect_2d_tex_by_id(&rect, rec->tex_flag);
        pos.x = spot.x;
        pos.y = spot.y + (top - rec->height);
        draw_sprite_idx(0xCFD, &pos);
    } else {
        if (plw->field_0x37E != 0) {
            shake = cockpit_split_flicker_ofs_hit[work->stagger_shake_idx_0x137];
        } else {
            shake = cockpit_split_flicker_ofs_idle[work->stagger_shake_idx_0x137];
        }
        pos.x = shake;
        pos.x += spot.x;
        pos.y = spot.y;
        draw_sprite_idx(0xCF8, &pos);
        rec = get_lsp_data(0xCFA, NULL);
        corner.x = rec->pos.x + pos.x;
        corner.y = rec->pos.y + pos.y;
        draw_rect_2d_tex_init(&rect, &corner.x, rec->width, rec->height, rec->color, &rec->uv0.u, &rec->uv1.u);
        rec = get_lsp_data(0xCF9, NULL);
        rect.y = rec->pos.y + pos.y;
        rect.end_y += (s16)(top - rec->height);
        rect.v0 -= top;
        draw_rect_2d_tex_by_id(&rect, rec->tex_flag);
    }
    if (work->stagger_flash_0x136 != 0) {
        rec = get_lsp_data(0xCFE, NULL);
        flash_pos.x = rec->pos.x;
        flash_pos.y = (rec->pos.y + scaled_max) - shown;
        uv_a.u = rec->uv0.u;
        uv_a.v = rec->uv1.v - shown;
        if (work->stagger_shake_idx_0x137 != 0) {
            flash_pos.x += spot.x;
            flash_pos.y += spot.y;
            draw_rect_2d_tex_init(&rect, &flash_pos.x, rec->width, shown,
                                  work->stagger_flash_0x136 | 0xC1183600, &uv_a.u, &rec->uv1.u);
            draw_rect_2d_tex_by_id(&rect, rec->tex_flag);
            return;
        }
        flash_pos.x += spot.x;
        flash_pos.y += spot.y;
        fill = (2.0f * (f32)work->stagger_shown_0x134) / (f32)plw->field_0x380;
        quad[0].x = quad[2].x = flash_pos.x;
        quad[1].x = quad[3].x = flash_pos.x + rec->width;
        quad[0].y = quad[1].y = flash_pos.y;
        quad[2].y = quad[3].y = flash_pos.y + shown;
        uvs[0].u = uvs[2].u = rec->uv0.u;
        uvs[1].u = uvs[3].u = rec->uv1.u;
        uvs[0].v = uvs[1].v = uv_a.v;
        if (fill <= 1.0f) {
            colors[0] = colors[1] = work->stagger_flash_0x136 | color_lerp(0x93DDF500, 0x16D9D000, fill);
            colors[2] = colors[3] = work->stagger_flash_0x136 | 0x93DDF500;
            uvs[2].v = uvs[3].v = rec->uv1.v;
            count = 4;
        } else {
            s32 half;
            quad[4].x = flash_pos.x;
            quad[5].x = quad[1].x;
            quad[4].y = quad[5].y = quad[3].y;
            half = scaled_max / 2;
            quad[2].y = quad[3].y = quad[3].y - half;
            colors[0] = colors[1] = work->stagger_flash_0x136 | color_lerp(0x16D9D000, 0xB79200, fill - 1.0f);
            colors[2] = colors[3] = work->stagger_flash_0x136 | 0x16D9D000;
            colors[4] = colors[5] = work->stagger_flash_0x136 | 0x93DDF500;
            uvs[4].u = uvs[0].u;
            uvs[5].u = uvs[1].u;
            uvs[2].v = uvs[3].v = rec->uv1.v - half;
            uvs[4].v = uvs[5].v = rec->uv1.v;
            count = 6;
        }
        drawshape_init(5, count);
        drawshape_set_vertex_array(quad);
        drawshape_set_color_array(colors);
        drawshape_set_texture_array(rec->tex_flag, uvs);
        drawshape_exec();
    }
}

/* The weapon-class frame per player and class, the class glow colours, and per player the glow and condition ids. */
u16 cockpit_weapon_frame_ids[2][7] = {
    { 0x0BA9, 0x0BA8, 0x0BA7, 0x0BA7, 0x0BA7, 0x0BA7, 0x0BA7 },
    { 0x0CF4, 0x0CF3, 0x0CF2, 0x0CF2, 0x0CF2, 0x0CF2, 0x0CF2 },
};
u32 cockpit_weapon_glow_colors[8] = {
    0xFF0000FF, 0x00000000, 0x00000000, 0x5ED300FF, 0x3068EEFF, 0xF0F0F0FF, 0xF000F0FF, 0x00000000,
};
CockpitWeaponIds cockpit_weapon_ids[2] = {
    { 0x0BAA, 0x0BAB },
    { 0x0CF5, 0x0CF6 },
};

/* 0x802F12A4 (0x168): Draws the weapon-class frame at `pos`: the class's frame, its pulsing glow and the condition
 * marker. */
void cockpit_weapon_frame_draw(CockpitWork* work, const _mh_ivec2_* pos, u8 player) {
    _SPR_DATA_ spr;
    _PLW* plw = work->plw;
    u8 kind = plw->field_0x56B;
    u32 glow;
    u32 alpha;

    draw_sprite_idx(cockpit_weapon_frame_ids[player][kind], pos);
    glow = cockpit_weapon_glow_colors[kind];
    if (glow != 0) {
        set_blendmode(4, 1, 1);
        alpha = 159 + (s8)(96.0f * math_sin_rad(9.58738019e-05f * (u16)((work->frame_0x190 & 0x7F) << 9))) + glow;
        spr_data_copy(&spr, get_lsp_data(cockpit_weapon_ids[player].glow_id, NULL));
        spr.color = alpha;
        draw_sprite(spr, pos);
        set_blendmode(4, 5, 1);
    }
    if (Pl_dm_condition_ck(plw, 0x300) != 0) {
        draw_sprite_anim_idx(cockpit_weapon_ids[player].cond_id, work->frame_0x190, pos);
    }
}

/* Per player the stagger gauge's flash animation, frame and bar ids; the flash colour per stagger level. */
CockpitStaggerIds cockpit_stagger_ids[2] = {
    { 0x0BCC, 0x0BCB, 0x0BCD },
    { 0x0D03, 0x0D02, 0x0D04 },
};
u32 cockpit_stagger_flash_colors[4] = { 0x00000000, 0xFAFAFA00, 0xF5F59100, 0xF596CD00 };

/* 0x802F140C (0x2B4): Draws the stagger gauge (at its own anchor for the first view, else offset from `pos`): the
 * level's flash, the frame and the bar, filled to the stagger value and pulsing white while the player is dazed. */
void cockpit_stagger_gauge_draw(CockpitWork* work, const _mh_ivec2_* pos, u8 player) {
    _mh_ivec2_ at;
    _SPR_DATA_ spr;
    CockpitState* state = &cockpit_state;
    _PLW* plw = work->plw;
    const _SPR_DATA_* rec;
    u32 flash;
    u32 near_color;
    u32 far_color;
    u32 color;
    u16 width;
    f32 t;

    if (player == 0) {
        get_lsp_data(0xBCA, &at);
    } else {
        rec = get_lsp_data(0xD01, NULL);
        at.x = pos->x + rec->pos.x;
        at.y = pos->y + rec->pos.y;
    }
    flash = cockpit_stagger_flash_colors[(u8)pl_stagger_level_get(plw)];
    if (flash != 0) {
        sprite_frame_apply(&spr, cockpit_stagger_ids[player].flash_id,
                           state->field_0x078 % (sprite_anim_last_frame(cockpit_stagger_ids[player].flash_id) + 1), NULL);
        spr.color = (u8)spr.color + flash;
        draw_sprite(spr, &at);
    }
    draw_sprite_idx(cockpit_stagger_ids[player].frame_id, &at);
    near_color = 0xE92412FF;
    far_color = 0xEB7416FF;
    if (plw->daze_timer_0x468 > 0) {
        t = 0.5f * (1.0f + math_sin_rad(9.58738019e-05f * (u16)((state->field_0x078 & 0x7F) << 9)));
        near_color = color_lerp(0xF4F4F4FF, 0xE92412FF, t);
        far_color = color_lerp(0xF4F4F4FF, 0xEB7416FF, t);
    }
    rec = get_lsp_data(cockpit_stagger_ids[player].bar_id, NULL);
    t = 0.00999999978f * plw->field_0x384;
    width = (rec->width * plw->field_0x384) / 100;
    color = color_lerp(near_color, far_color, t);
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect((s16)(rec->pos.x + at.x), (s16)(rec->pos.y + at.y), (s16)width, rec->height);
    drawshape_set_corner_colors(near_color, color, color, near_color);
    drawshape_exec();
}

/* The charge gauge's frame animation for the first and the second view (sprite ids, 0xFFFF-terminated). */
u16 cockpit_charge_frame_ids[12] = {
    0x0C16, 0x0C17, 0x0C14, 0x0C15, 0x0C1E, 0x0C18, 0x0C19, 0x0C1A, 0x0C1B, 0x0C1C, 0xFFFF, 0x0000,
};
u16 cockpit_charge_frame_ids_split[12] = {
    0x0D2F, 0x0D30, 0x0D2D, 0x0D2E, 0x0D37, 0x0D31, 0x0D32, 0x0D33, 0x0D34, 0x0D35, 0xFFFF, 0x0000,
};

/* 0x802F16C0 (0x284): Draws the charge gauge at `pos`: the frame at the gauge level, the bar filled to the shown
 * value (gold while the level is low, red otherwise), and the flashing cap while the gauge is nearly full. */
#pragma fp_contract off
void cockpit_charge_gauge_draw(CockpitWork* work, const _mh_ivec2_* pos, u8 player) {
    _mh_ivec2_ at;
    _PLW* plw = work->plw;
    const _SPR_DATA_* rec;
    u32 color;
    u32 dark;
    u32 light;
    u16 width;
    u16 frame;
    u16 count;
    f32 t;

    at.x = pos->x;
    at.y = pos->y;
    draw_sprite_anim_ary(player != 0 ? cockpit_charge_frame_ids_split : cockpit_charge_frame_ids, work->gauge_level_0x16A, &at);
    rec = get_lsp_data(player != 0 ? 0xD36 : 0xC1D, NULL);
    t = 0.00999999978f * work->gauge_value_0x168;
    width = (rec->width * work->gauge_value_0x168) / 100;
    if (cockpit_gauge_low_ck(plw) != 0) {
        dark = 0xF1C516FF;
        light = 0xFAE632FF;
    } else {
        dark = 0xCF3169FF;
        light = 0xEC3E7CFF;
    }
    color = color_lerp(dark, light, t);
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect((s16)(rec->pos.x + at.x), (s16)(rec->pos.y + at.y), (s16)width, rec->height);
    drawshape_set_corner_colors(dark, color, color, dark);
    drawshape_exec();
    if (pl_mode8_alive_ck(plw) != 0 && plw->field_0x36C < 100) {
        count = sprite_anim_last_frame(player != 0 ? 0xD38 : 0xC1F) + 1;
        frame = work->frame_0x190 % count;
        t = 1.0f + 0.699999988f * (0.5f * (1.0f + math_sin_rad(6.28318548f * ((f32)frame / (f32)count))));
        color_scale(dark, t, 1.0f);
        color_scale(light, t, 1.0f);
        if (pl_charge_flash_ck(plw) != 0) {
            draw_sprite_anim_idx(player != 0 ? 0xD38 : 0xC1F, frame, &at);
        }
    }
}
#pragma fp_contract on

/* The two players' weapon frame anchors (layout ids). */
u16 cockpit_split_weapon_anchors[4] = { 0x0CCA, 0x0CCB, 0x0CCC, 0x0CCD };

/* 0x802F1944 (0x1D8): `cockpit_weapon_hud_draw` for player `player`'s split view: the aim view, the lock-on marker and the ammo
 * gauge on the second item page; the weapon frame and its stagger or charge gauge on the first. */
void cockpit_split_weapon_hud_draw(CockpitWork* work, u8 player) {
    _mh_ivec2_ frame_pos;
    _mh_ivec2_ aim_pos;
    _mh_ivec2_ gauge;
    _PLW* plw;
    const _SPR_DATA_* rec;

    set_blendmode(4, 5, 1);
    plw = work->plw;
    if (pl_item_page2_ck(plw) == 1) {
        if (camera_mode1_ck_player(player) == 1) {
            cockpit_split_aim_view_draw(work, player);
            return;
        }
        if (camera_work_ck_player(player) == 0 && (u32)pl_target_lock_ck(plw) == 1 && pl_aim_act_ck(plw) == 1) {
            cockpit_eye_marker_draw_split(plw);
        }
        if (cockpit_hud_hidden_ck(plw, player) == 1) {
            if (player == 0) {
                get_lsp_data(ck_WideMode() != 0 ? 0xE11 : 0xDE2, &aim_pos);
            } else {
                get_lsp_data(ck_WideMode() != 0 ? 0xE12 : 0xDE3, &aim_pos);
            }
            cockpit_split_ammo_gauge_draw(plw, aim_pos.x, aim_pos.y, 0);
        }
    } else if (cockpit_hud_hidden_ck(plw, player) != 0) {
        get_lsp_data(cockpit_split_weapon_anchors[player], &frame_pos);
        cockpit_weapon_frame_draw(work, &frame_pos, 1);
        switch (plw->field_0x002) {
        case 7:
            cockpit_stagger_gauge_draw(work, &frame_pos, 1);
            break;
        case 8:
            rec = get_lsp_data(0xD2C, NULL);
            gauge.x = rec->pos.x + frame_pos.x;
            gauge.y = rec->pos.y + frame_pos.y;
            cockpit_charge_gauge_draw(work, &gauge, 1);
            break;
        }
    }
}

/* 0x802F1B1C (0x1DC): `cockpit_aim_view_draw` for player `player`'s split view: the scope frame and its animation with the
 * ammo gauge while aiming, and the zoom ring. */
void cockpit_split_aim_view_draw(CockpitWork* work, u8 player) {
    _mh_ivec2_ pos;
    _mh_ivec2_ gauge;
    f32 zoom;
    u8 aiming;
    u8 zooming;
    _PLW* plw = work->plw;
    s32 wide = ck_WideMode();

    camera_aim_zoom_get_player(plw, &aiming, &zooming, &zoom, player);
    if (player == 0) {
        get_lsp_data(wide != 0 ? 0xDE4 : 0xDB1, &pos);
    } else {
        get_lsp_data(wide != 0 ? 0xE0F : 0xDDE, &pos);
    }
    if (aiming != 0) {
        draw_sprite_ary(wide != 0 ? cockpit_split_scope_frame_ids_wide : cockpit_split_scope_frame_ids, &pos);
        draw_sprite_anim_ary(wide != 0 ? cockpit_split_scope_anim_ids_wide : cockpit_split_scope_anim_ids, work->gauge_frame_0x167, &pos);
        if (player == 0) {
            get_lsp_data(wide != 0 ? 0xE09 : 0xDD6, &gauge);
            cockpit_split_ammo_gauge_draw(plw, gauge.x, gauge.y, 1);
        } else {
            get_lsp_data(wide != 0 ? 0xE10 : 0xDDF, &gauge);
            cockpit_split_ammo_gauge_draw(plw, gauge.x, gauge.y, 1);
        }
    }
    if (zooming != 0) {
        zoom *= (f32)sprite_anim_last_frame(wide != 0 ? 0xDF8 : 0xDC5);
        draw_sprite_anim_ary(wide != 0 ? cockpit_split_zoom_ring_ids_wide : cockpit_split_zoom_ring_ids, (u16)zoom, &pos);
    }
}

/* The split view's ammo gauge state icons (empty, reloading, idle), plain and wide. */
u16 cockpit_split_ammo_icons[3] = { 0x0DD9, 0x0DDA, 0x0DDB };
u16 cockpit_split_ammo_icons_wide[3] = { 0x0E0C, 0x0E0D, 0x0E0E };

/* 0x802F1CF8 (0x23C): `cockpit_ammo_gauge_draw` for the split view: the ammo gauge of the player's own cockpit record, with the
 * wide layout's cells. */
void cockpit_split_ammo_gauge_draw(_PLW* plw, s16 x, s16 y, u8 aim) {
    _mh_ivec2_ pos;
    _mh_ivec2_ start;
    s16 step;
    const u16* icons;
    s32 wide;
    CockpitWork* work = &cockpit_work[plw->chunk_ofs];
    u8 capacity;
    u8 loaded;
    s32 i;

    wide = ck_WideMode();
    icons = wide != 0 ? cockpit_split_ammo_icons_wide : cockpit_split_ammo_icons;
    pos.x = x;
    pos.y = y;
    if (plw->magazine_0x270 == 0) {
        if (plw->field_0x018 == 1 && pl_item_slot_next_get(plw, plw->item_slot_sel, 1) != 0xFF) {
            draw_sprite_anim_idx(icons[2], work->frame_0x190, &pos);
        }
    } else {
        capacity = plw->ammo_capacity_0x26A;
        loaded = plw->field_0x269;
        step = get_lsp_data(wide != 0 ? 0xE0A : 0xDD7, NULL)->width;
        if (aim != 0) {
            pos.x = (s32)(320.0f * hud_screen_width_get()) - (capacity * step) / 2;
        }
        uv_pair_copy(&start, &pos);
        for (i = 0; i < capacity; i++) {
            if (work->item_frame_0x166 != 0 && i == loaded) {
                draw_sprite_anim_ary(cockpit_ammo_fire_ids, work->item_frame_0x166, &pos);
            } else if (i < loaded) {
                draw_sprite_anim_idx(wide != 0 ? 0xE0A : 0xDD7, 0, &pos);
            } else {
                draw_sprite_anim_idx(wide != 0 ? 0xE0B : 0xDD8, 100, &pos);
            }
            pos.x += step;
        }
        if (Pl_dm_condition_ck(plw, 0x300) != 0) {
            draw_sprite_anim_idx(0xC74, work->frame_0x190, &start);
        }
        pos.x = x;
        pos.y = y;
        if (pl_reload_motion_ck(plw) != 0) {
            draw_sprite_anim_idx(icons[1], work->frame_0x190, &pos);
        } else if (loaded == 0 && plw->field_0x018 == 1) {
            draw_sprite_anim_idx(icons[0], work->frame_0x190, &pos);
        }
    }
}

/* The two players' flash gauge anchors (layout ids). */
u16 cockpit_split_flash_anchors[4] = { 0x0CD6, 0x0CD7, 0x0CD8, 0x0CD9 };

/* 0x802F1F34 (0x1A4): Draws player `player`'s flash gauge while act kind 7 charges: the frame animation and the bar,
 * filled and coloured by the charge over its limit. */
void cockpit_split_flash_gauge_draw(CockpitWork* work, u8 player) {
    _mh_ivec2_ pos;
    _PLW* plw = work->plw;
    const _SPR_DATA_* rec;
    u32 color;
    s16 width;
    f32 t;

    if (plw->field_0x00A == 7 && plw->field_0x3A4 != 0) {
        set_blendmode(4, 5, 1);
        get_lsp_data(cockpit_split_flash_anchors[player], &pos);
        draw_sprite_anim_ary(cockpit_flash_anim_ids, work->flash_frame_0x180, &pos);
        rec = get_lsp_data(0xBD2, NULL);
        t = plw->field_0x3A4 * (1.0f / plw->field_0x3A5);
        width = (rec->width * (f32)plw->field_0x3A4) / plw->field_0x3A5;
        color = color_lerp(0xC50A83FF, 0xD813BEFF, t);
        drawshape_init(3, 0xFFFF);
        drawshape_set_vertex_rect((s16)(rec->pos.x + pos.x), (s16)(rec->pos.y + pos.y), width, rec->height);
        drawshape_set_corner_colors(0xC50A83FF, color, color, 0xC50A83FF);
        drawshape_exec();
    }
}

/* 0x802F20D8 (0x48): The unit's static initialiser: constructs both cockpit work records and the cockpit state. */
void cockpit_static_init(void) {
    __construct_array(cockpit_work, (void*)cockpit_work_construct, NULL, sizeof(CockpitWork), 2);
    cockpit_state_construct(&cockpit_state);
}

/* 0x802F2120 (0x58): Constructs the cockpit state: the positions of its four pit traps. */
CockpitState* cockpit_state_construct(CockpitState* state) {
    PitTrap* trap = state->pit_traps_0x018;

    do {
        pit_trap_construct(trap);
        trap++;
    } while (trap < &state->pit_traps_0x018[4]);
    return state;
}

/* 0x802F2178 (0x30): Constructs one pit trap record (its position). */
PitTrap* pit_trap_construct(PitTrap* trap) {
    VEC3_ctor(&trap->pos);
    return trap;
}

/* 0x802F21A8 (0x60): Constructs one cockpit work record: its item tag and its four marker positions. */
CockpitWork* cockpit_work_construct(CockpitWork* work) {
    VEC3* pos;

    cockpit_item_tag_construct(&work->item_tag_0x02C);
    pos = work->name_pos_0x054;
    do {
        VEC3_ctor(pos);
        pos++;
    } while (pos < &work->name_pos_0x054[4]);
    return work;
}

/* 0x802F2208 (0x30): Constructs one item tag (its world position). */
CockpitItemTag* cockpit_item_tag_construct(CockpitItemTag* tag) {
    VEC3_ctor(&tag->pos);
    return tag;
}

/* 0x802EE288 (0xA8): Draws the item icon of slot `slot` (none for 0xFF, an empty or a 0xDF slot), crossed out in red
 * when `pl_item_now_usable_ck` says the item cannot be used. */
void cockpit_item_slot_icon_draw(_PLW* plw, u16 id, u8 part, u16 slot, const _mh_ivec2_* pos) {
    u16 item;

    if (slot != 0xFF) {
        item = plw->slot_id[slot].item_id;
        if (item != 0 && item != 0xDF) {
            draw_itemicon_anim_idx(id, part, item, pos);
            if (pl_item_now_usable_ck(plw, item) == 0) {
                draw_itemicon_frame_idx(id, part, 0x71, 0xFF0000FF, pos);
            }
        }
    }
}


/* 0x802EC6C4 (0x3C).  A one-bit predicate over `pl_gauge_level_get`'s byte: the branch-free
 * `subfic`/`orc`/`subf` sequence is the target's own shape. */
u32 cockpit_gauge_low_ck(_PLW* plw)
{
    return pl_gauge_level_get(plw) <= 1;
}

/* 0x802EDAF0 (0x1C).  Zero a 4-byte run and the second byte of a following flag record - the
 * caller (`cockpit_work_init`) hands the two pointers into its 0x194 record. */
void cockpit_item_ring_reset(_PLW* plw, u8* a, u8* b)
{
    (void)plw;
    a[0] = 0;
    a[1] = 0;
    a[2] = 0;
    a[3] = 0;
    b[1] = 0;
}

/* 0x802ED588 (0x98).  Fold the two 0x3D8/0x3DC act bitfields through the two 4-byte mask tables
 * (the `kind` selects the table pair) into one bitmask: bit `n` is raised when the n-th non-zero
 * mask word is set in the field. */
s32 cockpit_act_icon_mask_get(_PLW* plw, u8 kind)
{
    s32 bit;
    s32 result;
    const u32* a;
    const u32* b;
    u32 flags;

    result = 0;
    bit = 1;
    if (kind == 0) {
        a = cockpit_act_mask_a0;
        b = cockpit_act_mask_b0;
    } else {
        a = cockpit_act_mask_a1;
        b = cockpit_act_mask_b1;
    }
    flags = plw->field_0x3D8;
    for (;;) {
        if (*a == 0) {
            break;
        }
        if ((flags & *a) != 0) {
            result |= bit;
        }
        bit <<= 1;
        a++;
    }
    flags = plw->field_0x3DC;
    for (;;) {
        if (*b == 0) {
            break;
        }
        if ((flags & *b) != 0) {
            result |= bit;
        }
        bit <<= 1;
        b++;
    }
    return result;
}

/* 0x802ED6F4 (0xF0).  Advance the two-byte cursor over the caller's bit table: while `timer` is
 * negative, arm on the lowest set bit; while it is non-negative, keep counting down on the same
 * bit and, once it runs out, arm on the next set bit (wrapping at `count`). */
void cockpit_cursor_step(u32 mask, s32 count, Cursor2* cur)
{
    if (mask == 0) {
        cur->timer = -1;
        return;
    }
    if (cur->timer < 0) {
        for (s32 i = 0; i < count; i++) {
            if ((mask & (1u << i)) != 0) {
                cur->index = i;
                cur->timer = 20;
                return;
            }
        }
        return;
    }
    if ((mask & (1u << cur->index)) != 0 && cur->timer > 0) {
        cur->timer -= 1;
        return;
    }
    cur->timer = -1;
    for (s32 i = 0; i < count; i++) {
        cur->index = cur->index + 1;
        if (cur->index >= count) {
            cur->index = 0;
        }
        if ((mask & (1u << cur->index)) != 0) {
            cur->timer = 20;
            return;
        }
    }
}

/* 0x802ED7E4 (0x50).  The 18/19-entry cursor's `kind == 0` lumping: mask the act fields, then arm
 * the cursor with the count the kind selects. */
void cockpit_act_cursor_step(_PLW* plw, Cursor2* cur, u8 kind)
{
    u8 k = kind;
    s32 count = (k == 0) ? 19 : 18;
    cockpit_cursor_step((u32)cockpit_act_icon_mask_get(plw, k), count, cur);
}

/* 0x802ED834 (0x58).  The second selector's 4/6-entry cursor. */
void cockpit_npc_cursor_step(_AINPC_W* npc, Cursor2* cur, u8 kind)
{
    cockpit_cursor_step((u32)cockpit_npc_icon_mask_get(npc, kind), kind == 0 ? 6 : 4, cur);
}

/* 0x802EECA0 (0x6C).  Count the live equipment slots (0..25) `cockpit_item_slot_live_ck` accepts. */
s16 cockpit_item_slot_live_count(_PLW* plw)
{
    s16 count = 0;
    u32 i = 0;
    do {
        if (cockpit_item_slot_live_ck(plw, i) != 0) {
            count = count + 1;
        }
        i = i + 1;
    } while (i < 26);
    return count;
}

/* 0x802EEC24 (0x7C).  A slot is "live" when its value is positive, it holds an item, the item
 * passes `item_category_ck`'s flag test and its record's first byte is not 1. */
s32 cockpit_item_slot_live_ck(_PLW* plw, u16 idx)
{
    _SLOTENT* slot = &plw->slot_id[idx];
    u16 item;

    if (slot->value > 0) {
        item = slot->item_id;
        if (item != 0) {
            if (item_category_ck(item, 8) != 0) {
                if (GetItemData(item)->kind_0x00 != 1) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* 0x802EE330 (0x7C).  One cockpit icon: when the act's status byte overlaps the caller's mask,
 * draw the position record through the 2D library in the fixed 0xFFE300FF colour, else play the
 * icon's animation. */
void cockpit_act_icon_draw(_PLW* plw, u16 id, u8 anim, u8 mask, const _mh_ivec2_* pos)
{
    if ((plw->field_0x309 & mask) != 0) {
        _SPR_DATA_ spr;
        sprite_frame_apply(&spr, id, anim, 0);
        spr.color = 0xFFE300FF;
        draw_sprite(spr, pos);
    } else {
        draw_sprite_anim_idx(id, anim, pos);
    }
}

/* 0x802EF0A0 (0x5C).  Project the caller's position through `eft053_shell_pos_project` and hand the
 * 12-byte result to the same-file `cockpit_aim_marker_draw` (the three word copies are the target's own shape). */
void cockpit_shell_marker_draw(_PLW* plw)
{
    VEC3 pos;

    VEC3_ctor(&pos);
    eft053_shell_pos_project(plw, &pos);
    cockpit_aim_marker_draw(pos);
}

/* 0x802EF62C (0x84).  The quest HUD's gate: off while `menu_item_frame_update(0)` or `camera_mode1_ck()` hold,
 * on only in area 1 with the act's state byte at 0 or 0xE. */
s32 cockpit_prompt_ck(_PLW* plw)
{
    if (menu_item_frame_update(0) != 0) {
        return 0;
    }
    if (camera_mode1_ck() == 1) {
        return 0;
    }

    switch (plw->field_0x3E4) {
    case 0:
    case 0xE:
        if (get_now_areano() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x802EF6B0 (0x80).  Arm/decay the blink record after the two quest work records: zero it when
 * the quest gate `cockpit_prompt_ck` is closed, else reload a 10-frame countdown from `quest_move_sub_flag_get` and
 * tick it down. */
void cockpit_prompt_update(void)
{
    CockpitState* blink = &cockpit_state;

    if (cockpit_prompt_ck(cockpit_work[0].plw) == 0) {
        blink->blink_timer_0x074 = 0;
        return;
    }

    u8 v = quest_move_sub_flag_get();
    if (v != 0) {
        blink->blink_timer_0x074 = 10;
        blink->blink_flag_0x075 = v;
        return;
    }
    if (blink->blink_timer_0x074 != 0) {
        blink->blink_timer_0x074 -= 1;
    }
}

/* 0x802EF400 (0x24).  Copy one 5-byte selection record field by field (the widths are the target's
 * `lbz`/`lbz`/`lhz`/`lbz`). */
void cockpit_hunt_log_entry_copy(HuntLogEntry* dst, const HuntLogEntry* src)
{
    dst->name_id = src->name_id;
    dst->crown = src->crown;
    dst->count = src->count;
    dst->frame = src->frame;
}

