/*
 * menu/get_pop_dat_ptr.cpp - the pop-data / option / lobby-screen file: the per-map pop file loads
 *   (`m%03d_%06d_c_pop.dat`, `06/..._f_pop.dat`, `06/..._r_pop.dat`) into the move work and the enemy roster, the option
 *   table `option_w` and its arena-profile twin, the lobby's room-visit screen (0x1F) and the screens 0x18/0x1E/0x22
 *   with their draws, the em020/em025 effect helpers and the demo words.  C++ (the range defines `get_pop_dat_ptr__Fv`,
 *   `ck_option_cfg__FUc`, `get_option_cfg__FUc`, `get_arena_cfg__FUcUc`, `set_option_cfg__FUcUc`, `set_def_option__Fv`,
 *   `set_option_mode__Fv`); the plain-spelled rows are defined inside `extern "C"`.
 * RANGE. .text 0x803BE30C-0x803C3A5C (87 functions); extab, extabindex, .ctors 0x8056F3B8, .data 0x805F84E0-0x805F8920,
 *   .bss 0x806D2AF8-0x806D2B20 (`pop_dat_ptrs`), .sdata 0x807936A8-0x80793738, .sbss 0x80794C60-0x80794C90, .sdata2
 *   0x8079C630-0x8079C680.  `dataorder.py` shows no seam inside the `.data` claim (strings, then tables).
 * FLAGS. `cflags_menu` (configure.py); `infer.py` on the target: 0 record forms and a `clrlwi` kept before a narrowing
 *   store (the peephole off), and extab records (`-Cpp_exceptions on`).
 * NAMES. Module `menu`: no `__FILE__` string reaches the range; the file keeps the first symbol, the runtime dump's own
 *   name.  The static initialiser `__sinit_\get_pop_dat_ptr_cpp` (the `.ctors` word 0x8056F3B8) fills
 *   `eft_em_model_pair_colors`.  GUESS (from each body and its callers): `note_goods_tbl`, `option_cfg_eq_ck`,
 *   `option_cfg_load`, `pop_dat_place_load`, `pop_dat_spare_load`, `pop_item_tbl_2`..`pop_item_tbl_9`,
 *   `option_default_tbl`, `option_value_tbl`, the `lb_pop_*` room-visit set, `lb_screen24/30/34_*`, the `lb_room_*`
 *   set (`lb_room_wpad_*`, `lb_room_lock_*`, `lb_room_menu_*`, `lb_room_list_draw`), `lb_item_slot_draw`,
 *   `lb_item_box_panel_draw`, `lb_item_box_rows_draw`, `lb_menu_frame_sprites`, `lb_menu_title_sprites`,
 *   `lb_menu_row_sprites`, `lb_item_box_frame_sprites`, `eft_em_model_ring_attach`, the `eft_em_model_pair_*` set,
 *   and the types `PopFile`, `PopFilePoint`, `LbPopWork`, `LbRoomWork`, `LbScreen30Work`, `LbScreen34Work`,
 *   `PopEftModels`.
 * RESIDUALS. Unwritten (objdiff scores them zero; 12 functions, 8076 B): 0x803C0F3C (`lb_room_wpad_step`),
 *   0x803C1B98-0x803C20A4, 0x803C2148, 0x803C2370-0x803C27B8, 0x803C2AE4-0x803C3018, 0x803C3098, 0x803C3510.
 *   Partial rows:
 *  - `pop_dat_spare_load`: `em_pop_spare_push` is declared with a `u8` area (`enemy/em_model.h`), so ours keeps a
 *    `clrlwi` the retail call (an unnarrowed `lhz`) does not have;
 *  - `lb_pop_room_enter`: retail leaves the peer search with no `i < 4` test after the loop (`b` to the end), ours
 *    tests `i` again (+4 B); the `while`/`return` shapes measure lower;
 *  - `entry_list_update`: `note_slot_flat_table` is declared 4 entries long (`lobby/lb_quest_screen.h`), so ours
 *    reaches it through `@sda21` where retail uses `lis`/`addi`;
 *  - `lb_room_move_step`, `lb_room_place_step`, `lb_room_lock_step`: retail passes `toggle_word_step` a `clrlwi`-ed
 *    0xFFFF (a `u16` parameter); `menu/menu_message.h` declares it `s32`;
 *  - `__sinit_\get_pop_dat_ptr_cpp`: our second load is scheduled before the `li`, retail's after it;
 *  - the rest (`lb_screen30_step`, `lb_room_slot_release`, `lb_room_box_list_set`, `lb_room_move_open`,
 *    `lb_room_place_put`, `lb_pop_peer_*`, `pop_dat_place_load`, `lb_name_entry_screen_step`,
 *    `eft_em_model_pair_create`): register numbering or one scheduling swap.
 *   Data: `.data`, `.sdata`, `.sbss` and `.sdata2` are emitted only as far as the written functions define them (the
 *   option, room, sprite and effect tables, the file-name strings); `note_goods_tbl` and the draw tables of the
 *   unwritten draws are not defined yet, and the emission order is not measured.
 */

#include "types.h"
#include "nw4r/math.h"       /* nw4r::math::VEC3, the 3-float record VEC3_ctor takes */
#include "mh3_pad/vec3.h"    /* VEC3_ctor / copyVec3 (owner src/mh3_pad.cpp) */
#include "fn_8004CAD8.h"     /* get_vsUser_work / _vs_user_data (owner src/fn_8004CAD8.cpp) */
#include "hud/layout.h"     /* _mh_ivec2_, which the band header's prototypes name */
#include "Runtime.PPCEABI.H/memset.h" /* memset (owner: the Runtime.PPCEABI.H lib) */
#include "Runtime.PPCEABI.H/memcpy.h" /* memcpy (owner: the Runtime.PPCEABI.H lib) */
#include "MSL_C/mem.h"       /* memcmp */
#include "MSL_C/string.h"    /* strcpy */
#include "MSL_C/strcat.h"    /* strcat */
#include "MSL_C/printf.h"    /* sprintf */
#include "unsplit/menu.h"    /* option_w, this band's own tables, its callees */
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "lobby/lb_server_sel_trans.h" /* demo_work_process (owner's header, rule 2) */
#include "lobby/lobby_w.h"   /* lobby_w */
#include "lobby/lb_npc.h"    /* LbPlaceRec */
#include "lobby/lb_player_area_enter.h"
#include "lobby/lb_cmd_pressed_ck.h"
#include "enemy/lobby_state_block.h"
#include "enemy/em_model.h"  /* em_roster_pop_entry_add, the spare pool */
#include "ef/get_move_work_adrs.h"
#include "ef/fn_800CDB2C.h"  /* load_file_direct, move_work_state_ck */
#include "ef/system_core.h"  /* work_mem_alloc */
#include "ef/work_mem_free.h"
#include "ef/eft_rot_vec_copy.h"
#include "quest/quest_types.h" /* Q_MoveWork */
#include "quest/quest_item_slot.h" /* quest_select_ready_ck */
#include "quest/quest_entry.h" /* quest_slot_progress_get */
#include "stage/stg_w.h"     /* stage_map_kind_get, stage_map_area_count_get */
#include "stage/get_now_mapno.h"
#include "stage/stage_zone_hit_ck.h" /* stage_group_word_get */
#include "sound/set_SE_volume.h"
#include "sound/set_BGM_volume.h"
#include "sound/fn_800D7F54.h" /* sysSE_req */
#include "set_now_brightness.h"
#include "camera/camera_main.h" /* camera_lobby_disarm */
#include "hud/cockpit.h"     /* ainpc_page_hold_set */
#include "fn_80040598.h"     /* kbd_open / kbd_move / set_kbd_param */
#include "fn_80047398.h"     /* subTransSet */
#include "menu/menu_item.h"  /* menu_work */
#include "menu/menu_message.h" /* menu_cursor_step */
#include "menu/place_info_set.h"
#include "menu/multi_result.h" /* lobby_pig_dress_view_set */
#include "Pl/pl_info_text_set.h"
#include "Pl/plw.h"          /* _PLW */
#include "menu/pop_place_item_tbl.h"
#include "menu/pop_spare_item_tbl.h"
#include "menu/option_cfg_max.h"
#include "menu/pop_dat_c_load.h"
#include "menu/pop_dat_f_load.h"
#include "menu/pop_dat_r_load.h"
#include "menu/pop_dat_ptrs_bind.h"
#include "menu/pop_map_kind_get.h"
#include "menu/ck_cfg.h"
#include "menu/get_option_pair.h"
#include "menu/lb_pop_screen_open.h"
#include "menu/lb_name_entry_screen_step.h"
#include "menu/lb_screen24_step.h"
#include "menu/lb_screen30_step.h"
#include "menu/lb_screen34_step.h"
#include "menu/place_model_load.h"
#include "lobby/lb_menu_row_mark_set.h"
#include "lobby/lb_list_init.h"     /* LbList, lb_list_init, lb_choice_* */
#include "lobby/lb_panel_close.h"
#include "lobby/lb_talk_page_open.h"
#include "lobby/lb_unlock_cond_ck.h"
#include "lobby/lb_npc_callees.h"   /* lobby_world_block, game_ready_ck */
#include "lobby/lb_quest_screen.h"  /* note_slot_to_value */
#include "enemy/em020_prog.h"       /* em020_profile_send */
#include "ef/eft052.h"
#include "ef/eft052_page_put.h"
#include "ef/cockpit_hold_entry.h"
#include "ef/cockpit_hold_block.h"
#include "userdata_item.h"   /* userdata_item_count_total */
#include "ef.h"              /* _EFT, EftModel */
#include "ef/eft_res.h"
#include "ef/effect.h"       /* eft_state_flags_set */
#include "color_rgba_copy.h"
#include "enemy/ENEMY_WORK.h"
#include "mh3_pad/eft_color_white.h"
#include "sound/fn_800E3CBC.h"   /* set_blendmode */
#include "hud/get_lsp_data.h"
#include "lobby/lb_item_box_count_draw.h"
#include "lobby/lb_list_draw_ex.h"
#include "sound/set_zmode.h"
#include "ef/eft052_item_get_open.h" /* eft052_item_box */

/* One point record of a place / free-hunt pop file.  size: 0x38 */
struct PopFilePoint {
    /* +0x00 */ u16 id_0x00;          /* 0: an unused record */
    /* +0x02 */ u16 area_0x02;
    /* +0x04 */ nw4r::math::VEC3 pos_0x04;
    /* +0x10 */ _CP_VECTOR rot_0x10;
    /* +0x1C */ u16 item_0x1C;
    /* +0x1E */ u8 pad_0x1E[0x6];
    /* +0x24 */ u16 count_0x24;
    /* +0x26 */ u16 bonus_0x26;
    /* +0x28 */ f32 radius_0x28;
    /* +0x2C */ f32 height_0x2C;
    /* +0x30 */ u8 model_0x30;        /* the model kind (0xFF: none) */
    /* +0x31 */ u8 point_0x31;
    /* +0x32 */ u8 pad_0x32[0x2];
    /* +0x34 */ u16 uses_0x34;
    /* +0x36 */ u16 uses_max_0x36;
};

/* A pop file as `load_file_direct` reads it: the record count, then the records (place points for the `c`/`f`
 * files, spare heads for the `r` file).  size: 0x4800 (the load buffer) */
struct PopFile {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ s32 count_0x04;
    /* +0x08 */ union {
        PopFilePoint points_0x08[1];
        EmPopSpare spares_0x08[1];
    };
};

/* The room-visit screen's work (`LbLobbyWork::pop_0x0AC`, screen 0x1F): its step, the peer count and cursor, the
 * room password typed and the visited peer's id.  size: 0x2000 */
struct LbPopWork {
    /* +0x00 */ s8 step_0x00;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ s8 sub_0x02;          /* the password entry's step */
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ s16 count_0x04;       /* the peers listed (the lobby's peer count less this player) */
    /* +0x06 */ u8 pad_0x06[0x2];
    /* +0x08 */ s32 yes_no_0x08;      /* the yes/no prompt's state (`lb_yes_no_step`) */
    /* +0x0C */ s16 sel_0x0C;         /* the peer under the cursor */
    /* +0x0E */ s16 wait_0x0E;        /* the wrong-password message's frames */
    /* +0x10 */ u8 pass_0x10[0x5];    /* the password the player typed */
    /* +0x15 */ char input_0x15[0xA]; /* the keyboard's buffer */
    /* +0x1F */ char id_0x1F[0xA];    /* the id of the peer whose room is visited */
    /* +0x29 */ u8 pad_0x29[0x3];
    /* +0x2C */ struct _PLW* player_0x2C;
    /* +0x30 */ u8 unused_0x30[0x2000 - 0x30];
};

/* The per-option maxima (`.data` 0x805F8528): `set_option_cfg` and the arena twin clamp to them. */
extern "C" {
u8 option_cfg_max[0x20] = {
    1, 1, 3, 4, 1, 2, 1, 2, 3, 3, 1, 2, 1, 1, 1, 1, 2, 7, 7, 1, 1, 1, 1, 150, 0, 0, 1, 1, 1, 1, 0, 0,
};
}

/* The default option table (`.data` 0x805F8548) `set_def_option`/`option_cfg_load` start from. */
u8 option_default_tbl[0x20] = {
    0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 5, 7, 0, 0, 0, 0, 100, 0, 0, 1, 1, 1, 0, 0, 0,
};

/* Option id -> (value, flag) byte pairs for options 1, 5, 6 and 7 (`.sdata` 0x807936B8). */
u8 option_value_tbl[8] = {0, 3, 3, 2, 5, 4, 9, 8};

/* The lobby menus' frame and title sprites (`.data` 0x805F8828 / 0x805F8840), 0xFFFF-terminated. */
u16 lb_menu_frame_sprites[12] = {0x226D, 0x2266, 0x2267, 0x2270, 0x226E, 0x226F, 0x2253, 0x2254, 0x2255, 0xFFFF, 0, 0};
u16 lb_menu_title_sprites[8] = {0x2251, 0x2252, 0x2268, 0x2269, 0x226A, 0x226B, 0x226C, 0xFFFF};

/* The item-box panel's frame sprites (`.data` 0x805F88AC), 0xFFFF-terminated. */
u16 lb_item_box_frame_sprites[0x24] = {
    0x22C1, 0x22C2, 0x22BE, 0x22BF, 0x22C0, 0x22A4, 0x22A5, 0x22A6, 0x22A7, 0x22A8, 0x22A9, 0x22AA,
    0x22AB, 0x22AC, 0x22AD, 0x22AE, 0x22AF, 0x22B0, 0x22B1, 0x22B2, 0x22B3, 0x22B4, 0x22B5, 0x22B6,
    0x22B7, 0x22B8, 0x22B9, 0x22BA, 0x22BB, 0x22BC, 0x22BD, 0x22A1, 0x22A2, 0x22A3, 0xFFFF, 0x0000,
};

/* The 0x38 model effect's per-variant scale and model id (`.sdata` 0x80793730 / 0x80793728). */
f32 eft_em_model_pair_scales[2] = {10.0f, 30.0f};
u16 eft_em_model_pair_ids[4] = {59, 171, 0, 0};

/* The two colours of the 0x38 model effect (`.sbss` 0x80794C88), filled by the static initialiser from the white and
 * red colour words of `src/mh3_pad.cpp`. */
u32 eft_em_model_pair_colors[2] = {eft_color_white, eft_color_red};

/* Screen 0x1E's work (`LbLobbyWork::screen30_0x0AC`): the room-layout list and its confirmation.  size: 0x2000 */
struct LbScreen30Work {
    /* +0x00 */ s8 step_0x00;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ LbList list_0x04;          /* the room layouts (two, three once condition 37 is met) */
    /* +0x24 */ s32 yes_no_0x24;
    /* +0x28 */ struct _PLW* player_0x28;
    /* +0x2C */ struct _PLW* partner_0x2C;
    /* +0x30 */ u8 unused_0x30[0x2000 - 0x30];
};

/* Screen 0x22's work (`LbLobbyWork::screen34_0x0AC`): a six-row page.  size: 0x2000 */
struct LbScreen34Work {
    /* +0x00 */ s8 step_0x00;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u16 moved_0x02;           /* the cursor helper's moved flags */
    /* +0x04 */ struct _PLW* player_0x04;
    /* +0x08 */ struct _PLW* partner_0x08;
    /* +0x0C */ s32 cursor_0x0C;
    /* +0x10 */ u8 unused_0x10[0x2000 - 0x10];
};

/* The room screen's work (`LbLobbyWork::room_0x0AC`, screen 0x18): the top menu, the place / move sub-screens and the
 * rooms' placed-model lists.  size: 0x2000 */
struct LbRoomWork {
    /* +0x000 */ s8 step_0x00;
    /* +0x001 */ s8 sub_0x01;              /* the room menu's sub-screen */
    /* +0x002 */ s8 mode_0x02;             /* the move / place sub-screen's step */
    /* +0x003 */ u8 pad_0x03;
    /* +0x004 */ s8 kbd_0x04;              /* the password keyboard's step */
    /* +0x005 */ u8 pad_0x05[0x3];
    /* +0x008 */ LbList menu_0x08;         /* the top menu */
    /* +0x028 */ LbList sub_0x28;          /* the room menu (place / move) */
    /* +0x048 */ LbList move_0x48;         /* the move screen's here / elsewhere list */
    /* +0x068 */ LbList rooms_0x68;        /* the other rooms */
    /* +0x088 */ u8 move_flag_0x88;
    /* +0x089 */ u8 other_rooms_0x89[0x4]; /* the room indices other than this one */
    /* +0x08D */ u8 here_used_0x8D;        /* this room holds a model */
    /* +0x08E */ u8 other_used_0x8E;       /* another room holds one */
    /* +0x08F */ u8 room_used_0x8F[0x4];   /* per room: it holds a model */
    /* +0x093 */ u8 slots_0x93[0x8];       /* this room's used spots (`lobby_w.place_ids_0x134` indices) */
    /* +0x09B */ u8 slot_marks_0x9B[0x8];  /* their row marks (1 placed, 2 also in the item box) */
    /* +0x0A3 */ u8 pad_0xA3;
    /* +0x0A4 */ s16 slot_count_0xA4;
    /* +0x0A6 */ s16 slot_max_0xA6;
    /* +0x0A8 */ s16 rooms_0xA8;           /* the rooms unlocked so far (1..4) */
    /* +0x0AA */ s16 slot_sel_0xAA;
    /* +0x0AC */ u16 items_0xAC[0x1E];     /* the hold strip's item list */
    /* +0x0E8 */ s32 toggle_0xE8;          /* the take-back question's state (`toggle_word_step`) */
    /* +0x0EC */ s16 lock_sel_0xEC;        /* the lock list's cursor */
    /* +0x0EE */ s16 lock_0xEE;            /* the lock being set (0 closed, 1 open, 2 password) */
    /* +0x0F0 */ u8 pass_0xF0[0x5];        /* the password being set */
    /* +0x0F5 */ char input_0xF5[0xB];     /* the keyboard's buffer */
    /* +0x100 */ struct _PLW* player_0x100;
    /* +0x104 */ struct _PLW* partner_0x104;
    /* +0x108 */ u8 pad_0x108[0x44];
    /* +0x14C */ u16 field_0x14C;
    /* +0x14E */ u8 unused_0x14E[0x2000 - 0x14E];
};

/* The room menu's list templates (`.data` 0x805F87B0 / 0x805F87D8): the top menu and the place / move sub-list, the
 * second pair for a ready game (no help line). */
LbChoiceTmpl lb_room_menu_tmpls[2] = {
    {2, 1, 0, 0x1EE, 0x1BD, NULL, 5, 3},
    {2, 2, 0, 0x1F1, -1, NULL, -1, 3},
};
LbChoiceTmpl lb_room_menu_tmpls_ready[2] = {
    {2, 1, 0, 0x1EE, -1, NULL, 5, 3},
    {2, 2, 0, 0x1F1, -1, NULL, -1, 3},
};

extern "C" {
/* The four rooms' option ids (`.sdata` 0x807936C0), and the top menu's row actions (0 place, 1 Wii Remote data, 2 lock) without
 * and with a ready game (0x807936C4 / 0x807936C8). */
u8 lb_room_option_ids[4] = {1, 5, 6, 7};
u8 lb_room_menu_actions[4] = {0, 1, 0, 0};
u8 lb_room_menu_actions_ready[4] = {0, 2, 0, 0};
/* The room menu's row titles and help lines for a ready game (0x807936CC / 0x807936D0), and the menus' row sprites
 * (0x807936E0). */
u16 lb_room_menu_titles[2] = {0x1EE, 0x1F0};
u16 lb_room_menu_helps[2] = {0x1BD, 0x1BF};
u16 lb_menu_row_sprites[4] = {0x2275, 0x2276, 0x2277, 0};

void lb_screen30_draw(void);
void lb_screen34_draw(void);
void lb_screen24_draw(void);
void lb_room_menu_open(LbRoomWork* work);
s32 lb_room_menu_step(LbRoomWork* work);
void lb_room_menu_init(LbRoomWork* work);
s16 lb_room_place_count_get(void);
void lb_room_place_list_build(LbRoomWork* work);
s32 lb_room_place_step(LbRoomWork* work);
void lb_room_slots_build(LbRoomWork* work);
void lb_room_slot_release(LbRoomWork* work);
void lb_room_slot_preview(LbRoomWork* work);
void lb_room_move_open(LbRoomWork* work);
void lb_room_move_list_init(LbRoomWork* work);
void lb_room_box_list_set(LbRoomWork* work, u8 quiet);
s32 lb_room_move_step(LbRoomWork* work);
s32 lb_room_wpad_step(LbRoomWork* work);
void lb_room_lock_open(LbRoomWork* work);
s32 lb_room_lock_step(LbRoomWork* work);
void entry_reset(LbRoomWork* work);
void entry_state_reset(LbRoomWork* work);
void entry_list_update(LbRoomWork* work);
void lb_room_place_put(LbRoomWork* work);
void eft_em_model_pair_release(_EFT* self);
void eft_em_model_pair_update(_EFT* self);
void eft_em_model_pair_create(_EFT* self);
void eft_em_model_pair_move(_EFT* self);
void eft_em_model_pair_end(_EFT* self);
void counter_inc(_EFT* self);
void lb_item_box_rows_draw(Eft052ItemBox* box);
void lb_room_menu_draw(LbRoomWork* work);
void lb_room_wpad_draw(LbRoomWork* work);
void lb_room_lock_draw(LbRoomWork* work);
void lb_room_list_draw(LbRoomWork* work);
}

/* The model block `_EFT::work_0x38` points at for the 0x38 model effects (not an `_EFT_WORK`): the model count, the
 * models and the colour.  size: 0x2C (approximation: the highest offset written + 2) */
struct PopEftModels {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ EftModel* models_0x04[8];
    /* +0x24 */ u16 field_0x24;
    /* +0x26 */ u8 color_0x26[4];      /* the models' colour (`color_rgba_copy`) */
    /* +0x2A */ u8 pad_0x2A[0x2];
};

/* The ten pop-data pointers (.bss 0x806D2AF8) the accessor below hands out; this unit's whole `.bss` claim. */
const struct EmWeightEntry*** pop_dat_ptrs[10];

/* The pop-data pointer copies `pop_dat_ptrs_bind` publishes (`.sbss` 0x80794C60..0x80794C88). */
extern "C" {
const struct EmWeightEntry*** pop_place_item_tbl;
const struct EmWeightEntry*** pop_spare_item_tbl;
const struct EmWeightEntry*** pop_item_tbl_2;
const struct EmWeightEntry*** pop_item_tbl_3;
const struct EmWeightEntry*** pop_item_tbl_4;
const struct EmWeightEntry*** pop_item_tbl_5;
const struct EmWeightEntry*** pop_item_tbl_6;
const struct EmWeightEntry*** pop_item_tbl_7;
const struct EmWeightEntry*** pop_item_tbl_8;
const struct EmWeightEntry*** pop_item_tbl_9;
}

/* Hands out the ten pop-data pointers. */
const struct EmWeightEntry**** get_pop_dat_ptr(void)
{
    return pop_dat_ptrs;
}

extern "C" {

nw4r::math::VEC3* vec3_construct(nw4r::math::VEC3* out);
void pop_dat_spare_load(Q_MoveWork* work, char* path);
void lb_pop_peer_count_set(LbPopWork* work);
void lb_pop_screen_close(void);
void lb_pop_confirm_open(LbPopWork* work);
void lb_pop_room_enter(LbPopWork* work);
LobbyPeerCard* lb_pop_peer_card_find(LbPopWork* work);
void lb_pop_peer_id_copy(LbPopWork* work);
LobbyPeerCard* lb_pop_peer_card_get(LbPopWork* work);
void lb_pop_screen_draw(void);

/* Loads a place pop file into the move work's buffer and files each used point with the roster. */
void pop_dat_place_load(Q_MoveWork* work, char* path)
{
    EmPopEntry entry;
    PopFile* file;
    PopFilePoint* point;
    s32 count;
    s32 i;
    u8 kind;

    vec3_construct(&entry.pos_0x00);
    file = (PopFile*)work->result_buffer_0x150;
    if (file == NULL) {
        return;
    }
    if (load_file_direct(path, (u32)file, 0x4800) == 0) {
        return;
    }
    count = file->count_0x04;
    point = file->points_0x08;
    for (i = 0; i < count; i++, point++) {
        if (point->id_0x00 == 0) {
            continue;
        }
        memset(&entry, 0, sizeof(entry));
        copyVec3(&entry.pos_0x00, &point->pos_0x04);
        eft_rot_vec_copy(&entry.rot_0x0C, &point->rot_0x10);
        entry.point_0x20 = kind = point->point_0x31;
        entry.point_kind_0x22 = point->point_0x31;
        entry.count_0x24 = point->count_0x24;
        entry.bonus_0x26 = point->bonus_0x26;
        entry.radius_0x18 = point->radius_0x28;
        entry.height_0x1C = point->height_0x2C;
        entry.model_0x2C = point->model_0x30;
        entry.area_0x2D = point->area_0x02;
        entry.item_0x30 = point->item_0x1C;
        entry.uses_0x28 = point->uses_0x34;
        entry.uses_max_0x2A = point->uses_max_0x36;
        switch (kind) {
        case 5:
        case 6:
        case 7:
        case 8:
        case 10:
        case 16:
        case 17:
        case 18:
            entry.model_0x2C = -1;
            entry.count_0x24 = 0xFFFF;
            entry.bonus_0x26 = 0xFFFF;
            entry.item_0x30 = 0;
            break;
        default:
            if (pop_map_kind_get() == 0 && work->sub_0xEA == 0) {
                entry.count_0x24 = 0xFFFF;
                entry.bonus_0x26 = 0xFFFF;
            }
            break;
        }
        em_roster_pop_entry_add(&entry, entry.area_0x2D);
    }
}

/* Constructs the 3-float record at `out` and hands the same pointer back. */
nw4r::math::VEC3* vec3_construct(nw4r::math::VEC3* out)
{
    VEC3_ctor(out);
    return out;
}

/* Loads the spare pop file and files each used spare head with the roster or the spare pool. */
void pop_dat_spare_load(Q_MoveWork* work, char* path)
{
    PopFile* file;
    EmPopSpare* spare;
    s32 count;
    s32 i;
    u16 slot;

    em_pop_spare_init();
    file = (PopFile*)work_mem_alloc(0x4800);
    if (file == NULL) {
        return;
    }
    if (load_file_direct(path, (u32)file, 0x4800) == 0) {
        return;
    }
    count = file->count_0x04;
    stage_map_area_count_get(pop_map_kind_get());
    spare = file->spares_0x08;
    for (i = 0; i < count; i++, spare++) {
        if (spare->field_0x000 == 0) {
            continue;
        }
        slot = (spare->field_0x000 & 0xFF00) >> 8;
        if (slot >= 20) {
            em_pop_spare_push(spare, spare->area_0x002, slot);
        } else {
            em_roster_spare_entry_add(spare, spare->area_0x002, slot);
        }
    }
    work_mem_free(file);
    em_pop_spare_spawn();
}

/* Loads the map's common pop file (`06/m<map>_000000_c_pop.dat`). */
void pop_dat_c_load(void)
{
    char name[64];
    char path[256];
    Q_MoveWork* work;

    if (quest_select_ready_ck() == 1) {
        return;
    }
    work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work == NULL) {
        return;
    }
    sprintf(name, "m%03d_%06d_c_pop.dat", pop_map_kind_get(), 0);
    strcpy(path, "06/");
    strcat(path, name);
    pop_dat_place_load(work, path);
}

/* Loads the map's free-hunt pop file (variant 1 in the quest select or the move state, else 2 or 0 by the
 * slot's progress). */
void pop_dat_f_load(void)
{
    char path[256];
    Q_MoveWork* work;
    s32 variant;

    work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work == NULL) {
        return;
    }
    if (quest_select_ready_ck() == 1 || move_work_state_ck() == 1) {
        variant = 1;
    } else {
        variant = quest_slot_progress_get(0) == 0 ? 0 : 2;
    }
    sprintf(path, "06/m%03d_%06d_f_pop.dat", pop_map_kind_get(), variant);
    pop_dat_place_load(work, path);
}

/* Loads the map's spare pop file. */
void pop_dat_r_load(void)
{
    char path[256];
    Q_MoveWork* work;
    s32 variant;

    if (quest_select_ready_ck() == 1) {
        return;
    }
    work = (Q_MoveWork*)get_move_work_adrs(0);
    if (work == NULL) {
        return;
    }
    if (move_work_state_ck() == 1) {
        variant = 1;
    } else {
        variant = quest_slot_progress_get(0) == 0 ? 0 : 2;
    }
    sprintf(path, "06/m%03d_%06d_r_pop.dat", pop_map_kind_get(), variant);
    pop_dat_spare_load(work, path);
}

/* Publishes the ten pop-data pointers into their small-data copies. */
void pop_dat_ptrs_bind(void)
{
    pop_place_item_tbl = pop_dat_ptrs[0];
    pop_spare_item_tbl = pop_dat_ptrs[1];
    pop_item_tbl_2 = pop_dat_ptrs[2];
    pop_item_tbl_3 = pop_dat_ptrs[3];
    pop_item_tbl_4 = pop_dat_ptrs[4];
    pop_item_tbl_5 = pop_dat_ptrs[5];
    pop_item_tbl_6 = pop_dat_ptrs[6];
    pop_item_tbl_7 = pop_dat_ptrs[7];
    pop_item_tbl_8 = pop_dat_ptrs[8];
    pop_item_tbl_9 = pop_dat_ptrs[9];
}

/* The current map's kind (the map number itself when the stage table has none). */
u8 pop_map_kind_get(void)
{
    u8 map = get_now_mapno();
    u8 kind = stage_map_kind_get(map);

    if (kind == 0xFF) {
        kind = map;
    }
    return kind;
}

} /* extern "C" */

/* Applies the option table's volumes and brightness (a brightness under 50 resets to 100). */
void set_option_mode(void)
{
    u8* options = option_w;

    set_SE_volume(options[18]);
    set_BGM_volume(options[17]);
    if (options[23] < 50) {
        set_now_brightness(1.0f);
        options[23] = 100;
    } else {
        set_now_brightness(options[23] / 100.0f);
    }
}

u8 get_arena_cfg(u8 player, u8 index);
void set_option_cfg(u8 index, u8 value);

extern "C" {

/* Applies the player's arena profile: the SE/BGM volumes and the brightness. */
void arena_cfg_apply(u8 player)
{
    if (system_w.vs_mode_0x8b0 == 0) {
        return;
    }
    if (get_vsUser_work(player) == NULL) {
        return;
    }
    set_SE_volume(get_arena_cfg(player, 18));
    set_BGM_volume(get_arena_cfg(player, 17));
    if (get_arena_cfg(player, 23) < 50) {
        set_now_brightness(1.0f);
        arena_cfg_set(player, 23, 100);
    } else {
        set_now_brightness(get_arena_cfg(player, 23) / 100.0f);
    }
}

/* Reads one arena profile byte clamped to its option maximum, inverting index 20. */
u8 ck_arena_cfg(u8 player, u8 index)
{
    _vs_user_data* user;
    u8 value;

    if (system_w.vs_mode_0x8b0 == 0) {
        return 0;
    }
    user = get_vsUser_work(player);
    if (user == NULL) {
        return 0;
    }
    value = user->option_0xD0[index];
    if (value > option_cfg_max[index]) {
        value = option_cfg_max[index];
    }
    if (index == 20) {
        value ^= 1;
    }
    return value;
}

} /* extern "C" */

/* Reads one option clamped to its table maximum, inverting index 20. */
u8 ck_option_cfg(u8 index)
{
    u8 value = option_w[index];

    if (value > option_cfg_max[index]) {
        value = option_cfg_max[index];
    }
    if (index == 20) {
        value ^= 1;
    }
    return value;
}

extern "C" {

/* Dispatches to the normal or the arena option check, by the system's VS/arena mode byte. */
u8 ck_cfg(u8 player, u8 index)
{
    if (system_w.vs_mode_0x8b0 == 0) {
        return ck_option_cfg(index);
    }
    return ck_arena_cfg(player, index);
}

/* Stores one arena profile byte clamped to its maximum (index 20 inverted) and pushes a changed volume through. */
void arena_cfg_set(u8 player, u8 index, u8 value)
{
    _vs_user_data* user;

    if (system_w.vs_mode_0x8b0 == 0) {
        return;
    }
    user = get_vsUser_work(player);
    if (user == NULL) {
        return;
    }
    if (value > option_cfg_max[index]) {
        value = option_cfg_max[index];
    }
    if (index == 20) {
        value ^= 1;
    }
    user->option_0xD0[index] = value;
    if (index == 18) {
        set_SE_volume(value);
    }
    if (index == 17) {
        set_BGM_volume(value);
    }
}

} /* extern "C" */

/* Stores one option clamped to its maximum (index 20 inverted) and pushes a changed volume through. */
void set_option_cfg(u8 index, u8 value)
{
    if (value > option_cfg_max[index]) {
        value = option_cfg_max[index];
    }
    if (index == 20) {
        value ^= 1;
    }
    ((s8*)option_w)[index] = (s8)value;
    if (index == 18) {
        set_SE_volume(value);
    }
    if (index == 17) {
        set_BGM_volume(value);
    }
}

/* Reads one option out of the arena profile of the VS user work, inverting index 20. */
u8 get_arena_cfg(u8 player, u8 index)
{
    _vs_user_data* user;

    if (system_w.vs_mode_0x8b0 == 0) {
        return 0;
    }
    user = get_vsUser_work(player);
    if (user == NULL) {
        return 0;
    }
    if (index == 20) {
        return user->option_0xD0[index] ^ 1;
    }
    return user->option_0xD0[index];
}

/* Reads one option, inverting index 20 (the one the option screen stores negated). */
u8 get_option_cfg(u8 index)
{
    if (index == 20) {
        return option_w[index] ^ 1;
    }
    return option_w[index];
}

extern "C" {

/* Dispatches to the normal or the arena option read, by the system's VS/arena mode byte. */
u8 get_cfg(u8 player, u8 index)
{
    if (system_w.vs_mode_0x8b0 == 0) {
        return get_option_cfg(index);
    }
    return get_arena_cfg(player, index);
}

/* 1 when option `index` (of the option table or the arena profile) holds `value`. */
u32 option_cfg_eq_ck(u8 player, u8 index, u8 value)
{
    u8 current;

    if (system_w.vs_mode_0x8b0 == 0) {
        current = get_option_cfg(index);
    } else {
        current = get_arena_cfg(player, index);
    }
    if (current == value) {
        return 1;
    }
    return 0;
}

/* A 4-byte `blr`: the option module's empty hook.  GUESS - nothing in the DOL calls it. */
void option_nop(void)
{
}

} /* extern "C" */

/* Copies the default option table into `option_w` and stores each entry through `set_option_cfg`. */
void set_def_option(void)
{
    u8* option;
    s32 i;

    memcpy(option_w, option_default_tbl, 31);
    for (i = 0, option = option_w; i < 31; i++) {
        set_option_cfg(i, *option);
        option++;
    }
}

extern "C" {

/* Copies the default option table into `options` and stores each entry through `set_option_cfg`. */
void option_cfg_load(u8* options)
{
    s32 i;

    memcpy(options, option_default_tbl, 31);
    for (i = 0; i < 31; i++, options++) {
        set_option_cfg(i, *options);
    }
}

/* Reads option 21 out of the option table or the arena profile. */
u8 get_option_21(void)
{
    if (system_w.vs_mode_0x8b0 == 0) {
        return option_w[21];
    }
    return get_arena_cfg(0, 21);
}

/* Maps an option id to the (value, flag) byte pair `option_value_tbl` holds for it. */
void get_option_pair(u8 index, u8* value, u8* flag)
{
    u8 entry = 255;

    switch (index) {
    case 1:
        entry = 0;
        break;
    case 5:
        entry = 1;
        break;
    case 6:
        entry = 2;
        break;
    case 7:
        entry = 3;
        break;
    }

    if (entry != 255) {
        *value = option_value_tbl[entry * 2];
        *flag = option_value_tbl[entry * 2 + 1];
    } else {
        *value = 0;
        *flag = 0;
    }
}

/* Lists the lobby's other peers: the count, with the cursor wrapped back to the first row when it ran past. */
void lb_pop_peer_count_set(LbPopWork* work)
{
    work->count_0x04 = lobby_state_block.peer_count_0x0004;
    if (work->count_0x04 <= 1) {
        work->count_0x04 = 0;
        work->sel_0x0C = 0;
        return;
    }
    work->count_0x04--;
    if (work->sel_0x0C + 1 > work->count_0x04) {
        work->sel_0x0C = 0;
    }
}

/* Opens the room-visit screen (screen 0x1F) for `player`. */
void lb_pop_screen_open(struct _PLW* player)
{
    LbPopWork* work = lobby_w.pop_0x0AC;

    memset(work, 0, 0x2000);
    work->player_0x2C = player;
    lobby_w.state_0x000 = 31;
    lobby_w.active_0x008 = 1;
    work->count_0x04 = 0;
    work->sel_0x0C = 0;
    memset(work->pass_0x10, 0, 5);
    lb_pop_peer_count_set(work);
    sysSE_req(5);
}

/* Closes the room-visit screen. */
void lb_pop_screen_close(void)
{
    lobby_w.state_0x000 = 0;
    lobby_w.active_0x008 = 0;
    camera_lobby_disarm();
}

/* Puts the visit question up (step 3, the yes/no prompt reset). */
void lb_pop_confirm_open(LbPopWork* work)
{
    work->step_0x00 = 3;
    work->yes_no_0x08 = 0;
}

/* Enters the visited peer's room: places the player in the room's lobby area, shows its place info and
 * Poogie, and closes the screen. */
void lb_pop_room_enter(LbPopWork* work)
{
    struct _PLW* player = work->player_0x2C;
    LobbyPeerCopy* peer;
    LbPlaceRec* place;
    s32 i;
    u32 kind;

    sysSE_req(0);
    peer = &lobby_state_block.peers_0x0008[1];
    for (i = 1; i < 4; i++, peer++) {
        if (memcmp(work->id_0x1F, peer->id_0x00, 10) == 0) {
            break;
        }
    }
    if (i >= 4) {
        return;
    }
    switch (peer->card.room_0x93) {
    default:
        kind = 5;
        break;
    case 1:
        kind = 6;
        break;
    case 2:
        kind = 7;
        break;
    }
    place = (LbPlaceRec*)stage_group_word_get(21, 3);
    while (place->area_0x00 != kind) {
        place++;
    }
    pl_info_text_set(player, peer->id_0x00);
    lb_player_area_enter(player, place, 1);
    place_info_set(peer->card.place_0x81);
    lobby_pig_dress_view_set(peer->card.pig_dress_0x94);
    player->field_0xB03 = 1;
    lb_pop_screen_close();
}

/* The card of the peer whose id the screen holds (the cursor follows it), or NULL once that peer left. */
LobbyPeerCard* lb_pop_peer_card_find(LbPopWork* work)
{
    LobbyPeerList* list = &lobby_state_block.peer_list_0x0004;
    s16 count = work->count_0x04;
    LobbyPeerCopy* peer;
    s16 row;
    s32 i;

    if (count <= 0) {
        return NULL;
    }
    peer = &list->peers_0x004[1];
    row = 0;
    for (i = 1; i < 4; i++, peer++) {
        if (list->peers_0x004[i].id_0x00[0] != 0) {
            if (memcmp(work->id_0x1F, peer->id_0x00, 10) == 0) {
                work->sel_0x0C = row;
                return &peer->card;
            }
            row++;
            if (row >= count) {
                break;
            }
        }
    }
    return NULL;
}

/* Copies the id of the peer under the cursor into the screen. */
void lb_pop_peer_id_copy(LbPopWork* work)
{
    LobbyPeerList* list = &lobby_state_block.peer_list_0x0004;
    s16 row = 0;
    s16 i;

    for (i = 1; i < 4; i++) {
        if (list->peers_0x004[i].id_0x00[0] != 0) {
            if (work->sel_0x0C == row) {
                strcpy(work->id_0x1F, list->peers_0x004[i].id_0x00);
                return;
            }
            row++;
        }
    }
}

/* The card of the peer under the cursor, or NULL. */
LobbyPeerCard* lb_pop_peer_card_get(LbPopWork* work)
{
    LobbyPeerCopy* peer = &lobby_state_block.peers_0x0008[1];
    s16 row = 0;
    s32 i;

    for (i = 1; i < 4; i++, peer++) {
        if (peer->id_0x00[0] != 0) {
            if (work->sel_0x0C == row) {
                return &peer->card;
            }
            row++;
        }
    }
    return NULL;
}

/* Steps the room-visit screen: the peer list, the password keyboard, the visit question and the wrong-password
 * message. */
void lb_name_entry_screen_step(void)
{
    LbPopWork* work = lobby_w.pop_0x0AC;
    LobbyPeerCard* card;

    lb_pop_peer_count_set(work);
    switch (work->step_0x00) {
    case 0:
        work->step_0x00++;
        break;
    case 1:
        ainpc_page_hold_set();
        if (work->count_0x04 > 0) {
            if (lb_cmd_pressed_ck(0x10)) {
                card = lb_pop_peer_card_get(work);
                if (card != NULL) {
                    switch (card->lock_0x92) {
                    case 1:
                        sysSE_req(0);
                        lb_pop_peer_id_copy(work);
                        lb_pop_confirm_open(work);
                        break;
                    case 2:
                        work->step_0x00 = 2;
                        work->sub_0x02 = 0;
                        memset(work->pass_0x10, 0, 5);
                        memset(work->input_0x15, 0, 10);
                        lb_pop_peer_id_copy(work);
                        sysSE_req(5);
                        break;
                    default:
                        sysSE_req(2);
                        break;
                    }
                }
            } else if (lb_cmd_pressed_ck(0x20)) {
                lb_pop_screen_close();
                sysSE_req(1);
            } else if (lb_cmd_repeat_ck(3)) {
                work->sel_0x0C = menu_cursor_step(work->sel_0x0C, work->count_0x04, lb_cmd_repeat_get(), 1, 2);
            }
        } else if (lb_cmd_pressed_ck(0x20)) {
            lb_pop_screen_close();
            sysSE_req(1);
        }
        break;
    case 2:
        ainpc_page_hold_set();
        if (lb_pop_peer_card_find(work) != NULL) {
            switch (work->sub_0x02) {
            case 0:
                if (kbd_open(4) == 1) {
                    menu_work.slot[0].field_0x32D = 1;
                    set_kbd_param(work->input_0x15, 8);
                    work->sub_0x02++;
                }
                break;
            case 1:
                switch (kbd_move()) {
                case 1:
                    menu_work.slot[0].field_0x32D = 0;
                    if (work->input_0x15[0] == 0 || memcmp(work->pass_0x10, work->input_0x15, 5) == 0) {
                        work->step_0x00 = 1;
                    } else {
                        memcpy(work->pass_0x10, work->input_0x15, 5);
                        lb_pop_confirm_open(work);
                    }
                    break;
                case -1:
                    menu_work.slot[0].field_0x32D = 0;
                    work->step_0x00 = 1;
                    break;
                }
                break;
            }
        } else {
            work->step_0x00 = 1;
            work->sel_0x0C = 0;
            sysSE_req(1);
        }
        break;
    case 3:
        ainpc_page_hold_set();
        card = lb_pop_peer_card_find(work);
        if (card != NULL) {
            if (card->lock_0x92 == 0) {
                work->step_0x00 = 1;
            } else {
                switch (lb_yes_no_step(&work->yes_no_0x08)) {
                case 1:
                    if (card->lock_0x92 == 2) {
                        if (memcmp(card->pass_0x7C, work->pass_0x10, 5) == 0) {
                            lb_pop_room_enter(work);
                        } else {
                            work->step_0x00 = 4;
                        }
                    } else {
                        lb_pop_room_enter(work);
                    }
                    break;
                case 2:
                    work->step_0x00 = 1;
                    break;
                }
            }
        } else {
            work->step_0x00 = 1;
            work->sel_0x0C = 0;
            sysSE_req(1);
        }
        break;
    case 4:
        if (work->wait_0x0E > 0) {
            work->wait_0x0E--;
        } else if (lb_cmd_pressed_ck(0x10)) {
            sysSE_req(0);
            work->step_0x00 = 1;
        }
        break;
    }
    subTransSet((u32)lb_pop_screen_draw, 0, 0);
}

} /* extern "C" */

extern "C" {

/* Opens screen 0x1E for `player` and `partner` (the talk page's room-layout list). */
void lb_screen30_open(struct _PLW* player, struct _PLW* partner)
{
    LbScreen30Work* work = lobby_w.screen30_0x0AC;

    memset(work, 0, 0x2000);
    work->player_0x28 = player;
    work->partner_0x2C = partner;
    lobby_w.state_0x000 = 30;
    lobby_w.active_0x008 = 1;
    lb_talk_page_open(0);
}

/* Closes screen 0x1E: sends the profile and closes the panel. */
void lb_screen30_close(void)
{
    em020_profile_send();
    lb_panel_close();
}

/* Steps screen 0x1E: the room-layout list, the confirmation and the talk page around them. */
void lb_screen30_step(void)
{
    LbScreen30Work* work = lobby_w.screen30_0x0AC;
    s32 result;

    switch (work->step_0x00) {
    case 0:
        result = lb_talk_page_mode_reset();
        if (result == 1 || result == 2) {
            work->step_0x00 = 1;
            lb_list_init(&work->list_0x04, (lb_unlock_cond_ck(37) == 1) + 2, 1, 502, -1, 0, 0, 3, 0);
            camera_talk_lock_set(1);
            sysSE_req(5);
        }
        break;
    case 1:
        ainpc_page_hold_set();
        switch (lb_choice_step(&work->list_0x04)) {
        case 1:
            work->step_0x00 = 2;
            work->yes_no_0x24 = 0;
            break;
        case 2:
            work->step_0x00 = 3;
            lb_talk_page_open(1);
            camera_talk_lock_set(0);
            break;
        }
        break;
    case 2:
        ainpc_page_hold_set();
        switch (lb_yes_no_step(&work->yes_no_0x24)) {
        case 1:
            work->step_0x00 = 3;
            lobby_world_block->room_layout_0x484D = work->list_0x04.cursor_0x00;
            lobby_world_block->room_lock_0x3F0A = 0;
            memset(lobby_world_block->room_pass_0x40FC, 0, 5);
            lb_talk_page_open(1);
            camera_talk_lock_set(0);
            break;
        case 2:
            work->step_0x00 = 1;
            break;
        }
        break;
    case 3:
        result = lb_talk_page_mode_reset();
        if (result == 1 || result == 2) {
            lb_screen30_close();
        }
        break;
    }
    if (work->step_0x00 != 0 && work->step_0x00 != 3) {
        subTransSet((u32)lb_screen30_draw, 0, 0);
    }
}

/* Opens screen 0x22 for `player` and `partner`. */
void lb_screen34_open(struct _PLW* player, struct _PLW* partner)
{
    LbScreen34Work* work = lobby_w.screen34_0x0AC;

    memset(work, 0, 0x2000);
    work->player_0x04 = player;
    work->partner_0x08 = partner;
    work->cursor_0x0C = 0;
    lobby_w.state_0x000 = 34;
    lobby_w.active_0x008 = 1;
    lb_talk_page_open(0);
}

/* Closes screen 0x22. */
void lb_screen34_close(void)
{
    lb_panel_close();
}

/* Steps screen 0x22: a six-row page the cursor walks until the cancel button. */
void lb_screen34_step(void)
{
    LbScreen34Work* work = lobby_w.screen34_0x0AC;
    s32 result;

    work->moved_0x02 = 0;
    switch (work->step_0x00) {
    case 0:
        result = lb_talk_page_mode_reset();
        if (result == 1 || result == 2) {
            work->step_0x00 = 1;
            camera_talk_lock_set(1);
            sysSE_req(5);
        }
        break;
    case 1:
        ainpc_page_hold_set();
        if (lb_cmd_pressed_ck(0x20)) {
            work->step_0x00 = 2;
            lb_talk_page_open(1);
            camera_talk_lock_set(0);
        } else if (lb_cmd_repeat_ck(12)) {
            work->cursor_0x0C = menu_cursor_step_fixed_tail(work->cursor_0x0C, 6, lb_cmd_repeat_get(), 4, 8,
                                                            &work->moved_0x02);
        }
        break;
    case 2:
        result = lb_talk_page_mode_reset();
        if (result == 1 || result == 2) {
            lb_screen34_close();
        }
        break;
    }
    if (work->step_0x00 != 0 && work->step_0x00 != 2) {
        subTransSet((u32)lb_screen34_draw, 0, 0);
    }
}

/* Opens the room screen (screen 0x18) for `player` and `partner`. */
void lb_screen24_open(struct _PLW* player, struct _PLW* partner)
{
    LbRoomWork* work = lobby_w.room_0x0AC;

    memset(work, 0, 0x2000);
    work->player_0x100 = player;
    work->partner_0x104 = partner;
    lobby_w.state_0x000 = 24;
    lobby_w.active_0x008 = 1;
    lb_talk_page_open(0);
}

/* Closes the room screen: sends the profile and closes the panel. */
void lb_screen24_close(void)
{
    em020_profile_send();
    lb_panel_close();
}

/* Steps the room screen: its top menu and the place, Wii Remote data and lock sub-screens. */
void lb_screen24_step(void)
{
    LbRoomWork* work = lobby_w.room_0x0AC;
    s32 result;
    u8* actions;

    switch (work->step_0x00) {
    case 0:
        result = lb_talk_page_mode_reset();
        if (result == 1 || result == 2) {
            work->step_0x00 = 1;
            if (game_ready_ck() == 1) {
                lb_choice_init(&work->menu_0x08, (const LbListTmpl*)&lb_room_menu_tmpls_ready[0], 0, 0);
            } else {
                lb_choice_init(&work->menu_0x08, (const LbListTmpl*)&lb_room_menu_tmpls[0], 0, 0);
            }
            sysSE_req(5);
            camera_talk_lock_set(1);
        }
        break;
    case 1:
        ainpc_page_hold_set();
        actions = lb_room_menu_actions;
        if (game_ready_ck() == 1) {
            actions = lb_room_menu_actions_ready;
        }
        switch (lb_choice_step(&work->menu_0x08)) {
        case 1:
            switch (actions[work->menu_0x08.cursor_0x00]) {
            case 0:
                work->step_0x00 = 2;
                lb_room_menu_open(work);
                break;
            case 1:
                work->step_0x00 = 3;
                entry_state_reset(work);
                break;
            case 2:
                work->step_0x00 = 4;
                lb_room_lock_open(work);
                break;
            }
            break;
        case 2:
            work->step_0x00 = 5;
            lb_talk_page_open(1);
            camera_talk_lock_set(0);
            break;
        }
        break;
    case 2:
        ainpc_page_hold_set();
        result = lb_room_menu_step(work);
        if (result == 1 || result == 2) {
            work->step_0x00 = 1;
            camera_lobby_goal_release();
        }
        break;
    case 3:
        ainpc_page_hold_set();
        result = lb_room_wpad_step(work);
        if (result == 1 || result == 2) {
            work->step_0x00 = 1;
        }
        break;
    case 4:
        ainpc_page_hold_set();
        result = lb_room_lock_step(work);
        if (result == 1 || result == 2) {
            work->step_0x00 = 1;
        }
        break;
    case 5:
        result = lb_talk_page_mode_reset();
        if (result == 1 || result == 2) {
            lb_screen24_close();
        }
        break;
    }
    if (work->step_0x00 != 0 && work->step_0x00 != 5) {
        subTransSet((u32)lb_screen24_draw, 0, 0);
    }
}

/* Sets the room menu's sub-list up (the place row greyed when no spot is free). */
void lb_room_menu_init(LbRoomWork* work)
{
    u16 off_mask = 0;

    if (lb_room_place_count_get() == 0) {
        off_mask |= 1;
    }
    if (game_ready_ck() == 1) {
        lb_choice_init(&work->sub_0x28, (const LbListTmpl*)&lb_room_menu_tmpls_ready[1], off_mask, 0);
    } else {
        lb_choice_init(&work->sub_0x28, (const LbListTmpl*)&lb_room_menu_tmpls[1], off_mask, 0);
    }
}

/* Opens the room menu: rebuilds the place list and the sub-list, and points the camera at this room. */
void lb_room_menu_open(LbRoomWork* work)
{
    u8 i;

    work->sub_0x01 = 0;
    lb_room_place_list_build(work);
    lb_room_menu_init(work);
    lb_menu_row_marks_clear();
    for (i = 0; i < 4; i++) {
        if (lb_room_option_ids[i] == lobby_w.area_0x002) {
            camera_lobby_view_set(i, 0);
        }
    }
}

/* Steps the room menu: the sub-list, the place screen and the move screen; 2 once cancelled. */
s32 lb_room_menu_step(LbRoomWork* work)
{
    s32 result = 0;
    s16 cursor;

    if (place_model_ready_ck() == 0) {
        return 0;
    }
    switch (work->sub_0x01) {
    case 0:
        switch (lb_choice_step(&work->sub_0x28)) {
        case 1:
            switch (work->sub_0x28.cursor_0x00) {
            case 0:
                work->sub_0x01 = 1;
                entry_reset(work);
                sysSE_req(5);
                break;
            case 1:
                work->sub_0x01 = 2;
                lb_room_move_open(work);
                sysSE_req(0);
                break;
            }
            break;
        case 2:
            result = 2;
            break;
        }
        break;
    case 1:
        switch (lb_room_place_step(work)) {
        case 1:
        case 2:
            work->sub_0x01 = 0;
            lb_room_place_list_build(work);
            cursor = work->sub_0x28.cursor_0x00;
            lb_room_menu_init(work);
            work->sub_0x28.cursor_0x00 = cursor;
            lb_menu_row_marks_clear();
            break;
        }
        break;
    case 2:
        switch (lb_room_move_step(work)) {
        case 1:
        case 2:
            work->sub_0x01 = 0;
            cursor = work->sub_0x28.cursor_0x00;
            lb_room_menu_init(work);
            work->sub_0x28.cursor_0x00 = cursor;
            lb_menu_row_marks_clear();
            break;
        }
        break;
    }
    return result;
}

/* Lists the placed models of this room's spots (their slots and marks: 1 placed, 2 also in the item box). */
void lb_room_slots_build(LbRoomWork* work)
{
    u8 base;
    u8 count;
    u8 i;
    u8 mark;

    work->slot_sel_0xAA = 0;
    work->slot_count_0xA4 = 0;
    memset(work->slots_0x93, 0xFF, 8);
    memset(work->slot_marks_0x9B, 0, 8);
    lb_menu_row_marks_clear();
    get_option_pair(lobby_w.area_0x002, &base, &count);
    work->slot_max_0xA6 = count;
    for (i = 0; i < count; i++) {
        if (lobby_w.place_ids_0x134[base + i] == 0xFF) {
            continue;
        }
        work->slots_0x93[work->slot_count_0xA4] = base + i;
        mark = 1;
        if (eft052_page_count_ck(note_slot_to_value(lobby_w.place_ids_0x134[base + i]), 1) > 0) {
            mark |= 2;
        }
        lb_menu_row_mark_set(i, mark);
        work->slot_marks_0x9B[work->slot_count_0xA4] = mark;
        work->slot_count_0xA4++;
    }
}

/* Puts the selected slot's model back into the item box and frees the slot. */
void lb_room_slot_release(LbRoomWork* work)
{
    u8 base;
    u8 count;
    u8 slot;

    if (work->slots_0x93[work->slot_sel_0xAA] == 0xFF) {
        return;
    }
    base = lobby_w.place_ids_0x134[work->slots_0x93[work->slot_sel_0xAA]];
    eft052_page_put(work->player_0x100, note_slot_to_value(base), 1, 1);
    slot = work->slots_0x93[work->slot_sel_0xAA];
    lobby_w.place_ids_0x134[slot] = 0xFF;
    lobby_world_block->place_0x483C[slot] = 0xFF;
    get_option_pair(lobby_w.area_0x002, &base, &count);
    place_model_release(work->slots_0x93[work->slot_sel_0xAA] - base);
}

/* Opens the move screen: the rooms unlocked so far, which of them hold models, and the here/elsewhere list. */
void lb_room_move_open(LbRoomWork* work)
{
    u16 off_mask = 0;
    u8 others;
    u8 room;
    u8 base;
    u8 count;
    u8 i;

    if (lb_unlock_cond_ck(37) == 1) {
        work->rooms_0xA8 = 4;
    } else if (lb_unlock_cond_ck(33) == 1) {
        work->rooms_0xA8 = 3;
    } else if (lb_unlock_cond_ck(29) == 1) {
        work->rooms_0xA8 = 2;
    } else {
        work->rooms_0xA8 = 1;
    }
    memset(work->room_used_0x8F, 1, 4);
    work->here_used_0x8D = 0;
    work->other_used_0x8E = 0;
    others = 0;
    for (room = 0; room < 4; room++) {
        if (lb_room_option_ids[room] != lobby_w.area_0x002) {
            work->other_rooms_0x89[others] = room;
            others++;
        }
        get_option_pair(lb_room_option_ids[room], &base, &count);
        work->room_used_0x8F[room] = 0;
        for (i = 0; i < count; i++) {
            if (lobby_w.place_ids_0x134[base + i] != 0xFF) {
                work->room_used_0x8F[room] = 1;
                if (lb_room_option_ids[room] == lobby_w.area_0x002) {
                    work->here_used_0x8D = 1;
                } else {
                    work->other_used_0x8E = 1;
                }
                break;
            }
        }
    }
    if (work->rooms_0xA8 == 1) {
        work->mode_0x02 = 1;
        lb_room_slots_build(work);
        lb_menu_row_mark_select(work->slots_0x93[work->slot_sel_0xAA]);
        return;
    }
    work->mode_0x02 = 0;
    memset(&work->move_0x48, 0, sizeof(work->move_0x48));
    if (work->here_used_0x8D == 0) {
        off_mask |= 1;
    }
    if (work->other_used_0x8E == 0) {
        off_mask |= 2;
    }
    lb_list_init(&work->move_0x48, 2, 5, 499, -1, off_mask, -1, 3, 0);
}

/* Sets the other-room list up (the rooms with no model greyed). */
void lb_room_move_list_init(LbRoomWork* work)
{
    u16 off_mask = 0;
    u8 i;

    for (i = 0; i < work->rooms_0xA8; i++) {
        if (work->room_used_0x8F[work->other_rooms_0x89[i]] == 0) {
            off_mask |= 1 << i;
        }
    }
    lb_list_init(&work->rooms_0x68, work->rooms_0xA8 - 1, 2, 0, -1, off_mask, 5, 3, 0);
}

/* Lists the picked room's placed models in the item-hold strip (`quiet` keeps a refresh silent). */
void lb_room_box_list_set(LbRoomWork* work, u8 quiet)
{
    CockpitHoldEntry entry;
    u8 base;
    u8 count;
    s16 rows;
    u8 used;
    s16 i;

    get_option_pair(lb_room_option_ids[work->other_rooms_0x89[work->rooms_0x68.cursor_0x00]], &base, &count);
    memset(work->items_0xAC, 0, sizeof(work->items_0xAC));
    rows = 0;
    used = 0;
    for (i = 0; i < count; i++) {
        if (lobby_w.place_ids_0x134[base + i] != 0xFF) {
            work->items_0xAC[i] = note_slot_to_value(lobby_w.place_ids_0x134[base + i]);
            used++;
        } else {
            work->items_0xAC[i] = 0;
        }
        rows++;
    }
    if (used == 0) {
        memset(work->items_0xAC, 0, sizeof(work->items_0xAC));
        rows = 0;
        work->room_used_0x8F[work->other_rooms_0x89[work->rooms_0x68.cursor_0x00]] = 0;
    }
    entry.limit_0x00 = 0;
    entry.msg_0x04 = -1;
    entry.msg2_0x06 = -1;
    entry.count_msg_0x0A = -1;
    entry.title_0x08 = 462;
    entry.sprite_0x0E = 5096;
    entry.highlight_0x10 = 0;
    entry.item_count_0x0C = rows;
    entry.items_0x14 = work->items_0xAC;
    entry.kind_0x18 = 2;
    if (quiet == 0 || used == 0) {
        eft052_hold_entry_set(&entry, 0);
    }
}

/* Steps the move screen: here/elsewhere, the slot list of this room, the slot's take-back question, the other rooms
 * and their hold strip; 2 once cancelled. */
s32 lb_room_move_step(LbRoomWork* work)
{
    s32 result = 0;
    CockpitHoldBlock* hold = (CockpitHoldBlock*)cockpit_hold_block;
    s16 cursor;
    u16 item;
    u8 base;
    u8 count;

    switch (work->mode_0x02) {
    case 0:
        switch (lb_choice_step(&work->move_0x48)) {
        case 1:
            switch (work->move_0x48.cursor_0x00) {
            case 0:
                work->mode_0x02 = 1;
                work->move_flag_0x88 = 0;
                lb_room_slots_build(work);
                lb_room_slot_preview(work);
                lb_menu_row_mark_select(work->slots_0x93[work->slot_sel_0xAA]);
                sysSE_req(0);
                break;
            case 1:
                work->mode_0x02 = 3;
                lb_room_move_list_init(work);
                sysSE_req(5);
                break;
            }
            break;
        case 2:
            result = 2;
            sysSE_req(1);
            break;
        }
        break;
    case 3:
        switch (lb_choice_step(&work->rooms_0x68)) {
        case 1:
            work->mode_0x02 = 4;
            lb_room_box_list_set(work, 0);
            break;
        case 2:
            work->mode_0x02 = 0;
            cursor = work->move_0x48.cursor_0x00;
            lb_room_move_open(work);
            work->move_0x48.cursor_0x00 = cursor;
            break;
        }
        break;
    case 4:
        switch (eft052_hold_step()) {
        case 1:
            eft052_hold_row_get(&item, NULL);
            eft052_page_put(work->player_0x100, item, 1, 1);
            get_option_pair(lb_room_option_ids[work->other_rooms_0x89[work->rooms_0x68.cursor_0x00]], &base, &count);
            lobby_w.place_ids_0x134[base + hold->row_0x01] = 0xFF;
            lobby_world_block->place_0x483C[base + hold->row_0x01] = 0xFF;
            hold->rows_0x10[hold->row_0x01].id_0x00 = 0;
            hold->rows_0x10[hold->row_0x01].count_0x02 = 0;
            lb_room_box_list_set(work, 1);
            break;
        case 2:
            if (work->room_used_0x8F[work->other_rooms_0x89[work->rooms_0x68.cursor_0x00]] == 0) {
                work->mode_0x02 = 0;
                cursor = work->move_0x48.cursor_0x00;
                lb_room_move_open(work);
                work->move_0x48.cursor_0x00 = cursor;
            } else {
                work->mode_0x02 = 3;
            }
            break;
        }
        break;
    case 1:
        if (lb_cmd_pressed_ck(0x10) && work->slot_count_0xA4 > 0 && (work->slot_marks_0x9B[work->slot_sel_0xAA] & 2)) {
            work->mode_0x02 = 2;
            work->toggle_0xE8 = 0;
            sysSE_req(0);
        } else if (lb_cmd_pressed_ck(0x20)) {
            if (work->rooms_0xA8 == 1) {
                result = 2;
            } else {
                work->mode_0x02 = 0;
                cursor = work->move_0x48.cursor_0x00;
                lb_room_move_open(work);
                work->move_0x48.cursor_0x00 = cursor;
            }
            lb_menu_row_marks_clear();
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(12)) {
            work->slot_sel_0xAA = menu_cursor_step_open_last(work->slot_sel_0xAA, work->slot_count_0xA4,
                                                             lb_cmd_repeat_get(), 4, 8, -1);
            if (work->slot_count_0xA4 > 1) {
                sysSE_stop(6);
            }
            lb_room_slot_preview(work);
            lb_menu_row_mark_select(work->slots_0x93[work->slot_sel_0xAA]);
        }
        break;
    case 2:
        switch (toggle_word_step(&work->toggle_0xE8, lobby_cmd_trig_get(), 4, 8, (u16)0xFFFF)) {
        case 1:
            lb_room_slot_release(work);
            lb_room_slots_build(work);
            sysSE_stop(8);
            if (work->slot_count_0xA4 == 0) {
                result = 2;
                sysSE_req(1);
            } else {
                work->mode_0x02 = 1;
                lb_menu_row_mark_select(work->slots_0x93[work->slot_sel_0xAA]);
                lb_room_slot_preview(work);
            }
            /* fall through */
        case 2:
            work->mode_0x02 = 1;
            break;
        }
        break;
    }
    return result;
}

} /* extern "C" */

extern "C" {

/* Lists the placeable models the item box holds (the note goods first, then the flat list) in the hold strip. */
void entry_list_update(LbRoomWork* work)
{
    CockpitHoldEntry entry;
    s16 count;
    s32 kind;
    s32 i;
    u16* goods;

    memset(work->items_0xAC, 0, sizeof(work->items_0xAC));
    count = 0;
    for (kind = 0; kind < 4; kind++) {
        goods = note_slot_table[kind];
        for (i = 0; i < 6; i++, goods++) {
            if (userdata_item_count_total(*goods, (u8*)lobby_world_block) != 0) {
                work->items_0xAC[count] = *goods;
                count++;
            }
        }
    }
    for (goods = note_slot_flat_table; *goods != 0; goods++) {
        if (userdata_item_count_total(*goods, (u8*)lobby_world_block) != 0) {
            work->items_0xAC[count] = *goods;
            count++;
        }
    }
    entry.limit_0x00 = 0;
    entry.msg_0x04 = 451;
    entry.msg2_0x06 = -1;
    entry.count_msg_0x0A = -1;
    entry.title_0x08 = 462;
    entry.sprite_0x0E = 5096;
    entry.highlight_0x10 = 0;
    entry.item_count_0x0C = 30;
    entry.items_0x14 = work->items_0xAC;
    entry.kind_0x18 = 0;
    eft052_hold_entry_set(&entry, 0);
}

/* The free spots of this room. */
s16 lb_room_place_count_get(void)
{
    u8 base;
    u8 count;
    s16 free;
    u8 i;

    get_option_pair(lobby_w.area_0x002, &base, &count);
    free = 0;
    for (i = 0; i < count; i++) {
        if (lobby_w.place_ids_0x134[base + i] == 0xFF) {
            free++;
        }
    }
    return free;
}

/* Lists this room's free spots (their slots, each row marked 3). */
void lb_room_place_list_build(LbRoomWork* work)
{
    u8 base;
    u8 count;
    u8 i;

    work->slot_sel_0xAA = 0;
    work->slot_count_0xA4 = 0;
    memset(work->slots_0x93, 0xFF, 8);
    lb_menu_row_marks_clear();
    get_option_pair(lobby_w.area_0x002, &base, &count);
    work->slot_max_0xA6 = count;
    for (i = 0; i < count; i++) {
        if (lobby_w.place_ids_0x134[base + i] != 0xFF) {
            continue;
        }
        work->slots_0x93[work->slot_count_0xA4] = base + i;
        lb_menu_row_mark_set(i, 3);
        work->slot_count_0xA4++;
    }
}


/* Opens the place sub-screen (step 0) and lists the item box's placeable models. */
void entry_reset(LbRoomWork* work)
{
    work->mode_0x02 = 0;
    entry_list_update(work);
}

/* Places the hold strip's model on the selected spot: takes it from the item box and loads its model. */
void lb_room_place_put(LbRoomWork* work)
{
    u8 base = 0;
    u8 count = 0;
    u16 item;

    eft052_hold_row_get(&item, NULL);
    eft052_page_count_add(item, 1);
    lobby_w.place_ids_0x134[work->slots_0x93[work->slot_sel_0xAA]] = note_value_to_slot(item);
    lobby_world_block->place_0x483C[work->slots_0x93[work->slot_sel_0xAA]] = note_value_to_slot(item);
    get_option_pair(lobby_w.area_0x002, &base, &count);
    place_model_load(1, work->slots_0x93[work->slot_sel_0xAA] - base, note_value_to_slot(item), lobby_w.field_0x001,
                     lobby_w.area_0x002);
}

/* Steps the place sub-screen: the hold strip, the spot list and the place question; 2 once done. */
s32 lb_room_place_step(LbRoomWork* work)
{
    s32 result = 0;

    switch (work->mode_0x02) {
    case 0:
        switch (eft052_hold_step()) {
        case 1:
            work->mode_0x02 = 1;
            lb_room_place_list_build(work);
            lb_room_slot_preview(work);
            lb_menu_row_mark_select(work->slots_0x93[work->slot_sel_0xAA]);
            break;
        case 2:
            result = 2;
            break;
        }
        break;
    case 1:
        if (lb_cmd_pressed_ck(0x10)) {
            work->mode_0x02 = 2;
            work->toggle_0xE8 = 0;
            sysSE_req(0);
        } else if (lb_cmd_pressed_ck(0x20)) {
            work->mode_0x02 = 0;
            lb_menu_row_marks_clear();
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(12)) {
            work->slot_sel_0xAA = menu_cursor_step_open_last(work->slot_sel_0xAA, work->slot_count_0xA4,
                                                             lb_cmd_repeat_get(), 4, 8, -1);
            if (work->slot_count_0xA4 > 1) {
                sysSE_stop(6);
            }
            lb_room_slot_preview(work);
            lb_menu_row_mark_select(work->slots_0x93[work->slot_sel_0xAA]);
        }
        break;
    case 2:
        switch (toggle_word_step(&work->toggle_0xE8, lobby_cmd_trig_get(), 4, 8, (u16)0xFFFF)) {
        case 1:
            lb_room_place_put(work);
            entry_list_update(work);
            sysSE_stop(7);
            if (lb_room_place_count_get() != 0) {
                work->mode_0x02 = 0;
                lb_menu_row_marks_clear();
            } else {
                result = 2;
            }
            break;
        case 2:
            work->mode_0x02 = 1;
            break;
        }
        break;
    }
    return result;
}

/* Resets the box sub-screen's state. */
void entry_state_reset(LbRoomWork* work)
{
    work->sub_0x01 = 0;
    work->lock_sel_0xEC = 0;
    work->field_0x14C = 0;
    work->toggle_0xE8 = 0;
}

/* Opens the lock sub-screen with the room's current lock and password. */
void lb_room_lock_open(LbRoomWork* work)
{
    work->sub_0x01 = 0;
    work->lock_sel_0xEC = 0;
    work->lock_0xEE = lobby_world_block->room_lock_0x3F0A;
    memcpy(work->pass_0xF0, lobby_world_block->room_pass_0x40FC, 5);
}

/* Steps the lock sub-screen: the lock list, the password keyboard and the confirmation; 2 once cancelled. */
s32 lb_room_lock_step(LbRoomWork* work)
{
    s32 result = 0;

    switch (work->sub_0x01) {
    case 0:
        if (lb_cmd_pressed_ck(0x10)) {
            work->lock_0xEE = work->lock_sel_0xEC;
            if (work->lock_0xEE == 2) {
                work->sub_0x01 = 1;
                work->kbd_0x04 = 0;
                memcpy(work->input_0xF5, work->pass_0xF0, 5);
                sysSE_req(5);
            } else {
                work->toggle_0xE8 = 0;
                work->sub_0x01 = 2;
                sysSE_req(0);
            }
        } else if (lb_cmd_pressed_ck(0x20)) {
            sysSE_req(1);
            result = 2;
        } else if (lb_cmd_repeat_ck(3)) {
            work->lock_sel_0xEC = menu_cursor_step(work->lock_sel_0xEC, 3, lb_cmd_repeat_get(), 1, 2);
        }
        break;
    case 1:
        switch (work->kbd_0x04) {
        case 0:
            if (kbd_open(4) == 1) {
                menu_work.slot[0].field_0x32D = 1;
                set_kbd_param(work->input_0xF5, 8);
                work->kbd_0x04++;
            }
            break;
        case 1:
            switch (kbd_move()) {
            case 1:
                menu_work.slot[0].field_0x32D = 0;
                if (work->input_0xF5[0] == 0 || memcmp(work->pass_0xF0, work->input_0xF5, 5) == 0) {
                    work->sub_0x01 = 0;
                    work->lock_0xEE = lobby_world_block->room_lock_0x3F0A;
                } else {
                    memcpy(work->pass_0xF0, work->input_0xF5, 5);
                    work->sub_0x01 = 2;
                    work->toggle_0xE8 = 0;
                }
                break;
            case -1:
                menu_work.slot[0].field_0x32D = 0;
                work->sub_0x01 = 0;
                work->lock_0xEE = lobby_world_block->room_lock_0x3F0A;
                break;
            }
            break;
        }
        break;
    case 2:
        switch (toggle_word_step(&work->toggle_0xE8, lobby_cmd_trig_get(), 4, 8, (u16)0xFFFF)) {
        case 1:
            result = 1;
            lobby_world_block->room_lock_0x3F0A = work->lock_0xEE;
            if (work->lock_0xEE != 2) {
                memset(work->pass_0xF0, 0, 5);
            }
            memcpy(lobby_world_block->room_pass_0x40FC, work->pass_0xF0, 5);
            if (work->lock_0xEE == 1 || work->lock_0xEE == 2) {
                sysSE_stop(28);
            } else if (work->lock_0xEE == 0) {
                sysSE_stop(29);
            }
            break;
        case 2:
            work->sub_0x01 = 0;
            work->lock_0xEE = lobby_world_block->room_lock_0x3F0A;
            memcpy(work->pass_0xF0, lobby_world_block->room_pass_0x40FC, 5);
            break;
        }
        break;
    }
    return result;
}

/* Draws an item slot's icon, with its blinking frame when `blink` and `active` are set and the pointer is idle. */
void lb_item_slot_draw(const _mh_ivec2_* pos, u16 item, u16 anim, s32 blink, s32 active)
{
    if (blink != 0 && active != 0 && chk_pointer() == 0) {
        set_blendmode(4, 1, 1);
        draw_sprite_anim_idx(8904, anim, pos);
        set_blendmode(4, 5, 1);
    }
    if (item != 0) {
        draw_itemicon_item_id_idx(8907, item, pos);
    }
}

/* Draws the item-box panel: its frame, its rows and its count. */
void lb_item_box_panel_draw(void)
{
    _mh_ivec2_ pos;

    get_lsp_data(8864, &pos);
    draw_sprite_ary(lb_item_box_frame_sprites, &pos);
    lb_item_box_rows_draw(&eft052_item_box);
    lb_item_box_count_draw(&eft052_item_box, 8863, 0);
}

/* Draws the room screen: its top menu, or the place / Wii Remote data / lock sub-screen, then the room list. */
void lb_screen24_draw(void)
{
    LbRoomWork* work = lobby_w.room_0x0AC;
    _mh_ivec2_ pos;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    switch (work->step_0x00) {
    case 1:
        if (game_ready_ck() == 1) {
            lb_list_draw_ex(&work->menu_0x08, 0x2250, lb_menu_frame_sprites, lb_menu_row_sprites, 0x1877, 0,
                            lb_room_menu_titles);
            lb_panel_msg_draw(0x1877, lb_room_menu_helps[work->menu_0x08.cursor_0x00]);
        } else {
            lb_choice_draw(&work->menu_0x08, 0x2250, lb_menu_frame_sprites, lb_menu_row_sprites, 0x1877, 0);
        }
        get_lsp_data(0x2250, &pos);
        draw_sprite_anim_ary(lb_menu_title_sprites, work->menu_0x08.count_0x06, &pos);
        draw_sprite_idx(0x2258, &pos);
        break;
    case 2:
        lb_room_menu_draw(work);
        break;
    case 3:
        lb_room_wpad_draw(work);
        break;
    case 4:
        lb_room_lock_draw(work);
        break;
    }
    lb_room_list_draw(work);
}

/* Draws screen 0x1E: the room-layout list, and its confirmation. */
void lb_screen30_draw(void)
{
    LbScreen30Work* work = lobby_w.screen30_0x0AC;
    _mh_ivec2_ pos;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    switch (work->step_0x00) {
    case 1:
        lb_choice_draw(&work->list_0x04, 0x2250, lb_menu_frame_sprites, lb_menu_row_sprites, 0x1877, 0);
        get_lsp_data(0x2250, &pos);
        draw_sprite_anim_ary(lb_menu_title_sprites, work->list_0x04.count_0x06, &pos);
        draw_sprite_idx(0x2256, &pos);
        lb_panel_msg_draw(0x1877, 0x1D9);
        break;
    case 2:
        lb_choice_draw(&work->list_0x04, 0x2250, lb_menu_frame_sprites, lb_menu_row_sprites, 0x1877, 1);
        get_lsp_data(0x2250, &pos);
        draw_sprite_anim_ary(lb_menu_title_sprites, work->list_0x04.count_0x06, &pos);
        draw_sprite_idx(0x2256, &pos);
        lb_panel_msg_draw(0x1877, 0x1DA);
        lb_panel_str_print(0x1877, 0x1DB, 2);
        lb_panel_yes_no_draw(0x1879, work->yes_no_0x24);
        break;
    }
}

/* Draws the room-lock icon of `lock` (1 open, 2 password, 0 closed). */
void lb_room_lock_icon_draw(const _mh_ivec2_* pos, u8 lock)
{
    switch (lock) {
    case 1:
        draw_sprite_idx(8761, pos);
        draw_sprite_idx(8760, pos);
        break;
    case 2:
        draw_sprite_idx(8767, pos);
        draw_sprite_idx(8766, pos);
        draw_sprite_idx(8765, pos);
        break;
    case 0:
        draw_sprite_idx(8764, pos);
        draw_sprite_idx(8762, pos);
        draw_sprite_idx(8763, pos);
        break;
    }
}

/* Points the lobby camera at the selected spot's half of room 7 (spots 4.. on the far side). */
void lb_room_slot_preview(LbRoomWork* work)
{
    u8 base;
    u8 count;

    if (lobby_w.area_0x002 == 7) {
        get_option_pair(lobby_w.area_0x002, &base, &count);
        if (work->slots_0x93[work->slot_sel_0xAA] - base >= 4) {
            camera_lobby_view_set(4, 1);
        } else {
            camera_lobby_view_set(3, 1);
        }
    }
}

/* Spawns the eight-model effect (kind 0x38, variant 0) for `owner`. */
void eft_em_model_ring_attach(void* owner /* untyped: opaque handle - the owner the effect follows */)
{
    _EFT* self;
    PopEftModels* models;
    s32 i;

    self = (_EFT*)eft_res_slot_get(44);
    if (self == NULL) {
        return;
    }
    self->type_0x02 = 0;
    self->release_0x40 = eft_em_model_pair_release;
    self->dispatch_0x34 = eft_em_model_pair_update;
    models = (PopEftModels*)self->work_0x38;
    models->count_0x00 = 8;
    models->field_0x24 = 0;
    for (i = 0; i < models->count_0x00; i++) {
        models->models_0x04[i] = (EftModel*)eft_res_model_get();
        if (models->models_0x04[i] == NULL) {
            eft_res_slot_release(self);
            return;
        }
    }
    self->rot_0x24.x = 0;
    self->rot_0x24.y = 0;
    self->rot_0x24.z = 0;
    eft_state_flags_set(self, 1, 0);
    self->field_0x03 = 56;
    self->source_0x30 = owner;
}

/* Spawns the two-model effect (kind 0x38, variant 1) on the enemy, in its area. */
void eft_em_model_pair_attach(struct _ENEMY_WORK* enemy)
{
    _EFT* self;
    PopEftModels* models;
    s32 i;

    if (enemy == NULL) {
        return;
    }
    self = (_EFT*)eft_res_slot_get(44);
    if (self == NULL) {
        return;
    }
    self->type_0x02 = 1;
    self->release_0x40 = eft_em_model_pair_release;
    self->dispatch_0x34 = eft_em_model_pair_update;
    models = (PopEftModels*)self->work_0x38;
    models->count_0x00 = 2;
    models->field_0x24 = 0;
    for (i = 0; i < models->count_0x00; i++) {
        models->models_0x04[i] = (EftModel*)eft_res_model_get();
        if (models->models_0x04[i] == NULL) {
            eft_res_slot_release(self);
            return;
        }
    }
    self->rot_0x24.x = 0;
    self->rot_0x24.y = 0;
    self->rot_0x24.z = 0;
    eft_state_flags_set(self, 1, 0);
    self->field_0x03 = 56;
    self->source_0x30 = enemy;
    self->flag_0x01 = 1;
    self->area_0x44 = enemy->area_no;
}

/* Releases the effect's models. */
void eft_em_model_pair_release(_EFT* self)
{
    PopEftModels* models = (PopEftModels*)self->work_0x38;

    eft_res_models_release(models->models_0x04, models->count_0x00);
    models->count_0x00 = 0;
}

/* Steps the effect: create, move, hold, end. */
void eft_em_model_pair_update(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        eft_em_model_pair_create(self);
        break;
    case 1:
        eft_em_model_pair_move(self);
        break;
    case 2:
        counter_inc(self);
        break;
    case 3:
        eft_em_model_pair_end(self);
        break;
    }
}

/* Creates the effect's models at the variant's scale and colour, then places them. */
void eft_em_model_pair_create(_EFT* self)
{
    PopEftModels* models = (PopEftModels*)self->work_0x38;
    f32 scale = eft_em_model_pair_scales[self->type_0x02];
    s32 i;

    self->state_0x05++;
    for (i = 0; i < models->count_0x00; i++) {
        if (res_eft_model_create((MHchar*)models->models_0x04[i], eft_em_model_pair_ids[self->type_0x02], 4) == NULL) {
            eft_em_model_pair_end(self);
            return;
        }
        setVector3(&models->models_0x04[i]->pos_0x1C, scale, scale, scale);
    }
    color_rgba_copy(models->color_0x26, (u8*)&eft_em_model_pair_colors[self->type_0x02]);
    eft_em_model_pair_move(self);
}

/* Advances the effect's step. */
void counter_inc(_EFT* self)
{
    self->state_0x05++;
}

/* Ends the effect: hands its slot back. */
void eft_em_model_pair_end(_EFT* self)
{
    eft_res_slot_release(self);
}


} /* extern "C" */
