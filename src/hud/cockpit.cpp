/* hud/cockpit - the in-game cockpit HUD's shared helpers: the blink clock, the HUD notice pool, the page arrows, the
 *   remote-pointer pick page, the item-message slots, the button guides, the notice band and the player name tags.
 * RANGE. .text 0x802D9EA4-0x802E0480 (126 functions, all written); .data 0x805D54A0-0x805D5740,
 *   .bss 0x806BD808-0x806BDCC8, .sdata 0x807925A0-0x807927B0, .sbss 0x80794B78-0x80794B80, .sdata2
 *   0x8079A878-0x8079A8C0, extab/extabindex (the unit's block in config/RMHE08/splits.txt).  Every data object is
 *   defined here, in the target's order and byte-identical; the string pair at 0x805D55C8 is the one `cockpit.cpp`
 *   `__FILE__` literal and its assert message.
 * RANGE. The TU ends at 0x802E0480: the six functions after it (`get_lsp_data_ptr` .. `get_lsp_anim`) and their data
 *   are `hud/layout.cpp`'s - `get_wide_offset` loads its 0.5f from 0x8079A8C0 (the layout pool) while this TU pools
 *   0.5f at 0x8079A878.
 * FLAGS. `cflags_main` (the `ai` lib entry in configure.py) plus `#pragma peephole off` (retail keeps the u8/u16
 *   `clrlwi` before narrow stores and calls: 13 notice rows at 65..92 % turn 100 %), `#pragma pool_data off` (each
 *   table gets its own lis/addi: `hud_notice_scroll_update` 82.95 -> 100) and `#pragma fp_contract off` (fmuls+fadds in
 *   the blink scale: `draw_sprite_blink_idx` 97.74 -> 100).
 * NAMES. GUESS (from each body and its callers): every `hud_notice_*`, `ainpc_page_*`, `cockpit_msg_slot_*`,
 *   `cockpit_notice_*`, `hud_*`, `button_guide_*`, `put_button_guide_r`, `str_tbl_34_get`, `monster_mark_*`,
 *   `draw_digit_rect`, `cockpit_blink*`, `money_frame_ids`, `chara_icon_height_tbl`, `rare_color_tbl`, `player_color_tbl`,
 *   `page_count_frame_ids` and the struct names (the pointer page keeps the map's earlier `ainpc_page_*` stem).  The
 *   registry names this unit proposed for foreign rows are GUESS too: `math_sin_rad`, `atlas_digit_uv_get`,
 *   `atlas_icon_uv_get`, `pointer_reset`, `kpad_pointer_read`, `pointer_update`, `pointer_dpd_state`,
 *   `se_req_page_toggle`, `se_item_get_play`, `place_rec_copy`, `word_read`, `em_hit_parts_get`, `em_tall_ck`,
 *   `em_part_sphere_get`, `place_list_highlight_draw`, `pl_root_lock_ck`, `item_bonus_class_get`, `chara_icon_tex_tbl`.
 * RESIDUALS. `setCockpitTransferMode` 82.5: retail stores the s32 `mode` raw after narrowing only `slot`; peephole off
 *   adds an `extsb`, the scoped `peephole on` drops both (an s8 parameter matches but clashes with the s32 declaration
 *   `unsplit/menu.h` and `hud/lobby_hud.cpp` carry).
 *   `cockpit_notice_show` 89.8: the `1 << kind` mask is shifted into r0 and narrowed into r5; retail shifts and
 *   narrows in r5.
 *   `cockpit_notice_update` 97.8: retail sets `ofs` after the loop's pointer induction (`li r8` after `mr r7`).
 *   `ainpc_page_select_button_ck` 96.6: the inversion needs `else if (down == 0)` to stay branched (a plain `else`
 *   becomes `subfic/or/srwi`); ours keeps the redundant compare.
 *   `ainpc_page_label_draw` 98.4: retail tests steps 2..3 as one unsigned range (`subi; cmplwi 1`), ours as two
 *   signed compares.
 *   `hud_edge_dir_get` 99.8: the four `out->x` sums land in r3 where retail uses r0, and the unmoved branch rotates
 *   r4/r5/r6.
 *   `cockpit_msg_slot_update` 99.3: `ef/eft052.h` declares `hud_item_msg_push` returning s32 and `hud_msg_sub_set`
 *   taking u8, so ours `extsb`s the stored id and drops the `extsb` before the call; declaring s8 for both gives 100
 *   (the owner's definitions would change with it; requests hud-cockpit#3/#4).
 *   `ainpc_page_target_pick` 99.9: retail pairs each by-value cursor copy with its label result (0x38/0x3C,
 *   0x30/0x34, ...), ours pools the copies apart; an inline label helper pairs them but in the other order (99.89).
 *   `ainpc_page_cursor_in_box_ck` 99.7: retail loads the box offset before the cursor and the box position; no operand
 *   order of the sums moves the loads.
 *   `cockpit_msg_slot_choice_a/b_build` 99.4/99.2: the hoisted 0/1 constants take r31/r30 the other way round
 *   (no statement order moves them).
 * SHAPES. The range tests are spelled as MWCC's own u8 forms (`(u8)(step + 253) <= 1`); a `switch` with default
 *   first places the default arm right after the compare chain (`ainpc_page_open_ck`, `item_get_with_msg`,
 *   `ainpc_page_label_draw`); a `_mh_ivec2_` result is bound to a `const&` before `uv_pair_copy` (no copy
 *   temporary); `put_button_guide_r` copies the frame sprite through the word view `HudSprWords` (retail's lwz/stw
 *   pairs).
 *   `hud_edge_dir_get` takes `const` source points (they stay cached across the stores to `out`), adds each
 *   offset into an s16 local with `+=` (the sum is stored raw, then `extsh`ed for the compare) and loads both source
 *   coordinates into locals before the margins; the message-slot rows pick layout ids with a ternary (the default
 *   `li` lands after the compare), declare the s16 anchor offsets first (r31/r30) and test `choice != 0` before the
 *   switch; `prompt` is u8 and `rows` u8 so the u16/s16 parameters get retail's `clrlwi`/`extsh`.
 *   Retail narrows the u8 results of `get_ControlType`, `note_timer_lobby_ready_ck`, `mailbox_unread_ck` and
 *   `ck_WideMode` at the call (`(u8)`/`(u32)` casts there); a u8 field compared with `== 4` is `(s32)` for `cmpwi`;
 *   `x / 2.0f` keeps retail's `fmuls x,0.5` operand and load order (`ainpc_page_label_pos_get`); a `(u16)` before the
 *   `& 0xFFF0` mask keeps retail's `clrlwi` (`ainpc_page_label_text_draw`); the confirm clock is incremented as a
 *   statement and re-read for its range test; the monster count and pick declare the enemy pointer first; the cursor
 *   draw switch takes `default` first and declares `id` before the flags; the pad move centres with
 *   `(s32)(screen.x / 2.0f)` (retail `extsh`es the s16); the pointer ray stages its forward scale in a float local and
 *   declares the scalars in retail's stack order; the trail loop starts with `i = 1, alpha = step`; the name tag's
 *   rank colour is a ternary; the guide width adds `w < 0 ? -w : w` (the `extsh` lands at the join).
 */

#include "types.h"
#include "main.h"
#include "hud/layout.h"
#include "hud/cockpit.h"
#include "hud/cockpit_work.h"
#include "hud/draw_frame_rect_z.h"
#include "hud/cockpit_blink_get.h"
#include "ai/fn_802D44F4.h"
#include "ef/math_sin_rad.h"
#include "atlas_digit_uv_get.h"
#include "ef/fn_800CDB2C.h"
#include "fn_8004CAD8.h"
#include "menu/menu_item.h"
#include "sound/set_zmode__FbUcb.h"
#include "sound/fn_800D7F54.h"
#include "sound/se_req_page_toggle.h"
#include "menu/get_pop_dat_ptr.h"
#include "menu/menu_row.h"
#include "ef/eft052.h"
#include "ef/sys_quest_move.h"
#include "ef/get_move_work_adrs.h"
#include "fn_80047398.h"
#include "fn_80056F24.h"
#include "pad_hold_ck.h"
#include "pointer_reset.h"
#include "kpad_pointer_read.h"
#include "mh3_pad/system_w.h"
#include "lobby/lobby_w.h"
#include "lobby/event_demo_running_ck.h"
#include "lobby/lb_server_sel_trans.h"
#include "lobby/lb_quest_work_active_ck.h"
#include "WPAD/wpad.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/printf.h"
#include "mh3_pad.h"
#include "mh3_pad/vec3.h"
#include "mh3_pad/Psw.h"
#include "mh3_pad/control.h"
#include "g3d/g3d_calcworld.h"
#include "g3d/g3d_camera.h"
#include "g3d/scn_root_get_current_camera.h"
#include "ef/pRoot.h"
#include "ef/eft_capsule_ctor.h"
#include "ef/effect.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/em_hit_parts_get.h"
#include "enemy/fn_8012BDF4.h"
#include "stage/stg_w.h"
#include "enemy/em_tall_ck.h"
#include "Pl/pl_hit.h"   /* em_part_sphere_get (rule 2: the owner's header) */
#include "lobby/lb_npc_callees.h"
#include "place_rec_copy.h"
#include "word_read.h"
#include "pointer_update.h"
#include "pointer_dpd_state.h"
#include "pointer_center.h"
#include "font/flfnt.h"
#include "ef/system_core.h"
#include "nw4r/db_assert.h"
#include "menu/place_list_highlight_draw.h"
#include "Pl/pl_root_lock_ck.h"
#include "hud/put_lsp_anchor_offset.h"
#include "hud/ainpc_page_pos_set.h"
#include "id_value.h"
#include "menu/menu_message.h"
#include "menu/menu_cursor_column_step.h"
#include "menu/menu_frame_draw_page.h"
#include "menu/menu_choice_window_draw.h"
#include "menu/menu_frame_page_draw.h"
#include "hud/cockpit_state.h"
#include "hud/draw_lsp_sprite_variant.h"
#include "hud/put_button_guide.h"
#include "hud/put_button_guide_b.h"
#include "hud/put_button_guide_b_idx.h"
#include "hud/put_button_guide_c.h"
#include "hud/put_button_guide_c_idx.h"
#include "hud/cockpit_lobby_reset.h"
#include "hud/hud_mail_notice_clear.h"
#include "hud/ainpc_page_mode2_set.h"
#include "lobby/lb_equip_page.h"
#include "atlas_icon_uv_get.h"
#include "menu/chara_icon_tex_tbl.h"
#include "menu/mailbox_row_offset_inc.h"
#include "menu/mailbox_unread_ck.h"
#include "lobby/note_timer_lobby_ready_ck.h"
#include "hud/cockpit_msg_slot_clear.h"
#include "hud/cockpit_msg_slot_busy_ck.h"
#include "hud/cockpit_msg_slot_update.h"
#include "hud/cockpit_msg_slot_draw.h"
#include "hud/cockpit_msg_slot_split_draw.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/pl_item_add.h"
#include "Pl/pl_item_page2_ck.h"
#include "Pl/item_bonus_class_get.h"
#include "sound/se_item_get_play.h"
#include "fn_80047398.h"

#pragma peephole off
#pragma pool_data off
#pragma fp_contract off

/* The cockpit blink clock: a 40-frame counter and the 0..1 pulse derived from it.  size: 0x8 */
typedef struct CockpitBlink {
    /* +0x00 */ u8 clock;
    /* +0x01 */ u8 pad_0x01[3];
    /* +0x04 */ f32 value;
} CockpitBlink;

/* `HudPos3`/`HudSize2` (the textured-rectangle helper's by-value arguments) live in `hud/cockpit.h`. */

/* One row of the kind-0 notice table: its sprite id run and two mode bytes.  size: 0x8 */
typedef struct HudNoticeAnimRow {
    /* +0x00 */ const u16* ids;
    /* +0x04 */ u8 additive;    /* drawn with the additive blend mode */
    /* +0x05 */ u8 one_shot;    /* the notice dies at the end of the run instead of holding */
    /* +0x06 */ u8 pad_0x06[2];
} HudNoticeAnimRow;

typedef void (*HudNoticeFunc)(HudNotice* notice);

/* The cockpit's notice band: the quest (0), new-mail (1), note (2) and lobby (3) notices.  size: 0x10 (the clear
 * covers 0xE) */
typedef struct CockpitNotice {
    /* +0x00 */ u8 frame_flags;   /* bit 3: the lobby notice was kept up this frame */
    /* +0x01 */ u8 active;        /* the shown notices, one bit each */
    /* +0x02 */ u8 fresh;         /* the notices still popping in */
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ s16 timer[4];     /* each notice's animation frame */
    /* +0x0C */ s16 clock;        /* the slide clock the settled notices share */
    /* +0x0E */ u8 pad_0x0E[2];
} CockpitNotice;

/* The joint holder `_PLW::physics_0x13C` points at: the player's `MHchar` sits at +0x04.
 * size: 0x4 + sizeof(MHchar) (approximate: only the embedded character is read) */
typedef struct HudPlwJoints {
    /* +0x00 */ u8 pad_0x00[4];
    /* +0x04 */ MHchar chr_0x04;
} HudPlwJoints;

/* A sprite record copied word by word (the copy `put_button_guide_r` makes of each frame sprite).  size: 0x24 */
typedef union HudSprWords {
    /* +0x00 */ _SPR_DATA_ spr;
    /* +0x00 */ u32 words[9];
} HudSprWords;

/* The lobby world block as the money panel reads it.  size: 0x1C (approximate: the field read + 4) */
typedef struct HudLobbyMoney {
    /* +0x00 */ u8 unused_0x00[0x18];
    /* +0x18 */ s32 money;        /* the save's zenny */
} HudLobbyMoney;

/* The list cursor of a message slot's item page; laid out as the head of `MenuListWork`, which the column step
 * takes.  size: 0x8 */
typedef struct CockpitMsgList {
    /* +0x00 */ u16 input;     /* the pad bits the cursor steps read */
    /* +0x02 */ u16 step;      /* the step bits the page arrows light */
    /* +0x04 */ s8 page;       /* the shown page */
    /* +0x05 */ s8 pages;      /* the page count */
    /* +0x06 */ s8 column;     /* the cursor column */
    /* +0x07 */ s8 columns;    /* the entries per page */
} CockpitMsgList;

/* One item-message slot: the message of a got item and, when the pouch is full, the dialog that swaps an item out.
 * size: 0x130 */
typedef struct CockpitMsgSlot {
    /* +0x000 */ s16 hold;           /* frames the slot stays with its ticket holder */
    /* +0x002 */ u16 ticket;         /* the current holder's ticket */
    /* +0x004 */ u8 state;           /* 0 idle, 1 message, 2 question shown, 3 asking, 4 picking, 5 confirming */
    /* +0x005 */ u8 count;           /* how many of the item */
    /* +0x006 */ s8 msg_id;          /* the HUD message the slot pushed (-1 none yet) */
    /* +0x007 */ u8 msg_delay;       /* frames before the question text replaces the message */
    /* +0x008 */ u16 item;           /* the item that did not fit */
    /* +0x00A */ u16 keys;           /* this frame's menu keys */
    /* +0x00C */ _PLW* plw;          /* the player getting the item */
    /* +0x010 */ IdValue* pouch;     /* the pouch's entries */
    /* +0x014 */ IdValue* box;       /* the item box page's entries */
    /* +0x018 */ MenuEntry rows[8];  /* the shown page's rows */
    /* +0x0D8 */ MenuEntry choice[2]; /* the yes/no choice */
    /* +0x108 */ u8 unused_0x108[0x18];
    /* +0x120 */ CockpitMsgList list;
    /* +0x128 */ s32 choice_index;   /* the marked choice (or the yes/no toggle) */
    /* +0x12C */ u8 game_mode;       /* `GameMode_ck` when the item came: 1 quest, 2 lobby */
    /* +0x12D */ u8 page2;           /* the player has the second pouch page (the box page follows entry 24) */
    /* +0x12E */ u8 fade;            /* the dialog's fade step (0..3) */
    /* +0x12F */ u8 pad_0x12F;
} CockpitMsgSlot;

/* The lobby world block as the item dialog reads it: the item box page.  size: 0x180 (approximate) */
typedef struct HudLobbyItems {
    /* +0x000 */ u8 unused_0x000[0x160];
    /* +0x160 */ IdValue box[8];
} HudLobbyItems;

/* The lobby world block as the monster marks read it: one bit per monster menu row.
 * size: 0x39C2 (approximate: the bitmap's length is not read) */
typedef struct HudLobbyMarks {
    /* +0x0000 */ u8 unused_0x0000[0x39B2];
    /* +0x39B2 */ u8 monster_marks[16];
} HudLobbyMarks;

/* A hit sphere: its centre and radius.  size: 0x10 */
typedef struct HudSphere {
    /* +0x00 */ VEC3 center;
    /* +0x0C */ f32 radius;
} HudSphere;

/* A world ray: its origin and normalised direction.  size: 0x18 */
typedef struct HudRay {
    /* +0x00 */ VEC3 origin;
    /* +0x0C */ VEC3 dir;
} HudRay;

/* The camera record the scene root's current camera handle reads as (the fields the pointer ray uses).
 * size: 0xB0 (approximate: the last field read + 4) */
typedef struct HudCamera {
    /* +0x00 */ u8 unused_0x00[0x74];
    /* +0x74 */ VEC3 pos;
    /* +0x80 */ VEC3 up;
    /* +0x8C */ VEC3 target;
    /* +0x98 */ u8 unused_0x98[0xAC - 0x98];
    /* +0xAC */ f32 fovy;        /* the vertical field of view, in degrees */
} HudCamera;

/* The pointer page's current pick: what the cursor is over and where its label goes.  size: 0xA */
typedef struct CockpitPointerPick {
    /* +0x00 */ u8 valid;
    /* +0x01 */ u8 kind;        /* 0 an item row, 1..3 a monster-list kind, 4 a monster */
    /* +0x02 */ u16 id;         /* the row's item/equipment id, or the monster */
    /* +0x04 */ _mh_ivec2_ pos; /* where the label is drawn */
    /* +0x08 */ u8 index;       /* the place-list entry under the cursor (0xFF none) */
    /* +0x09 */ u8 pad_0x09;
} CockpitPointerPick;

/* The pointer page (the remote-pointer pick overlay the cockpit opens while its button is held).  size: 0x4C */
typedef struct CockpitPointerPage {
    /* +0x00 */ u8 open;          /* the page may run this frame (`ainpc_page_open_ck`) */
    /* +0x01 */ u8 step;          /* 0 closed, 1 browsing, 2 over a target, 3 holding the button, 4 confirming */
    /* +0x02 */ u8 pad_0x02[2];
    /* +0x04 */ _mh_ivec2_ cursor; /* the cursor's screen position */
    /* +0x08 */ _mh_ivec2_ trail[4]; /* the cursor's last positions, newest first (the trail sprites) */
    /* +0x18 */ u8 pick_mode;     /* 1 when the item list asked for the page (place-list picks), 0 for world picks */
    /* +0x19 */ u8 target_hit;    /* the cursor is over a monster */
    /* +0x1A */ u8 in_box;        /* the cursor is inside the confirm box */
    /* +0x1B */ u8 confirm_clock; /* the confirm box's blink clock */
    /* +0x1C */ u8 mark_frame;    /* the "marked" animation's frame (0 idle) */
    /* +0x1D */ u8 frame_1127;    /* the frame of layout animation 1127 */
    /* +0x1E */ u8 frame_1125;    /* the frame of layout animation 1125 (the page's fade) */
    /* +0x1F */ u8 fade_in;       /* frames since the page opened (to 23) */
    /* +0x20 */ u8 frame_1120;    /* the frame of layout animation 1120 */
    /* +0x21 */ u8 grow;          /* the cursor's grow step (to 5) */
    /* +0x22 */ u8 cycle;         /* a 0..15 cycle */
    /* +0x23 */ u8 pointer_valid; /* the remote's pointer is on screen */
    /* +0x24 */ u8 dpd_wait;      /* frames to wait for the pointer after opening */
    /* +0x25 */ u8 hold;          /* another screen holds the page open (`ainpc_page_hold_set`) */
    /* +0x26 */ u8 active;        /* the page ran this frame */
    /* +0x27 */ u8 selecting;     /* a pick is being confirmed */
    /* +0x28 */ CockpitPointerPick sel;
    /* +0x32 */ _mh_ivec2_ anchor; /* the guide's anchor offset (`ainpc_page_pos_set`) */
    /* +0x36 */ s16 pad_speed;     /* the d-pad cursor's acceleration (-16 at rest) */
    /* +0x38 */ u8 open_latch;    /* the open button has been seen since the page opened */
    /* +0x39 */ u8 pad_0x39[3];
    /* +0x3C */ PlaceRec row;     /* the help record of the picked place-list entry */
} CockpitPointerPage;

extern "C" {

void cockpit_blink_update(void);
f32 hud_screen_width_get(void);
void draw_sprite_pair_blink(u16 id_a, u16 mask_a, u16 id_b, u16 mask_b, u16 flags, const _mh_ivec2_* pos);
void draw_sprite_blink_idx(u16 id_a, u16 id_b, u32 color, u16 flags, const _mh_ivec2_* pos);
void put_page_count(u16* ids, u32 page, u32 count, u16 flags, const _mh_ivec2_* pos);
void ainpc_page_reset(void);
void ainpc_page_idle(void);
void ainpc_page_update(void);
s32 ainpc_page_open_ck(void);
u32 ainpc_page_dpd_ready_ck(void);
u32 ainpc_page_target_pick(void);
u32 hud_ray_sphere_hit_ck(HudSphere* sphere, HudRay* ray, f32* dist);
void hud_pointer_ray_get(VEC3* origin, VEC3* dir, _mh_ivec2_* screen);
u32 ainpc_page_monster_pick(void);
HudRay* hud_ray_init(HudRay* ray);
u8 ainpc_page_monster_count(void);
u8 ainpc_page_pick_kind_get(CockpitPointerPage* st);
_mh_ivec2_ ainpc_page_label_pos_get(s8 kind, _mh_ivec2_ cursor);
s32 hud_edge_dir_get(_mh_ivec2_* out, const _mh_ivec2_* from, const _mh_ivec2_* to);
void hud_pick_mark_draw(u8 state, u8 frame);
void ainpc_page_label_text_draw(CockpitPointerPage* st, s8 kind, u16 id, _mh_ivec2_ pos);
void hud_trail_sprite_draw(CockpitPointerPage* st, u16 id, u8 frame, u32 color);
void hud_cursor_sprite_draw(CockpitPointerPage* st, u16 id, u32 color);
void put_button_guide_r(u8 guide, _mh_ivec2_* pos);
s16 item_get_with_msg(_PLW* plw, u16 item, u16 ticket, u8 flags, s16 count);
s16 cockpit_ticket_acquire(_PLW* plw, u16* ticket);
s8* str_tbl_34_get(u8 id);
void cockpit_notice_quest_show(void);
void cockpit_notice_note_show(void);
void cockpit_notice_show(u8 kind, u32 fresh);
void cockpit_notice_quest_clear(void);
void cockpit_notice_lobby_clear(void);
void cockpit_notice_clear(u8 kind);
void cockpit_notice_init(void);
void cockpit_notice_update(void);
void cockpit_notice_icon_draw(u8 kind, s16 frame);
void cockpit_notice_lobby_draw(u8 kind, s16 frame);
void cockpit_notice_quest_draw(s16 frame);
s32 draw_lsp_element(_PLW* plw, u8 icon, u8 arrow, u16 rank);
void chara_icon_pos_get(_PLW* plw, u8 kind, VEC3* out);
void cockpit_msg_slot_keys_get(CockpitMsgSlot* slot, u8 player);
s32 cockpit_msg_slot_box_ck(CockpitMsgSlot* slot, s16 index);
IdValue* cockpit_msg_slot_entry_get(CockpitMsgSlot* slot, s16 index);
s32 cockpit_msg_slot_page_usable_ck(CockpitMsgSlot* slot);
void cockpit_msg_slot_rows_build(CockpitMsgSlot* slot);
void cockpit_msg_slot_choice_a_build(CockpitMsgSlot* slot, s16 choice);
void cockpit_msg_slot_choice_b_build(CockpitMsgSlot* slot, s16 choice);
u32 ainpc_page_cursor_in_box_ck(_mh_ivec2_* cursor);
u8 ainpc_page_sel_state_get(CockpitPointerPick* sel, PlaceRec* row);
void ainpc_page_buttons_get(u16* trigger, u16* hold);
u32 ainpc_page_open_button_ck(u16 trigger);
u32 ainpc_page_select_button_ck(CockpitPointerPage* st, u16 trigger, u16 hold);
void ainpc_page_cursor_pad_move(CockpitPointerPage* st, u16 trigger);
void ainpc_page_cursor_pointer_move(CockpitPointerPage* st);
void ainpc_page_cursor_draw(void);
void ainpc_page_label_draw(void);
void ainpc_page_guide_draw(void);
void monster_mark_set(u8 monster);
s32 monster_mark_ck(u8 monster);
void draw_digit_rect(HudPos3 pos, HudSize2 size, u8 digit, u32 color);
void hud_notice_init(HudNotice* notice, u8 kind, u8 id, s16 x, s16 y, u8 visible, u8 animating, u8 frame);
void hud_notice_visible_set(HudNotice* notice, bool visible);
void hud_notice_anim_set(HudNotice* notice, bool animating);
void hud_notice_pos_set(HudNotice* notice, s16 x, s16 y);
void hud_notice_pos_get(HudNotice* notice, s16* x, s16* y);
void hud_notice_kill(HudNotice* notice);
s32 hud_notice_exists_ck(u8 kind, u8 id);
u8 hud_notice_id_get(HudNotice* notice);
u8 ainpc_entry_kind_get(HudNotice* notice);
s32 ainpc_entry_live_ck(HudNotice* notice);
s32 hud_notice_flag_sync(HudNotice* notice);
u8* hud_notice_flag_ptr_get(HudNotice* notice);
void hud_notice_anim_update(HudNotice* notice);
u8 hud_notice_frame_get(HudNotice* notice);
u8 hud_notice_frame_step(HudNotice* notice, u8 step);
s32 hud_notice_anim_ck(HudNotice* notice);
void hud_notice_state_next(HudNotice* notice);
u8 hud_notice_state_get(HudNotice* notice);
void hud_notice_anim_draw(HudNotice* notice);
s32 hud_notice_visible_ck(HudNotice* notice);
void hud_notice_loop_update(HudNotice* notice);
void hud_notice_frame_set(HudNotice* notice, u8 frame);
u8 hud_notice_frame_wrap(HudNotice* notice, u8 last);
HudNoticeWork* hud_notice_work_get(HudNotice* notice);
void hud_notice_loop_init(HudNotice* notice);
void hud_notice_loop_draw(HudNotice* notice);
void hud_notice_scroll_update(HudNotice* notice);
void hud_notice_scroll_draw(HudNotice* notice);

/* The HUD colour of each player slot (0x805D54A0). */
u32 player_color_tbl[4] = {
    0xF22515FF, 0x3DD7D2FF, 0xE6E60AFF, 0x0AE80AFF,
};

/* The per-kind update and draw steps of the notice pool (0x805D54B0, 0x805D54BC). */
HudNoticeFunc hud_notice_update_tbl[3] = {
    hud_notice_anim_update, hud_notice_loop_update, hud_notice_scroll_update,
};
HudNoticeFunc hud_notice_draw_tbl[3] = {
    hud_notice_anim_draw, hud_notice_loop_draw, hud_notice_scroll_draw,
};

/* The sprite id runs of the kind-0 notices (0x807925A0..0x807925E0). */
u16 hud_notice_ids_0[] = { 0x0BE8, 0xFFFF };
u16 hud_notice_ids_1[] = { 0x0BB4, 0xFFFF };
u16 hud_notice_ids_2[] = { 0x03D5, 0x03D6, 0x03D7, 0xFFFF };
u16 hud_notice_ids_3[] = { 0x03D8, 0x03D9, 0x03DA, 0xFFFF };
u16 hud_notice_ids_4[] = { 0x03DC, 0x03DD, 0xFFFF };
u16 hud_notice_ids_5[] = { 0x03DE, 0x03DF, 0xFFFF };
u16 hud_notice_ids_6[] = { 0x0E54, 0x0E55, 0xFFFF };
u16 hud_notice_ids_7[] = { 0x1B5F, 0x1B60, 0xFFFF };
u16 hud_notice_ids_8[] = { 0x1B61, 0x1B62, 0xFFFF };
u16 hud_notice_ids_9[] = { 0x0CFF, 0xFFFF };

/* The kind-0 notice rows (0x805D54C8). */
HudNoticeAnimRow hud_notice_anim_tbl[10] = {
    { hud_notice_ids_0, 1, 1 },
    { hud_notice_ids_1, 1, 1 },
    { hud_notice_ids_2, 0, 1 },
    { hud_notice_ids_3, 0, 1 },
    { hud_notice_ids_4, 0, 1 },
    { hud_notice_ids_5, 0, 1 },
    { hud_notice_ids_6, 1, 1 },
    { hud_notice_ids_7, 0, 1 },
    { hud_notice_ids_8, 0, 1 },
    { hud_notice_ids_9, 1, 1 },
};

/* The kind-2 notices' front and back sprite runs (0x805D5518.., 0x807925E8..0x80792618). */
u16 hud_notice_scroll_back_1[] = {
    0x0ED8, 0x0ECC, 0x0ECD, 0x0ECE, 0x0ECF, 0x0ED0, 0x0ED1, 0x0ED2, 0x0ED3, 0x0ED4, 0x0ED5, 0x0ED6, 0x0ED7, 0xFFFF,
};
u16 hud_notice_scroll_back_wide_1[] = {
    0x0ED8, 0x0EDA, 0x0ED9, 0x0EDB, 0x0EDC, 0x0EDD, 0x0EDE, 0x0EDF, 0x0EE0, 0x0EE1, 0x0EE2, 0x0EE3, 0x0EE4, 0xFFFF,
};
u16 hud_notice_scroll_front_0[] = { 0x0ECB, 0x0ECA, 0x0EC9, 0xFFFF };
u16 hud_notice_scroll_front_1[] = { 0x0EEF, 0x0EEE, 0xFFFF };
u16 hud_notice_scroll_back_0[] = { 0x0EF4, 0x0EF0, 0x0EF1, 0xFFFF };
u16 hud_notice_scroll_back_wide_0[] = { 0x0EF4, 0x0EF2, 0x0EF3, 0xFFFF };
u16 hud_notice_scroll_front_2[] = { 0x0EE7, 0x0EE6, 0xFFFF };
u16 hud_notice_scroll_back_2[] = { 0x0EEC, 0x0EE8, 0x0EE9, 0xFFFF };
u16 hud_notice_scroll_back_wide_2[] = { 0x0EEC, 0x0EEA, 0x0EEB, 0xFFFF };

const u16* hud_notice_scroll_front_tbl[3] = {
    hud_notice_scroll_front_0, hud_notice_scroll_front_1, hud_notice_scroll_front_2,
};
const u16* hud_notice_scroll_back_tbl[3] = {
    hud_notice_scroll_back_1, hud_notice_scroll_back_0, hud_notice_scroll_back_2,
};
const u16* hud_notice_scroll_back_wide_tbl[3] = {
    hud_notice_scroll_back_wide_1, hud_notice_scroll_back_wide_0, hud_notice_scroll_back_wide_2,
};

/* The random texture offset range of each kind-2 notice (0x80792620). */
u16 hud_notice_scroll_ofs_range[3] = { 0x0B61, 0x0444, 0x0889 };

/* The notice pool (0x806BD808). */
HudNotice ainpc_entry_tbl[16];

/* The blink clock (0x80794B78). */
CockpitBlink cockpit_blink;

}

/* 0x802D9EA4 (0x4): runs the blink clock for this frame. */
void ai_npc_reaction_forward(void)
{
    cockpit_blink_update();
}

/* 0x802D9EA8 (0xC): returns the blink pulse (0..1). */
f32 cockpit_blink_get(void)
{
    return cockpit_blink.value;
}

/* 0x802D9EB4 (0xA4): steps the 40-frame clock and derives the pulse from a sine of it, clamped to 0..1. */
void cockpit_blink_update(void)
{
    CockpitBlink* blink = &cockpit_blink;
    f32 value;

    if (++cockpit_blink.clock >= 40) {
        blink->clock = 0;
    }
    value = 0.5f * (1.0f + math_sin_rad(0.15707964f * blink->clock));
    if (value < 0.0f) {
        value = 0.0f;
    }
    if (value > 1.0f) {
        value = 1.0f;
    }
    blink->value = value;
}

/* 0x802D9F58 (0x18): returns player `player`'s HUD colour. */
u32 player_color_get(u8 player)
{
    return player_color_tbl[player];
}

/* 0x802D9F70 (0x44): returns the horizontal HUD scale: 1 on a 4:3 screen, else the screen width over 640. */
f32 hud_screen_width_get(void)
{
    _MH_VEC2 size;

    if (ck_WideMode() == 0) {
        return 1.0f;
    }
    get_ScreenSize(&size);
    return 0.0015625f * size.x;
}

/* 0x802D9FB4 (0xB8): draws the atlas digit `digit` as a textured rectangle at `pos`, `size` wide, in `color`. */
void draw_digit_rect(HudPos3 pos, HudSize2 size, u8 digit, u32 color)
{
    _DRAW_RECT_2D_TEX rect;
    s16 at[2];
    s16 uv0[2];
    s16 uv1[2];

    at[0] = pos.x;
    at[1] = pos.y;
    atlas_digit_uv_get(uv0, uv1, digit);
    draw_rect_2d_tex_init(&rect, at, size.w, size.h, color, uv0, uv1);
    draw_rect_2d_tex_by_id(&rect, 0);
}

/* 0x802DA06C (0xD0): draws the atlas digit `digit` as a `w` x `h` rectangle at (`x`, `y`, `z`). */
void draw_frame_rect_z(s16 x, s16 y, s16 w, s16 h, u8 digit, u32 color, f32 z)
{
    HudPos3 pos(x, y, z);
    HudSize2 size(w, h);

    draw_digit_rect(pos, size, digit, color);
}

/* 0x802DA13C (0x34): fills a pool slot and marks it live. */
void hud_notice_init(HudNotice* notice, u8 kind, u8 id, s16 x, s16 y, u8 visible, u8 animating, u8 frame)
{
    notice->active = 1;
    notice->kind = kind;
    notice->id = id;
    notice->state = 0;
    notice->visible = visible;
    notice->animating = animating;
    notice->frame = frame;
    notice->flag_ptr = 0;
    notice->x = x;
    notice->y = y;
}

/* 0x802DA170 (0xC): shows or hides the notice. */
void hud_notice_visible_set(HudNotice* notice, bool visible)
{
    notice->visible = visible;
}

/* 0x802DA17C (0xC): starts or stops the notice's animation. */
void hud_notice_anim_set(HudNotice* notice, bool animating)
{
    notice->animating = animating;
}

/* 0x802DA188 (0xC): moves the notice. */
void hud_notice_pos_set(HudNotice* notice, s16 x, s16 y)
{
    notice->x = x;
    notice->y = y;
}

/* 0x802DA194 (0x14): reads the notice's position. */
void hud_notice_pos_get(HudNotice* notice, s16* x, s16* y)
{
    *x = notice->x;
    *y = notice->y;
}

/* 0x802DA1A8 (0x8): makes the notice follow the state byte `flag`. */
void hud_notice_set_flag_ptr(HudNotice* notice, u8* flag)
{
    notice->flag_ptr = flag;
}

/* 0x802DA1B0 (0x50): frees every slot of the pool. */
void ai_slots_clear(void)
{
    HudNotice* notice = ainpc_entry_tbl;
    s32 i;

    for (i = 16; i > 0; i--, notice++) {
        hud_notice_kill(notice);
    }
}

/* 0x802DA200 (0xC): frees the slot. */
void hud_notice_kill(HudNotice* notice)
{
    notice->active = 0;
}

/* 0x802DA20C (0xA4): whether a live notice of `kind` and `id` exists. */
s32 hud_notice_exists_ck(u8 kind, u8 id)
{
    HudNotice* notice = ainpc_entry_tbl;
    s32 i;

    for (i = 16; i > 0; i--, notice++) {
        if (ainpc_entry_live_ck(notice) && kind == ainpc_entry_kind_get(notice) && id == hud_notice_id_get(notice)) {
            return 1;
        }
    }
    return 0;
}

/* 0x802DA2B0 (0x8): returns the notice's id. */
u8 hud_notice_id_get(HudNotice* notice)
{
    return notice->id;
}

/* 0x802DA2B8 (0x8): returns the notice's kind. */
u8 ainpc_entry_kind_get(HudNotice* notice)
{
    return notice->kind;
}

/* 0x802DA2C0 (0x14): whether the slot is live. */
s32 ainpc_entry_live_ck(HudNotice* notice)
{
    return notice->active != 0;
}

/* 0x802DA2D4 (0x70): frees every notice of kind `kind`. */
void ainpc_entry_kill_kind(s32 kind)
{
    u8 want;
    HudNotice* notice = ainpc_entry_tbl;
    s32 i = 16;

    want = kind;
    for (; i > 0; i--, notice++) {
        if (want == ainpc_entry_kind_get(notice)) {
            hud_notice_kill(notice);
        }
    }
}

/* 0x802DA344 (0x88): runs the update step of every live notice. */
void ainpc_entry_update_all(void)
{
    HudNotice* notice = ainpc_entry_tbl;
    s32 i;

    for (i = 16; i > 0; i--, notice++) {
        if (ainpc_entry_live_ck(notice)) {
            hud_notice_update_tbl[ainpc_entry_kind_get(notice)](notice);
        }
    }
}

/* 0x802DA3CC (0x88): runs the draw step of every live notice. */
void ainpc_entry_draw_all(void)
{
    HudNotice* notice = ainpc_entry_tbl;
    s32 i;

    for (i = 16; i > 0; i--, notice++) {
        if (ainpc_entry_live_ck(notice)) {
            hud_notice_draw_tbl[ainpc_entry_kind_get(notice)](notice);
        }
    }
}

/* 0x802DA454 (0xA4): claims a free slot for a notice; returns it, or 0 when the pool is full. */
HudNotice* hud_notice_spawn(s32 kind, s32 id, s16 x, s16 y, s32 visible, s32 animating, s32 frame)
{
    HudNotice* notice = ainpc_entry_tbl;
    s32 i;

    for (i = 16; i > 0; i--, notice++) {
        if (!ainpc_entry_live_ck(notice)) {
            hud_notice_init(notice, kind, id, x, y, visible, animating, frame);
            return notice;
        }
    }
    return 0;
}

/* 0x802DA4F8 (0x88): follows the watched state byte: bit 0 ends the notice (returns 0), bit 1 stops the
 * animation and bit 2 hides it. */
s32 hud_notice_flag_sync(HudNotice* notice)
{
    u8* flag = hud_notice_flag_ptr_get(notice);

    if (flag != 0) {
        if (*flag & 1) {
            return 0;
        }
        hud_notice_anim_set(notice, (*flag & 2) == 0);
        hud_notice_visible_set(notice, (*flag & 4) == 0);
    }
    return 1;
}

/* 0x802DA580 (0x8): returns the watched state byte. */
u8* hud_notice_flag_ptr_get(HudNotice* notice)
{
    return notice->flag_ptr;
}

/* 0x802DA588 (0x180): the kind-0 step: follows the state byte, hides notice 0 while the item menu is busy and
 * plays the row's sprite run (held at its end, or ended for a one-shot row). */
void hud_notice_anim_update(HudNotice* notice)
{
    switch (hud_notice_state_get(notice)) {
    case 0:
        if (hud_notice_flag_sync(notice) == 0) {
            hud_notice_kill(notice);
        } else {
            hud_notice_state_next(notice);
        }
        break;
    case 1:
        if (hud_notice_flag_sync(notice) == 0) {
            hud_notice_kill(notice);
            break;
        }
        if (hud_notice_id_get(notice) == 0) {
            MenuFrameWork* frame = (MenuFrameWork*)cockpit_work[0].plw;
            if (menu_item_frame_update(frame) == 1 || frame->field_0x5BC != 0 || frame->field_0x5BE != 0 ||
                frame->field_0x5BD != 0) {
                hud_notice_visible_set(notice, 0);
            } else {
                hud_notice_visible_set(notice, 1);
            }
        }
        if (hud_notice_anim_ck(notice)) {
            s32 last;
            HudNoticeAnimRow* row = &hud_notice_anim_tbl[hud_notice_id_get(notice)];

            hud_notice_frame_step(notice, 1);
            last = sprite_ary_last_frame(row->ids);
            if (hud_notice_frame_get(notice) > last) {
                if (row->one_shot == 0) {
                    hud_notice_anim_set(notice, 0);
                } else {
                    hud_notice_kill(notice);
                }
            }
        }
        break;
    }
}

/* 0x802DA708 (0x8): returns the notice's frame. */
u8 hud_notice_frame_get(HudNotice* notice)
{
    return notice->frame;
}

/* 0x802DA710 (0x20): advances the frame by `step` while the notice animates; returns the frame. */
u8 hud_notice_frame_step(HudNotice* notice, u8 step)
{
    if (notice->animating != 0) {
        notice->frame += step;
    }
    return notice->frame;
}

/* 0x802DA730 (0x14): whether the notice animates. */
s32 hud_notice_anim_ck(HudNotice* notice)
{
    return notice->animating != 0;
}

/* 0x802DA744 (0x10): moves the notice to its next step. */
void hud_notice_state_next(HudNotice* notice)
{
    notice->state++;
}

/* 0x802DA754 (0x8): returns the notice's step. */
u8 hud_notice_state_get(HudNotice* notice)
{
    return notice->state;
}

/* 0x802DA75C (0xB0): the kind-0 draw: the row's sprite run at the notice's frame. */
void hud_notice_anim_draw(HudNotice* notice)
{
    HudNoticeAnimRow* row;
    _mh_ivec2_ pos;

    if (hud_notice_visible_ck(notice)) {
        row = &hud_notice_anim_tbl[hud_notice_id_get(notice)];
        if (row->additive != 0) {
            set_blendmode(4, 1, 1);
        } else {
            set_blendmode(4, 5, 1);
        }
        hud_notice_pos_get(notice, &pos.x, &pos.y);
        draw_sprite_anim_ary(row->ids, hud_notice_frame_get(notice), &pos);
    }
}

/* 0x802DA80C (0x14): whether the notice is shown. */
s32 hud_notice_visible_ck(HudNotice* notice)
{
    return notice->visible != 0;
}

/* 0x802DA820 (0xB8): the kind-1 step: sets the loop up, then wraps its frame (notice 33 never drops below 100). */
void hud_notice_loop_update(HudNotice* notice)
{
    HudNoticeWork* work;

    switch (hud_notice_state_get(notice)) {
    case 0:
        hud_notice_loop_init(notice);
        hud_notice_state_next(notice);
        break;
    case 1:
        work = hud_notice_work_get(notice);
        hud_notice_frame_step(notice, 1);
        hud_notice_frame_wrap(notice, work->loop.last_frame);
        if (hud_notice_id_get(notice) == 33 && hud_notice_frame_get(notice) < 100) {
            hud_notice_frame_set(notice, 100);
        }
        break;
    }
}

/* 0x802DA8D8 (0x8): sets the notice's frame. */
void hud_notice_frame_set(HudNotice* notice, u8 frame)
{
    notice->frame = frame;
}

/* 0x802DA8E0 (0x28): wraps the frame into 0..`last`; returns it. */
u8 hud_notice_frame_wrap(HudNotice* notice, u8 last)
{
    u8 frame = notice->frame % (last + 1);
    notice->frame = frame;
    return frame;
}

/* 0x802DA908 (0x8): returns the notice's kind-specific work. */
HudNoticeWork* hud_notice_work_get(HudNotice* notice)
{
    return &notice->work;
}

/* 0x802DA910 (0xD4): sets a kind-1 notice up from the menu layout table row 406 (its run and anchor). */
void hud_notice_loop_init(HudNotice* notice)
{
    HudNoticeWork* work = hud_notice_work_get(notice);
    const u16** rows = (const u16**)get_menu_lsp_tbl(406);
    const u16* row = rows[hud_notice_id_get(notice)];
    _mh_ivec2_ pos;

    work->loop.ids = row + 1;
    if (hud_notice_id_get(notice) == 33) {
        hud_notice_frame_set(notice, 100);
    }
    if (hud_notice_id_get(notice) == 18) {
        work->loop.last_frame = 99;
    } else {
        work->loop.last_frame = sprite_ary_last_frame(work->loop.ids);
    }
    get_lsp_data(*row, &pos);
    hud_notice_pos_set(notice, pos.x, pos.y);
}

/* 0x802DA9E4 (0x70): the kind-1 draw: the loop's run at the notice's frame. */
void hud_notice_loop_draw(HudNotice* notice)
{
    HudNoticeWork* work;
    _mh_ivec2_ pos;

    if (hud_notice_visible_ck(notice)) {
        work = hud_notice_work_get(notice);
        hud_notice_pos_get(notice, &pos.x, &pos.y);
        draw_sprite_anim_ary(work->loop.ids, hud_notice_frame_get(notice), &pos);
    }
}

/* 0x802DAA54 (0x1B8): the kind-2 step: picks the runs and a random texture offset, fades in for 30 frames,
 * holds 90 and fades out over 40. */
void hud_notice_scroll_update(HudNotice* notice)
{
    HudNoticeWork* work = hud_notice_work_get(notice);
    u32 rnd;
    u32 ofs;

    switch (hud_notice_state_get(notice)) {
    case 0:
        work->scroll.front_ids = hud_notice_scroll_front_tbl[hud_notice_id_get(notice)];
        if (ck_WideMode() == 0) {
            work->scroll.back_ids = hud_notice_scroll_back_tbl[hud_notice_id_get(notice)];
        } else {
            work->scroll.back_ids = hud_notice_scroll_back_wide_tbl[hud_notice_id_get(notice)];
        }
        rnd = ran_suu(1);
        ofs = (rnd * hud_notice_scroll_ofs_range[hud_notice_id_get(notice)] * 2) >> 16;
        work->scroll.tex_ofs = (s16)ofs - hud_notice_scroll_ofs_range[hud_notice_id_get(notice)];
        hud_notice_state_next(notice);
        break;
    case 1:
        if (hud_notice_frame_step(notice, 1) >= 30) {
            hud_notice_state_next(notice);
            work->scroll.hold_timer = 90;
        }
        break;
    case 2:
        if (--work->scroll.hold_timer <= 0) {
            hud_notice_state_next(notice);
        }
        break;
    case 3:
        if (hud_notice_frame_get(notice) < 40) {
            hud_notice_frame_step(notice, 1);
        }
        break;
    }
}

/* 0x802DAC0C (0xD8): the kind-2 draw: the back run, then each front sprite shifted by the texture offset. */
void hud_notice_scroll_draw(HudNotice* notice)
{
    HudNoticeWork* work;
    const u16* id;
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;

    if (hud_notice_visible_ck(notice)) {
        set_blendmode(4, 5, 1);
        work = hud_notice_work_get(notice);
        hud_notice_pos_get(notice, &pos.x, &pos.y);
        draw_sprite_anim_ary(work->scroll.back_ids, hud_notice_frame_get(notice), &pos);
        for (id = work->scroll.front_ids; *id != 0xFFFF; id++) {
            sprite_frame_apply(&spr, *id, hud_notice_frame_get(notice), 0);
            spr.tex_id += work->scroll.tex_ofs;
            draw_sprite(spr, &pos);
        }
    }
}

/* 0x802DACE4 (0x140): draws two blinking layout sprites (an id of 0xFFFF is skipped); a sprite whose mask is set in
 * `flags` plays its animation frame 1 instead. */
void draw_sprite_pair_blink(u16 id_a, u16 mask_a, u16 id_b, u16 mask_b, u16 flags, const _mh_ivec2_* pos)
{
    _SPR_DATA_ spr;
    f32 scale;

    if (chk_pointer() == 0) {
        scale = 1.0f + 0.3f * cockpit_blink_get();
        if (id_a != 0xFFFF) {
            if (flags & mask_a) {
                draw_sprite_anim_idx(id_a, 1, pos);
            } else {
                spr_data_copy(&spr, get_lsp_data(id_a, 0));
                spr.color = color_scale(spr.color, scale, 1.0f);
                draw_sprite(spr, pos);
            }
        }
        if (id_b != 0xFFFF) {
            if (flags & mask_b) {
                draw_sprite_anim_idx(id_b, 1, pos);
            } else {
                spr_data_copy(&spr, get_lsp_data(id_b, 0));
                spr.color = color_scale(spr.color, scale, 1.0f);
                draw_sprite(spr, pos);
            }
        }
    }
}

/* 0x802DAE24 (0x124): draws two blinking layout sprites; flag 4 (first) / 8 (second) draws it in `color` instead. */
void draw_sprite_blink_idx(u16 id_a, u16 id_b, u32 color, u16 flags, const _mh_ivec2_* pos)
{
    _SPR_DATA_ spr;
    f32 scale;

    if (chk_pointer() == 0) {
        scale = 1.0f + 0.3f * cockpit_blink_get();
        if (id_a != 0xFFFF) {
            spr_data_copy(&spr, get_lsp_data(id_a, 0));
            if (flags & 4) {
                spr.color = color;
            } else {
                spr.color = color_scale(spr.color, scale, 1.0f);
            }
            draw_sprite(spr, pos);
        }
        if (id_b != 0xFFFF) {
            spr_data_copy(&spr, get_lsp_data(id_b, 0));
            if (flags & 8) {
                spr.color = color;
            } else {
                spr.color = color_scale(spr.color, scale, 1.0f);
            }
            draw_sprite(spr, pos);
        }
    }
}

/* 0x802DAF48 (0x1F8): the page arrows and the "page / count" digits of a paged list (`table`: the frame, the two
 * arrows, the count and page digit sprites); `mode` bit 1 hides the arrow at either end, bit 0 uses two-digit
 * numbers and bit 7 hides the arrows. */
void PutPageArrow(u16* table, s16 page, s16 count, u16 flags, const _mh_ivec2_* pos, u8 mode)
{
    u32 color = 0;
    _SPR_DATA_ spr;
    u16 ids[3];
    u16 prev;
    u16 next;

    if ((flags & 0xC) && count > 1 && chk_pointer() == 0) {
        sprite_frame_apply(&spr, 22, 1, 0);
        color = spr.color;
    }
    draw_sprite_idx(table[0], pos);
    if (!(mode & 0x80)) {
        if (!(mode & 2)) {
            if (count > 1) {
                sprite_frame_apply(&spr, 19, 1, 0);
                draw_sprite_blink_idx(table[1], table[2], spr.color, flags, pos);
            }
        } else if (count > 1) {
            prev = table[1];
            next = table[2];
            if (page == 0) {
                prev = 0xFFFF;
            }
            if (page == count - 1) {
                next = 0xFFFF;
            }
            sprite_frame_apply(&spr, 19, 1, 0);
            draw_sprite_blink_idx(prev, next, spr.color, flags, pos);
        }
    }
    if (!(mode & 1)) {
        draw_number_idx(table[4], page + 1, color, pos);
        draw_number_idx(table[3], count, 0, pos);
    } else {
        ids[0] = table[3];
        ids[1] = table[5];
        ids[2] = table[6];
        draw_number_2digit_ary(ids, count, 0, pos);
        ids[0] = table[4];
        ids[1] = table[7];
        ids[2] = table[8];
        draw_number_2digit_ary(ids, page + 1, color, pos);
    }
}

/* The frame sprites of the page counter (0x80792628). */
u16 page_count_frame_ids[] = { 0x006F, 0x0070, 0x0071, 0xFFFF };

/* 0x802DB140 (0x114): the "page N" counter of a paged list: its frame, the arrows (left while `page` < `count`, right
 * past page 1) and the page number in one or two digits. */
void put_page_count(u16* ids, u32 page, u32 count, u16 flags, const _mh_ivec2_* pos)
{
    _mh_ivec2_ at;
    char text[8];
    u16 left;
    u16 right;
    u16 font_id;
    u16 part;

    draw_sprite_ary(ids + 4, pos);
    at.x = pos->x + 9;
    at.y = pos->y + 17;
    draw_sprite_ary(page_count_frame_ids, &at);
    left = page < count ? ids[0] : 0xFFFF;
    right = page > 1 ? ids[1] : 0xFFFF;
    draw_sprite_pair_blink(left, 1, right, 2, flags, pos);
    sprintf(text, "%d", page);
    part = (flags & 0xF) != 0;
    font_id = page < 10 ? ids[2] : ids[3];
    draw_font_anim_idx(font_id, part, (s8*)text, 1, pos);
}

/* The colour of each equipment rarity (0x805D5574). */
u32 rare_color_tbl[7] = {
    0xF0F0F0FF, 0xB092F2FF, 0xDED55EFF, 0xE88E9EFF, 0x6FC773FF, 0x708EF8FF, 0xDA565AFF,
};

/* 0x802DB254 (0x18): returns the colour of rarity `index`. */
u32 get_rare_color(u8 index)
{
    return rare_color_tbl[index];
}

/* The pointer page (0x806BDA08). */
CockpitPointerPage ainpc_page_state;

/* 0x802DB26C (0x4C): clears the pointer page state. */
void ainpc_page_state_init(void)
{
    memset(&ainpc_page_state, 0, sizeof(CockpitPointerPage));
    ainpc_page_state.dpd_wait = 1;
    ainpc_page_state.sel.index = 0xFF;
}

/* 0x802DB2B8 (0x34): releases the page hold and clears its anchor. */
void ainpc_page_reset(void)
{
    ainpc_page_hold_clear();
    ainpc_page_state.anchor.x = 0;
    ainpc_page_state.anchor.y = 0;
}

/* 0x802DB2EC (0x4): does nothing. */
void ainpc_page_idle(void)
{
}

/* 0x802DB2F0 (0x618): the pointer page's frame: opens it while the remote's button is held, moves the cursor,
 * runs the pick/confirm steps and closes it again (with the close sound). */
void ainpc_page_update(void)
{
    CockpitPointerPage* st = &ainpc_page_state;
    u8 prev_step = st->step;
    u16 trigger;
    u16 hold;
    s32 open;

    ainpc_page_state.open = 0;
    st->active = 0;
    ainpc_page_open_ck();
    kpad_pointer_read();
    if (ainpc_page_state.open == 1) {
        subTransSetPrio(6, (u32)ainpc_page_guide_draw, 0, 0);
        subTransSetPrio(7, (u32)ainpc_page_label_draw, 0, 0);
        subTransSetPrio(10, (u32)ainpc_page_cursor_draw, 0, 0);
        ainpc_page_buttons_get(&trigger, &hold);
        open = 0;
        if (st->open_latch == 0) {
            if (ainpc_page_open_button_ck(trigger) == 1) {
                open = 1;
                pointer_reset();
                se_req_page_toggle();
                st->dpd_wait = 1;
                st->open_latch = 1;
            }
        } else if (ainpc_page_open_button_ck(trigger) == 1) {
            open = 1;
        }
        if (st->fade_in != 0 && st->fade_in < 23 && st->step == 1) {
            open = 1;
        }
        if (open == 0) {
            st->open_latch = 0;
        }
        if (open != 0) {
            if (get_option_cfg(4) == 0) {
                ainpc_page_cursor_pointer_move(st);
            } else {
                ainpc_page_cursor_pad_move(st, trigger);
            }
            if (ainpc_page_dpd_ready_ck() == 1) {
                st->active = 1;
                if (st->fade_in < 23) {
                    st->fade_in++;
                }
                if (st->grow < 5) {
                    st->grow++;
                }
                if (sprite_anim_last_frame(1125) > st->frame_1125) {
                    st->frame_1125++;
                }
                st->frame_1120++;
                if (st->frame_1120 > sprite_anim_last_frame(1120)) {
                    st->frame_1120 = 0;
                }
                st->frame_1127++;
                if (st->frame_1127 > sprite_anim_last_frame(1127)) {
                    st->frame_1127 = 0;
                }
                if (++st->cycle > 15) {
                    st->cycle = 0;
                }
                if (st->selecting == 0 && st->mark_frame != 0) {
                    st->mark_frame++;
                    if (st->mark_frame > sprite_ary_last_frame(((const u16**)get_menu_lsp_tbl(96))[3])) {
                        st->mark_frame = 0;
                    }
                }
                switch (st->step) {
                case 0:
                    st->step = 1;
                    st->grow = 0;
                    st->frame_1120 = 0;
                    st->fade_in = 0;
                    st->sel.valid = 0;
                    st->sel.index = 0xFF;
                    break;
                case 1:
                    if (st->fade_in >= 23 && ainpc_page_target_pick() == 1) {
                        st->step = 2;
                        if (st->target_hit == 1) {
                            st->step = 2;
                            st->mark_frame = 0;
                            st->frame_1120 = 0;
                            st->grow = 0;
                        }
                    }
                    break;
                case 2:
                    if (ainpc_page_target_pick() == 0) {
                        st->step = 1;
                        st->sel.valid = 0;
                        st->sel.index = 0xFF;
                    } else {
                        if (st->frame_1120 == 7) {
                            sysSE_req(21);
                        }
                        if (ainpc_page_select_button_ck(st, trigger, hold) == 1) {
                            if (st->target_hit == 1) {
                                st->step = 4;
                                st->fade_in = 0;
                                switch (ainpc_page_sel_state_get(&st->sel, &st->row)) {
                                case 1:
                                case 2:
                                    sysSE_req(2);
                                    break;
                                case 0:
                                    sysSE_req(22);
                                    break;
                                }
                            } else {
                                st->step = 3;
                                st->fade_in = 0;
                                sysSE_req(22);
                            }
                        }
                    }
                    break;
                case 3:
                    if (ainpc_page_select_button_ck(st, trigger, hold) == 0) {
                        st->selecting = 0;
                        st->step = 1;
                        st->sel.valid = 0;
                        st->sel.index = 0xFF;
                        st->frame_1120 = 0;
                        st->grow = 0;
                        st->fade_in = 0;
                    }
                    break;
                case 4:
                    st->in_box = ainpc_page_cursor_in_box_ck(&st->cursor);
                    if (ainpc_page_select_button_ck(st, trigger, hold) == 1) {
                        if (st->in_box != 0) {
                            st->confirm_clock++;
                            if ((u8)(st->confirm_clock + 196) > 19) {
                                st->confirm_clock = 60;
                            }
                        } else {
                            st->confirm_clock = 30;
                        }
                    } else {
                        if (st->in_box != 0) {
                            if (st->sel.kind == 4) {
                                if (monster_mark_ck(st->sel.id) != 0) {
                                    sysSE_req(2);
                                } else {
                                    monster_mark_set(st->sel.id);
                                    hud_item_msg_push(1, 38, 0);
                                    st->mark_frame = 1;
                                    sysSE_req(23);
                                }
                            } else {
                                switch ((s8)menu_row_reserve((struct MenuRowData*)&st->row)) {
                                case 0:
                                    hud_item_msg_push(1, 45, 0);
                                    st->mark_frame = 1;
                                    sysSE_req(23);
                                    break;
                                case 1:
                                case 2:
                                    sysSE_req(2);
                                    break;
                                }
                            }
                            st->selecting = 0;
                        } else {
                            st->selecting = 0;
                        }
                        st->step = 1;
                        st->sel.valid = 0;
                        st->sel.index = 0xFF;
                        st->frame_1120 = 0;
                        st->grow = 0;
                        st->fade_in = 0;
                        se_req_page_toggle();
                    }
                    break;
                default:
                    st->active = 0;
                    break;
                }
            }
        }
    } else {
        st->open_latch = 0;
    }
    place_list_deactivate_all();
    if (st->active == 0) {
        if (prev_step != 0 || (st->step != 0 && st->open == 0)) {
            sysSE_req(1);
        }
        st->step = 0;
        st->target_hit = 0;
        st->selecting = 0;
        st->sel.valid = 0;
        st->sel.index = 0xFF;
        st->mark_frame = 0;
        if (st->frame_1125 != 0) {
            st->frame_1125--;
        }
    }
}

/* 0x802DB908 (0x218): whether the pointer page may open this frame (not while the pad is held, a demo runs, a menu
 * or a fade is up); marks the page open and records whether the item list asked for it. */
s32 ainpc_page_open_ck(void)
{
    CockpitPointerPage* st = &ainpc_page_state;
    SysQuestMove* move;

    if (pad_hold_ck() == 1 || system_w.field_0x05 == 1) {
        st->open = 0;
        return 0;
    }
    switch (GameMode_ck()) {
    default:
        return 0;
    case 1:
        move = (SysQuestMove*)get_move_work_adrs(0);
        if (move == 0) {
            return 0;
        }
        if (move->menu_lock_0x112 == 1) {
            return 0;
        }
        if (get_fade_stat(0) == 2) {
            return 0;
        }
        break;
    case 2:
        if (lobby_w.area_change_0x026 == 1) {
            return 0;
        }
        break;
    }
    if ((u8)event_demo_running_ck() == 1) {
        st->open = 0;
        return 0;
    }
    if (menu_work.slot[0].field_0x32D == 1) {
        st->hold = 0;
        st->open = 0;
        return 0;
    }
    switch (GameMode_ck()) {
    case 1:
        if (PlayMode_ck() == 2 && system_w.field_0x8af == 0) {
            st->hold = 0;
            st->open = 0;
            return 0;
        }
        if (menu_item_frame_update(0) != 0 && menu_item_list_ready_ck() != 0 && (s8)talk_msg_speaker_get() == -1 &&
            lb_quest_work_active_ck() == 0) {
            st->open = 1;
            st->pick_mode = 1;
            return 1;
        }
        if (st->hold == 1) {
            st->open = 1;
            st->pick_mode = 0;
            return 1;
        }
        break;
    case 2:
        if (st->hold == 1) {
            st->open = 1;
            st->pick_mode = 0;
            return 1;
        }
        break;
    }
    st->open = 0;
    return 0;
}

/* 0x802DBB20 (0x70): whether the pointer is usable: always with the pad option, else once the remote's pointer has been
 * live for `dpd_wait` frames. */
u32 ainpc_page_dpd_ready_ck(void)
{
    if (get_option_cfg(4) == 1) {
        return 1;
    }
    if (WPADIsDpdEnabled(0) == 1) {
        if (ainpc_page_state.dpd_wait == 0) {
            return 1;
        }
        ainpc_page_state.dpd_wait--;
    }
    return 0;
}

/* 0x802DBB90 (0x420): finds what the cursor is over: a place-list entry of the item list (its help record decides the
 * pick's kind and id, and a changed pick moves the label), or, in world mode, a monster.  Returns 1 on a pick. */
u32 ainpc_page_target_pick(void)
{
    CockpitPointerPage* st = &ainpc_page_state;
    s32 index;
    PlaceRec* rec;

    st->target_hit = 0;
    if (st->pointer_valid == 0) {
        st->selecting = 0;
        return 0;
    }
    switch (st->pick_mode) {
    case 0:
        index = place_list_overlap_find(st->cursor.x, st->cursor.y, 10, 10);
        if ((s8)index < 0) {
            break;
        }
        rec = (PlaceRec*)place_rec_key_ptr(index);
        if (rec == 0) {
            break;
        }
        place_rec_copy(&st->row, rec);
        switch (rec->kind) {
        case 1:
        case 2:
            if (st->sel.valid == 0 || st->sel.kind != 0 || st->sel.id != rec->value_0x08) {
                const _mh_ivec2_& label = ainpc_page_label_pos_get(0, st->cursor);
                uv_pair_copy(&st->sel.pos, &label);
            }
            st->sel.valid = 1;
            st->sel.kind = 0;
            st->sel.id = rec->value_0x08;
            st->sel.index = index;
            st->target_hit = 1;
            st->selecting = 1;
            break;
        case 3:
        case 4:
            if (st->sel.valid == 0 || st->sel.kind != 0 || st->sel.id != rec->value_0x06) {
                const _mh_ivec2_& label = ainpc_page_label_pos_get(0, st->cursor);
                uv_pair_copy(&st->sel.pos, &label);
            }
            st->sel.valid = 1;
            st->sel.kind = 0;
            st->sel.id = rec->value_0x06;
            st->sel.index = index;
            st->target_hit = 1;
            st->selecting = 1;
            break;
        case 5:
            if (st->sel.valid == 0 || st->sel.kind != 0 || st->sel.id != 0xFFFF) {
                const _mh_ivec2_& label = ainpc_page_label_pos_get(0, st->cursor);
                uv_pair_copy(&st->sel.pos, &label);
            }
            st->sel.valid = 1;
            st->sel.kind = 0;
            st->sel.id = 0xFFFF;
            st->sel.index = index;
            st->target_hit = 1;
            st->selecting = 1;
            break;
        case 6:
            if (st->sel.valid == 0 || st->sel.kind != 0 || st->sel.id != rec->value_0x0A) {
                const _mh_ivec2_& label = ainpc_page_label_pos_get(0, st->cursor);
                uv_pair_copy(&st->sel.pos, &label);
            }
            st->sel.valid = 1;
            st->sel.kind = 0;
            st->sel.id = rec->value_0x0A;
            st->sel.index = index;
            st->target_hit = 1;
            st->selecting = 1;
            break;
        case 7:
            if (st->sel.valid == 0 || st->sel.kind != 1 || st->sel.id != rec->value_0x04) {
                const _mh_ivec2_& label = ainpc_page_label_pos_get(1, st->cursor);
                uv_pair_copy(&st->sel.pos, &label);
            }
            st->sel.valid = 1;
            st->sel.kind = 1;
            st->sel.id = rec->value_0x04;
            st->sel.index = index;
            break;
        case 8:
            if (st->sel.valid == 0 || st->sel.kind != 2 || st->sel.id != rec->value_0x04) {
                const _mh_ivec2_& label = ainpc_page_label_pos_get(2, st->cursor);
                uv_pair_copy(&st->sel.pos, &label);
            }
            st->sel.valid = 1;
            st->sel.kind = 2;
            st->sel.id = rec->value_0x04;
            st->sel.index = index;
            break;
        case 9:
            if (st->sel.valid == 0 || st->sel.kind != 3 || st->sel.id != rec->value_0x04) {
                const _mh_ivec2_& label = ainpc_page_label_pos_get(3, st->cursor);
                uv_pair_copy(&st->sel.pos, &label);
            }
            st->sel.valid = 1;
            st->sel.kind = 3;
            st->sel.id = rec->value_0x04;
            st->sel.index = index;
            break;
        default:
            return 0;
        }
        return 1;
    case 1:
        return ainpc_page_monster_pick();
    }
    return 0;
}

/* 0x802DBFB0 (0xFC): whether `ray` passes within the sphere's radius; writes the distance along the ray. */
u32 hud_ray_sphere_hit_ck(HudSphere* sphere, HudRay* ray, f32* dist)
{
    VEC3 rel;
    VEC3 along;
    VEC3 tmp;
    VEC3 tmp2;
    f32 dot;

    VEC3_ctor(&rel);
    VEC3_ctor(&along);
    subVec3(&tmp, &sphere->center, &ray->origin);
    copyVec3(&rel, &tmp);
    dot = vec3_dot(&rel.x, &ray->dir.x);
    if (dot > 0.0f) {
        vec3_scale_by(&along.x, &ray->dir.x, dot);
        subVec3(&tmp2, &rel, &along);
        copyVec3(&rel, &tmp2);
    }
    if (vec3_dot(&rel.x, &rel.x) <= sphere->radius * sphere->radius) {
        *dist = abs_f32(dot);
        return 1;
    }
    return 0;
}

/* 0x802DC0AC (0x1BC): builds the world ray through screen point `screen` from the current camera: its origin and
 * its normalised direction. */
void hud_pointer_ray_get(VEC3* origin, VEC3* dir, _mh_ivec2_* screen)
{
    s32 handle_copy;
    f32 sin;
    f32 cos;
    s32 handle;
    HudCamera* camera;
    VEC3 right;
    VEC3 up;
    VEC3 fwd;
    VEC3 step;
    VEC3 tmp;
    _MH_VEC2 half;
    f32 scale;

    handle = scn_root_get_current_camera(pRoot);
    word_copy_return_dst(&handle_copy, &handle);
    camera = (HudCamera*)word_read(&handle_copy);
    VEC3_ctor(&right);
    VEC3_ctor(&up);
    VEC3_ctor(&fwd);
    subVec3(&tmp, &camera->target, &camera->pos);
    copyVec3(&fwd, &tmp);
    vec3_cross(&right.x, &fwd.x, &camera->up.x);
    vec3_normalize_into(&right, &right);
    vec3_normalize_into(&fwd, &fwd);
    vec3_cross(&up.x, &fwd.x, &right.x);
    get_ScreenSize(&half);
    half.x *= 0.5f;
    half.y *= 0.5f;
    sin_cos_deg(&sin, &cos, 0.5f * camera->fovy);
    scale = half.y * cos / sin;
    vec3_scale_by(&dir->x, &fwd.x, scale);
    VEC3_ctor(&step);
    vec3_scale_by(&step.x, &right.x, (f32)screen->x - half.x);
    addVec3To(dir, &step);
    vec3_scale_by(&step.x, &up.x, (f32)screen->y - half.y);
    addVec3To(dir, &step);
    copyVec3(origin, &camera->pos);
    vec3_normalize_into(dir, dir);
}

/* 0x802DC268 (0x1F8): the world pick: the nearest live monster of this area (with a menu row) whose hit spheres the
 * pointer ray crosses becomes the pick.  Returns whether one was found. */
u32 ainpc_page_monster_pick(void)
{
    CockpitPointerPage* st = &ainpc_page_state;
    _ENEMY_WORK* em;
    u16 max = get_move_work_max(3);
    u8 area = get_now_areano();
    nw4r::math::CAPSULE capsule;
    HudSphere sphere;
    HudRay ray;
    s16* part;
    u32 i;
    f32 best;
    f32 dist;

    eft_capsule_ctor(&capsule);
    eft_vec_ctor(&sphere.center);
    hud_ray_init(&ray);
    best = 3.4028235e38f;
    st->selecting = 0;
    hud_pointer_ray_get(&ray.origin, &ray.dir, &st->cursor);
    em = (_ENEMY_WORK*)get_move_work_adrs(3);
    for (i = 0; i < max; i++, em++) {
        if (em->active == 0 || em->visible_0x001 == 0) {
            continue;
        }
        if (em->field_0x1D4 <= 0.0f) {
            continue;
        }
        if ((u32)em_act_ck(em, 12, 0xFF) == 1) {
            continue;
        }
        if (em->area_no != area) {
            continue;
        }
        if (em->field_0x1E2 == 4 && em_tall_ck(em) == 0) {
            continue;
        }
        if (menu_row_monster_index_get(em->team) < 0) {
            continue;
        }
        part = (s16*)em_hit_parts_get(em);
        if (part == 0) {
            continue;
        }
        for (; *part != -1; part += 22) {
            if ((u8)em_part_sphere_get(em, (EmWeakPoint*)part, (PlCapsule*)&capsule, (HitSphere*)&sphere.center) == 0 &&
                hud_ray_sphere_hit_ck(&sphere, &ray, &dist) != 0 && dist < best) {
                best = dist;
                st->sel.valid = 1;
                st->sel.kind = 4;
                st->sel.id = em->team;
                uv_pair_copy(&st->sel.pos, &st->cursor);
                st->target_hit = 1;
                st->selecting = 1;
            }
        }
    }
    return st->selecting != 0;
}

/* 0x802DC460 (0x38): constructs a ray (its origin and direction). */
HudRay* hud_ray_init(HudRay* ray)
{
    VEC3_ctor(&ray->origin);
    VEC3_ctor(&ray->dir);
    return ray;
}

/* 0x802DC498 (0xD0): counts the live monsters of this area the pointer page can pick. */
u8 ainpc_page_monster_count(void)
{
    _ENEMY_WORK* em;
    u16 max = get_move_work_max(3);
    u8 area = get_now_areano();
    u8 count = 0;
    u32 i;

    em = (_ENEMY_WORK*)get_move_work_adrs(3);

    for (i = 0; i < max; i++, em++) {
        if (em->active == 0) {
            continue;
        }
        if (em_die_ck(em) != 0) {
            continue;
        }
        if (em->area_no != area) {
            continue;
        }
        if (em->field_0x1E2 == 4 && em_tall_ck(em) == 0) {
            continue;
        }
        if (menu_row_monster_index_get(em->team) < 0) {
            continue;
        }
        count++;
    }
    return count;
}

/* 0x802DC568 (0x1F8): whether the cursor lies inside the confirm box (layout 1039 posed by animation 1047). */
u32 ainpc_page_cursor_in_box_ck(_mh_ivec2_* cursor)
{
    _mh_ivec2_ base;
    _MH_VEC2 scale;
    _SPR_DATA_ frame;
    _SPR_DATA_* box;
    s16 y;
    u16 w;
    u16 h;

    get_lsp_data(1039, &base);
    sprite_frame_apply(&frame, 1047, 999, 0);
    scale.x = frame.u_scale / 100.0f;
    scale.y = frame.v_scale / 100.0f;
    base.x += (s16)(frame.pos.x + (s32)(frame.ofs_x * scale.x));
    base.y += (s16)(frame.pos.y + (s32)(frame.ofs_y * scale.y));
    box = get_lsp_data(1118, 0);
    y = box->pos.y + (box->ofs_y + cursor->y);
    w = box->width + (s32)(frame.width * scale.x);
    if (w < (u16)(box->width + ((s16)(box->pos.x + (box->ofs_x + cursor->x)) - base.x))) {
        return 0;
    }
    h = box->height + (s32)(frame.height * scale.y);
    return (u16)(box->height + (y - base.y)) <= h;
}

/* 0x802DC760 (0xFC): what the page can pick right now: 1 items (or monsters in world mode), 2 monster-list rows only,
 * 0 nothing. */
u8 ainpc_page_pick_kind_get(CockpitPointerPage* st)
{
    u8 kind = 0;
    u8 lists;
    u8 items;

    switch (st->pick_mode) {
    case 0:
        lists = place_list_key_count(7);
        lists += place_list_key_count(8);
        lists += place_list_key_count(9);
        items = place_list_key_count(1);
        items += place_list_key_count(2);
        items += place_list_key_count(3);
        items += place_list_key_count(4);
        items += place_list_key_count(5);
        items += place_list_key_count(6);
        if (items != 0) {
            kind = 1;
        } else if (lists != 0) {
            kind = 2;
        }
        break;
    case 1:
        if (ainpc_page_monster_count() != 0) {
            kind = 1;
        }
        break;
    }
    return kind;
}

/* 0x802DC85C (0x94): whether the pick can be marked: 0 yes, 1 already marked, 2 no free row. */
u8 ainpc_page_sel_state_get(CockpitPointerPick* sel, PlaceRec* row)
{
    if (sel->kind > 3) {
        if ((s32)sel->kind == 4 && monster_mark_ck(sel->id) != 0) {
            return 1;
        }
    } else {
        if (row->kind == 0) {
            return 1;
        }
        if (menu_row_find((struct MenuRowData*)row) == 1) {
            return 1;
        }
        if ((u16)menu_row_empty_count() == 0) {
            return 2;
        }
    }
    return 0;
}

/* 0x802DC8F0 (0xA0): reads the trigger and hold button words of the active controller (none while the lobby network
 * is busy). */
void ainpc_page_buttons_get(u16* trigger, u16* hold)
{
    *hold = 0;
    *trigger = 0;
    if (lobby_net_busy_ck() != 1) {
        switch ((u8)get_ControlType(0)) {
        case 1:
            *trigger = Psw[0].control_0x0FC;
            *hold = Psw[0].field_0x100;
            break;
        case 2:
            *trigger = Psw[0].field_0xca;
            *hold = Psw[0].field_0xce;
            break;
        }
    }
}

/* 0x802DC990 (0x84): whether the page's open button is down (remote B / classic ZL). */
u32 ainpc_page_open_button_ck(u16 trigger)
{
    u32 down = 0;

    switch ((u8)get_ControlType(0)) {
    case 1:
        if (trigger & 4) {
            down = 1;
        }
        break;
    case 2:
        if (trigger & 0x100) {
            down = 1;
        }
        break;
    }
    return down;
}

/* 0x802DCA14 (0xE8): whether the select button counts as pressed in this step (held while confirming, else the
 * trigger; inverted for the pad option while confirming). */
u32 ainpc_page_select_button_ck(CockpitPointerPage* st, u16 trigger, u16 hold)
{
    u32 down = 0;

    if (get_option_cfg(4) == 0) {
        if ((u8)(st->step + 253) <= 1) {
            if (trigger & 0x200) {
                down = 1;
            }
        } else if (hold & 0x200) {
            down = 1;
        }
    } else {
        down = 0;
        if (hold & 0x200) {
            down = 1;
        }
        if ((u8)(st->step + 253) <= 1) {
            if (down == 1) {
                down = 0;
            } else if (down == 0) {
                down = 1;
            }
        }
    }
    return down;
}

/* 0x802DCAFC (0x25C): moves the cursor with the d-pad (accelerating while held), clamps it to the screen and shifts
 * the trail; the first frame centres it. */
void ainpc_page_cursor_pad_move(CockpitPointerPage* st, u16 trigger)
{
    s16 dx = 0;
    s16 dy = 0;
    _MH_VEC2 screen;
    s16 speed;
    s16 cx;
    s16 cy;

    get_ScreenSize(&screen);
    if (trigger & 0x3C00) {
        if (st->pad_speed < 8) {
            st->pad_speed += 2;
        }
    } else {
        st->pad_speed = -16;
    }
    speed = 4;
    if (st->pad_speed > 0) {
        speed = st->pad_speed + 4;
    }
    if (trigger & 0x2000) {
        dy = -speed;
    }
    if (trigger & 0x1000) {
        dy += speed;
    }
    if (trigger & 0x800) {
        dx = -speed;
    }
    if (trigger & 0x400) {
        dx += speed;
    }
    if (st->step != 0) {
        st->cursor.x += dx;
        st->cursor.y += dy;
        if (st->cursor.x < 0) {
            st->cursor.x = 0;
        }
        if (st->cursor.y < 0) {
            st->cursor.y = 0;
        }
        if ((s16)screen.x < st->cursor.x) {
            st->cursor.x = screen.x;
        }
        if ((s16)screen.y < st->cursor.y) {
            st->cursor.y = screen.y;
        }
        if (st->pointer_valid != 0) {
            st->trail[3] = st->trail[2];
            st->trail[2] = st->trail[1];
            st->trail[1] = st->trail[0];
            st->trail[0] = st->cursor;
        }
    } else {
        st->pointer_valid = 1;
        st->pad_speed = -16;
        cx = (s32)(screen.x / 2.0f);
        st->cursor.x = cx;
        cy = (s32)(screen.y / 2.0f);
        st->cursor.y = cy;
        st->trail[0].x = cx;
        st->trail[1].x = cx;
        st->trail[2].x = cx;
        st->trail[3].x = cx;
        st->trail[0].y = cy;
        st->trail[1].y = cy;
        st->trail[2].y = cy;
        st->trail[3].y = cy;
    }
}

/* 0x802DCD58 (0x134): moves the cursor to the remote's pointer and shifts the trail (reset when the pointer comes
 * back on screen). */
void ainpc_page_cursor_pointer_move(CockpitPointerPage* st)
{
    s16 x;
    s16 y;

    pointer_update();
    x = pointer_center[0];
    y = pointer_center[1];
    if (pointer_dpd_state != 0) {
        st->pointer_valid = 1;
    } else {
        st->pointer_valid = 0;
    }
    if (st->step != 0) {
        if (st->pointer_valid != 0) {
            if (pointer_dpd_state == 1) {
                st->trail[3] = st->trail[2];
                st->trail[2] = st->trail[1];
                st->trail[1] = st->trail[0];
                st->trail[0].x = x;
                st->trail[0].y = y;
            } else {
                st->trail[0].x = x;
                st->trail[1].x = x;
                st->trail[2].x = x;
                st->trail[3].x = x;
                st->trail[0].y = y;
                st->trail[1].y = y;
                st->trail[2].y = y;
                st->trail[3].y = y;
            }
        }
        st->cursor.x = x;
        st->cursor.y = y;
    } else if (st->pointer_valid != 0) {
        st->cursor.x = x;
        st->trail[0].x = x;
        st->trail[1].x = x;
        st->trail[2].x = x;
        st->trail[3].x = x;
        st->cursor.y = y;
        st->trail[0].y = y;
        st->trail[1].y = y;
        st->trail[2].y = y;
        st->trail[3].y = y;
    }
}

/* The screen fractions the off-screen arrow snaps to a corner at (0x80792634..0x80792640). */
f32 hud_edge_corner_lo_x = 0.1f;
f32 hud_edge_corner_hi_x = 0.9f;
f32 hud_edge_corner_lo_y = 0.1f;
f32 hud_edge_corner_hi_y = 0.9f;

/* 0x802DCE8C (0x67C): the screen-edge point the pointer left through (from the last two trail points) and the
 * direction code of the edge (0 right, 2 bottom, 4 left, 6 top; the odd codes are the corners, 5 when unmoved). */
s32 hud_edge_dir_get(_mh_ivec2_* out, const _mh_ivec2_* from, const _mh_ivec2_* to)
{
    _MH_VEC2 screen;
    s16 dx;
    s16 dy;
    s32 dir;
    s16 near;
    s16 right;
    s16 bottom;
    s16 edge;
    s16 top;

    get_ScreenSize(&screen);
    dx = from->x - to->x;
    dy = from->y - to->y;
    if (dx != 0) {
        if (dy != 0) {
            if (from->x < to->x) {
                edge = (s16)((-to->x * dy) / dx);
                edge += to->y;
                out->y = edge;
                if (out->y >= 0 && out->y <= screen.y) {
                    out->x = 0;
                    dir = 4;
                } else if (out->y < 0) {
                    edge = (s16)((-to->y * dx) / dy);
                    edge += to->x;
                    out->x = edge;
                    out->y = 0;
                    dir = 6;
                } else {
                    edge = (s32)(dx * (screen.y - to->y) / dy);
                    edge += to->x;
                    out->x = edge;
                    out->y = screen.y;
                    dir = 2;
                }
            } else {
                edge = (s32)(dy * (screen.x - to->x) / dx);
                edge += to->y;
                out->y = edge;
                if (out->y >= 0 && out->y <= screen.y) {
                    out->x = screen.x;
                    dir = 0;
                } else if (out->y < 0) {
                    edge = (s16)((-to->y * dx) / dy);
                    edge += to->x;
                    out->x = edge;
                    out->y = 0;
                    dir = 6;
                } else {
                    edge = (s32)(dx * (screen.y - to->y) / dy);
                    edge += to->x;
                    out->x = edge;
                    out->y = screen.y;
                    dir = 2;
                }
            }
        } else {
            if (from->x < to->x) {
                out->x = 0;
                dir = 4;
            } else {
                out->x = screen.x;
                dir = 0;
            }
            out->y = from->y;
        }
    } else if (dy != 0) {
        if (from->y != to->y) {
            out->x = from->x;
            if (from->y < to->y) {
                out->y = 0;
                dir = 6;
            } else {
                out->y = screen.y;
                dir = 2;
            }
        } else {
            out->y = 0;
            out->x = 0;
            dir = 5;
        }
    } else {
        uv_pair_copy(out, from);
        top = from->y;
        near = from->x;
        right = screen.x - near;
        bottom = screen.y - top;
        if (near < right) {
            dir = 4;
        } else {
            near = right;
            dir = 0;
        }
        if (top < near) {
            near = top;
            dir = 6;
        }
        if (bottom < near) {
            dir = 2;
        }
        switch (dir) {
        case 0:
            out->x = screen.x;
            break;
        case 2:
            out->y = screen.y;
            break;
        case 4:
            out->x = 0;
            break;
        case 6:
            out->y = 0;
            break;
        }
    }
    switch (dir) {
    case 0:
        if (out->y < (s16)(screen.y * hud_edge_corner_lo_y)) {
            out->y = 0;
            return 7;
        }
        if (out->y > (s16)(screen.y * hud_edge_corner_hi_y)) {
            out->y = screen.y;
            return 1;
        }
        break;
    case 2:
        if (out->x < (s16)(screen.x * hud_edge_corner_lo_x)) {
            out->x = 0;
            return 3;
        }
        if (out->x > (s16)(screen.x * hud_edge_corner_hi_x)) {
            out->x = screen.x;
            return 1;
        }
        break;
    case 4:
        if (out->y < (s16)(screen.y * hud_edge_corner_lo_y)) {
            out->y = 0;
            return 5;
        }
        if (out->y > (s16)(screen.y * hud_edge_corner_hi_y)) {
            out->y = screen.y;
            return 3;
        }
        break;
    case 6:
        if (out->x < (s16)(screen.x * hud_edge_corner_lo_x)) {
            out->x = 0;
            return 5;
        }
        if (out->x > (s16)(screen.x * hud_edge_corner_hi_x)) {
            out->x = screen.x;
            dir = 7;
        }
        break;
    }
    return dir;
}

/* 0x802DD508 (0x258): draws the pointer cursor for the page's step (its colour and pick marks), or the edge arrow
 * while the pointer is off screen. */
void ainpc_page_cursor_draw(void)
{
    CockpitPointerPage* st = &ainpc_page_state;
    u32 color = 0;
    u8 draw = st->active == 1;
    s32 marked = 0;
    u16 id;
    u8 can_pick = 0;
    u8 state = 0;
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    s32 dir;

    set_zmode(0, 0, 0);
    set_blendmode(4, 5, 1);
    if (st->step == 0) {
        return;
    }
    switch (st->pick_mode) {
    case 0:
        can_pick = ainpc_page_pick_kind_get(st) == 1;
        break;
    case 1:
        can_pick = 1;
        break;
    }
    switch (st->step) {
    default:
        return;
    case 0:
        draw = 0;
        break;
    case 1:
        if (st->selecting == 0 || st->target_hit == 0) {
            id = 1112;
        } else {
            id = 1118;
        }
        break;
    case 2:
        if (st->target_hit == 0) {
            id = 1123;
        } else {
            id = 1118;
        }
        break;
    case 3:
        id = 1123;
        color = 0xF096C8FF;
        break;
    case 4:
        id = 1122;
        marked = 1;
        break;
    }
    if (can_pick != 0) {
        if (st->selecting != 0) {
            switch (ainpc_page_sel_state_get(&st->sel, &st->row)) {
            case 0:
                state = 0;
                break;
            case 1:
                state = 1;
                break;
            case 2:
                state = 2;
                break;
            }
        } else if (st->mark_frame != 0) {
            state = 3;
        }
    }
    if (draw != 0) {
        if (st->pointer_valid == 0) {
            dir = hud_edge_dir_get(&pos, &st->trail[0], &st->trail[1]);
            sprite_frame_apply(&spr, 1124, st->cycle, 0);
            spr.tex_id = (u16)dir << 13;
            draw_sprite(spr, &pos);
            return;
        }
        hud_cursor_sprite_draw(st, id, color);
        if (marked != 0 && (u8)(state + 255) <= 1) {
            draw_sprite_anim_idx(1127, st->frame_1127, &st->cursor);
        }
    }
}

/* 0x802DD760 (0x268): draws the pick's mark, the place-list highlight and the label of the pick (a monster's name or
 * the item/list row's text). */
void ainpc_page_label_draw(void)
{
    CockpitPointerPage* st = &ainpc_page_state;
    s32 confirming;
    s32 show;
    u8 can_pick;
    u8 frame;
    u8 state;
    u32 index;
    u8 len;

    show = 1;
    confirming = 0;
    can_pick = 0;
    frame = 3;
    state = 0;
    set_zmode(0, 0, 0);
    set_blendmode(4, 5, 1);
    font_flush();
    if (st->step == 0) {
        if (st->pick_mode == 1) {
            hud_pick_mark_draw(0, 0);
        }
        return;
    }
    switch (st->pick_mode) {
    case 0:
        can_pick = ainpc_page_pick_kind_get(st) == 1;
        break;
    case 1:
        can_pick = 1;
        break;
    }
    switch (st->step) {
    default:
        return;
    case 0:
        show = 0;
        frame = 0;
        break;
    case 1:
        show = 0;
        break;
    case 2:
    case 3:
        break;
    case 4:
        confirming = 1;
        frame = st->confirm_clock;
        break;
    }
    if (can_pick != 0) {
        if (st->selecting != 0) {
            switch (ainpc_page_sel_state_get(&st->sel, &st->row)) {
            case 0:
                state = 0;
                break;
            case 1:
                state = 1;
                break;
            case 2:
                state = 2;
                break;
            }
        } else if (st->mark_frame != 0) {
            frame = st->mark_frame;
            state = 3;
        }
        hud_pick_mark_draw(state, frame);
    }
    index = st->sel.index;
    place_list_highlight_draw(1, &index);
    if (show != 0 && st->sel.valid != 0) {
        if (st->sel.kind > 3) {
            if ((s32)st->sel.kind == 4 && st->selecting != 0 && confirming == 0) {
                len = flfntStrLen((char*)str_tbl_33_get(st->sel.id));
                if (len < 4) {
                    len = 4;
                }
                len -= 4;
                draw_sprite_anim_ary((const u16*)get_menu_lsp_tbl(97), len, &st->sel.pos);
                draw_font_anim_idx(1131, len, str_tbl_33_get(st->sel.id), 0, &st->sel.pos);
            }
        } else if (st->sel.id != 0) {
            ainpc_page_label_text_draw(st, st->sel.kind, st->sel.id, st->sel.pos);
        }
        font_flush();
    }
}

/* 0x802DD9C8 (0xA0): draws the pick-state mark `state` (menu layout table row 96, its classic-controller column under
 * option 7) at frame `frame`. */
void hud_pick_mark_draw(u8 state, u8 frame)
{
    _mh_ivec2_ pos;
    const u16* (*rows)[4];

    get_lsp_data(1039, &pos);
    rows = (const u16* (*)[4])get_menu_lsp_tbl(96);
    if (get_option_cfg(7) == 0) {
        draw_sprite_anim_ary(rows[0][state], frame, &pos);
    } else {
        draw_sprite_anim_ary(rows[1][state], frame, &pos);
    }
}

/* The label window's size in 18-pixel cells for each pick kind (0x805D55B8). */
s16 hud_label_cell_tbl[4][2] = {
    { 14, 3 },
    { 14, 8 },
    { 14, 3 },
    { 14, 3 },
};

/* 0x802DDA68 (0x19C): where the label window of pick kind `kind` goes: to the side of the screen away from `cursor`,
 * near the bottom. */
_mh_ivec2_ ainpc_page_label_pos_get(s8 kind, _mh_ivec2_ cursor)
{
    s16 w = hud_label_cell_tbl[kind][0] * 18 + 64;
    s16 h = hud_label_cell_tbl[kind][1] * 18 + 64;
    _MH_VEC2 screen;
    _MH_VEC2 pos;
    _mh_ivec2_ out;
    f32 half;

    get_ScreenSize(&screen);
    if ((u32)ck_WideMode() == 1) {
        if (screen.x / 2.0f < cursor.x) {
            pos.x = 48.0f;
        } else {
            pos.x = screen.x - w - 48.0f;
        }
    } else {
        half = screen.x / 2.0f;
        if (half < cursor.x) {
            pos.x = 8.0f + (half - w);
        } else {
            pos.x = half - 8.0f;
        }
    }
    pos.y = 16.0f + (0.75f * screen.y - h);
    out.x = pos.x;
    out.y = pos.y;
    return out;
}

/* 0x802DDC04 (0x35C): draws the label window of a list/item pick: its frame (with the button guide while the pick is
 * possible) and the name and description lines of `id`. */
void ainpc_page_label_text_draw(CockpitPointerPage* st, s8 kind, u16 id, _mh_ivec2_ pos)
{
    s16 w = hud_label_cell_tbl[kind][0] * 18 + 64;
    s16 h = hud_label_cell_tbl[kind][1] * 18 + 64;
    u32 guide;
    u8** desc;
    _mh_ivec2_ at;

    draw_window_frame_style(pos.x, pos.y, w, h, 0);
    if (st->active == 1) {
        guide = 0;
        switch (ainpc_page_pick_kind_get(st)) {
        case 1:
            if (st->step != 4) {
                guide = 1;
            }
            break;
        case 2:
            if (st->step != 3) {
                guide = 1;
            }
            break;
        }
        if (guide == 1) {
            at.x = pos.x + (s16)((u16)w & 0xFFF0) - 20;
            at.y = (s16)((u16)h & 0xFFF0) + pos.y - 28;
            put_button_guide_r(18, &at);
        }
    }
    font_set_size(18, 18);
    desc = 0;
    switch (kind) {
    case 0:
        if (id == 0xFFFF) {
            font_print_ex(pos.x + 24, pos.y + 16, 5, (s8*)get_str_tbl(68)[3]);
            font_print_ex(pos.x + 24, pos.y + 38, 0, (s8*)get_str_tbl(68)[7]);
        } else {
            font_print_ex(pos.x + 24, pos.y + 16, 5, (s8*)ItemName(id));
            font_print_ex(pos.x + 24, pos.y + 38, 0, (s8*)"%s", ItemExp(id));
        }
        break;
    case 1:
        desc = get_str_tbl(82);
        font_print_ex(pos.x + 24, pos.y + 16, 5, (s8*)str_tbl_item_kind_get(id));
        break;
    case 2:
        desc = get_str_tbl(83);
        font_print_ex(pos.x + 24, pos.y + 16, 5, (s8*)str_tbl_skill_get(id));
        break;
    case 3:
        desc = get_str_tbl(84);
        font_print_ex(pos.x + 24, pos.y + 16, 5, (s8*)str_tbl_course_get(id));
        break;
    default:
        nw4r::db::Panic("cockpit.cpp", 3148, "NW4R:Failed assertion 0");
        break;
    }
    if (desc != 0) {
        font_print_ex(pos.x + 24, pos.y + 38, 0, reinterpret_cast<s8*>(desc[id - 1]));
    }
}

/* 0x802DDF60 (0xCC): draws layout `id` at frame `frame` (in `color` when given) at the cursor, with three fading copies
 * along the trail. */
void hud_trail_sprite_draw(CockpitPointerPage* st, u16 id, u8 frame, u32 color)
{
    _SPR_DATA_ spr;
    s32 step;
    s32 alpha;
    u32 base;
    u32 rgb;
    s32 i;

    sprite_frame_apply(&spr, id, frame, 0);
    if (color != 0) {
        spr.color = color;
    }
    base = spr.color;
    rgb = base & 0xFFFFFF00;
    step = base & 0xFF;
    for (i = 1, alpha = step; i < 4; i++) {
        spr.color = rgb | ((alpha / 4) * 75 / 100);
        draw_sprite(spr, &st->trail[4 - i]);
        alpha += step;
    }
    spr.color = base;
    draw_sprite(spr, &st->cursor);
}

/* 0x802DE02C (0x1F8): draws the cursor sprite set `id` (the browse, pick, mark and hold cursors) with its trail. */
void hud_cursor_sprite_draw(CockpitPointerPage* st, u16 id, u32 color)
{
    u8 frame;

    switch (id) {
    case 1112:
        if (st->fade_in < 24) {
            hud_trail_sprite_draw(st, 1115, st->fade_in, 0);
            hud_trail_sprite_draw(st, 1116, st->fade_in, 0);
            hud_trail_sprite_draw(st, 1117, st->fade_in, 0);
        }
        hud_trail_sprite_draw(st, 1112, st->fade_in, 0);
        hud_trail_sprite_draw(st, 1113, st->fade_in, 0);
        hud_trail_sprite_draw(st, 1114, st->fade_in, 0);
        break;
    case 1118:
        if (st->grow < 5) {
            frame = st->grow + 50;
            hud_trail_sprite_draw(st, 1112, frame, 0);
            hud_trail_sprite_draw(st, 1113, frame, 0);
            hud_trail_sprite_draw(st, 1114, frame, 0);
        }
        hud_trail_sprite_draw(st, 1118, st->grow, 0);
        hud_trail_sprite_draw(st, 1119, st->grow, 0);
        hud_trail_sprite_draw(st, 1120, st->frame_1120, 0);
        hud_trail_sprite_draw(st, 1121, st->frame_1120, 0);
        break;
    case 1122:
        hud_trail_sprite_draw(st, 1122, 0, 0);
        break;
    case 1123:
        if (st->grow < 3) {
            frame = st->grow + 50;
            hud_trail_sprite_draw(st, 1112, frame, 0);
            hud_trail_sprite_draw(st, 1113, frame, 0);
            hud_trail_sprite_draw(st, 1114, frame, 0);
        }
        hud_trail_sprite_draw(st, 1123, st->grow, color);
        break;
    }
}

/* 0x802DE224 (0x14): holds the pointer page open for another screen. */
void ainpc_page_hold_set(void)
{
    ainpc_page_state.hold = 1;
}

/* 0x802DE238 (0x14): releases the hold. */
void ainpc_page_hold_clear(void)
{
    ainpc_page_state.hold = 0;
}

/* 0x802DE24C (0x14): sets the anchor offset of the page's button guide. */
void ainpc_page_pos_set(_mh_ivec2_ pos)
{
    uv_pair_copy(&ainpc_page_state.anchor, &pos);
}

/* 0x802DE260 (0x100): draws the held page's button guide (its frame, the remote/classic button and the state icon). */
void ainpc_page_guide_draw(void)
{
    CockpitPointerPage* st = &ainpc_page_state;
    u16 id = 0;
    _mh_ivec2_ at;

    set_blendmode(4, 5, 1);
    if (st->hold == 1) {
        put_lsp_anchor_offset(414, &at, st->anchor);
        if (st->active == 0) {
            id = 421;
        } else {
            switch (ainpc_page_pick_kind_get(st)) {
            case 0:
                id = 422;
                break;
            case 2:
                id = 420;
                break;
            }
        }
    }
    if (id != 0) {
        draw_sprite_ary((const u16*)get_menu_lsp_tbl(19), &at);
        if (get_option_cfg(7) == 0) {
            draw_sprite_idx(418, &at);
        } else {
            draw_sprite_idx(419, &at);
        }
        draw_sprite_idx(id, &at);
    }
}

/* 0x802DE360 (0x90): draws the page's first button mark at layout `panel` (moved by `offset` when given) unless the
 * page is up or the player is locked. */
void ainpc_page_mark_a_draw(u16 panel, const _mh_ivec2_* offset)
{
    _mh_ivec2_ at;

    if (ainpc_page_state.active != 1 && pl_root_lock_ck() != 1) {
        if (offset == 0) {
            get_lsp_data(panel, &at);
        } else {
            put_lsp_anchor_offset(panel, &at, *offset);
        }
        draw_sprite_idx(1125, &at);
    }
}

/* 0x802DE3F0 (0x90): the same for the second button mark. */
void ainpc_page_mark_b_draw(u16 panel, const _mh_ivec2_* offset)
{
    _mh_ivec2_ at;

    if (ainpc_page_state.active != 1 && pl_root_lock_ck() != 1) {
        if (offset == 0) {
            get_lsp_data(panel, &at);
        } else {
            put_lsp_anchor_offset(panel, &at, *offset);
        }
        draw_sprite_idx(1126, &at);
    }
}

/* 0x802DE480 (0x2C): whether the pointer page is open and running. */
s32 chk_pointer(void)
{
    return ainpc_page_state.open != 0 && ainpc_page_state.step != 0;
}

/* 0x802DE4AC (0x50): marks monster `monster` in the lobby world block's monster bitmap. */
void monster_mark_set(u8 monster)
{
    s8 row = menu_row_monster_index_get(monster);

    ((HudLobbyMarks*)lobby_world_block)->monster_marks[row >> 3] |= (u8)(1 << (row & 7));
}

/* 0x802DE4FC (0x54): whether monster `monster` is marked. */
s32 monster_mark_ck(u8 monster)
{
    s8 row = menu_row_monster_index_get(monster);

    return ((1 << (row & 7)) & ((HudLobbyMarks*)lobby_world_block)->monster_marks[row >> 3]) != 0;
}

/* The two item-message slots, one per split screen (0x806BDA58). */
CockpitMsgSlot cockpit_msg_slots[2];

/* 0x802DE550 (0x28): empties message slot `idx`. */
void cockpit_msg_slot_clear(u8 idx)
{
    CockpitMsgSlot* slot = &cockpit_msg_slots[idx];

    slot->hold = 0;
    slot->ticket = 0;
    slot->state = 0;
}

/* 0x802DE578 (0xF8): takes the item-message slot of `plw`'s screen for the caller holding ticket `*ticket` (a new one
 * when the slot is idle); returns 0, or -1 while another holder has it. */
s16 cockpit_ticket_acquire(_PLW* plw, u16* ticket)
{
    CockpitMsgSlot* slot;

    if (screen_split_mode_ck() == 0) {
        slot = cockpit_msg_slots;
    } else {
        slot = &cockpit_msg_slots[plw->chunk_ofs];
    }
    if (plw != 0 && screen_split_mode_ck() == 0 && Pl_master_ck(plw) == 0) {
        return -1;
    }
    if (slot->state != 0) {
        return -1;
    }
    if (slot->hold <= 0) {
        *ticket = ++slot->ticket;
        slot->hold = 4;
        return 0;
    }
    if (*ticket == slot->ticket) {
        slot->hold = 4;
        return 0;
    }
    return -1;
}

/* 0x802DE670 (0x28): whether message slot `idx` runs its dialog. */
u32 cockpit_msg_slot_busy_ck(u8 idx)
{
    return cockpit_msg_slots[idx].state != 0;
}

/* 0x802DE698 (0x26C): gives `plw` `count` of `item` with the item-got message (`flags` bit 0; bit 2 adds the fanfare,
 * bit 3 picks the other text); when the pouch is full the slot opens the discard dialog.  Returns the give result, or
 * -1 while the slot is taken. */
s16 item_get_with_msg(_PLW* plw, u16 item, u16 ticket, u8 flags, s16 count)
{
    CockpitMsgSlot* slot;
    s16 result;

    if (screen_split_mode_ck() == 0) {
        slot = cockpit_msg_slots;
    } else {
        slot = &cockpit_msg_slots[plw->chunk_ofs];
    }
    if (slot->state != 0) {
        return -1;
    }
    if (slot->hold > 0 && ticket != slot->ticket) {
        return -1;
    }
    slot->game_mode = GameMode_ck();
    switch (slot->game_mode) {
    default:
        return -1;
    case 1:
        result = pl_item_add(plw, item, count);
        break;
    case 2:
        result = userdata_item_give(lobby_world_block, item, count, 1);
        break;
    }
    slot->item = 0;
    switch (result) {
    case 0:
    case 1:
        if (flags & 1) {
            if (flags & 4) {
                hud_item_msg_push(2, 10, item);
                se_item_get_play();
            } else if (flags & 8) {
                hud_item_msg_push(2, 19, item);
            } else {
                hud_item_msg_push(2, 8, item);
            }
        }
        break;
    case 2:
    case 3:
        if (flags & 1) {
            hud_item_msg_push(2, 17, item);
        }
        break;
    case 5:
        if (item_bonus_class_get(item) != 0) {
            if (flags & 1) {
                hud_item_msg_push(1, 14, 0);
            }
            return result;
        }
        slot->item = item;
        slot->state = 1;
        slot->count = count;
        slot->msg_id = -1;
        slot->plw = plw;
        slot->page2 = pl_item_page2_ck(plw) == 1;
        if (slot->game_mode == 2) {
            slot->pouch = (IdValue*)userdata_equip_item_slots_get(lobby_world_block);
            slot->box = (IdValue*)((HudLobbyItems*)lobby_world_block)->box;
        } else {
            slot->pouch = (IdValue*)plw->slot_id;
            slot->box = (IdValue*)plw->spare_slot_id;
        }
        slot->fade = 0;
        break;
    }
    return result;
}

/* 0x802DE904 (0x34): reads player `player`'s menu keys (the pressed d-pad/confirm bits and the held shoulder bits). */
void cockpit_msg_slot_keys_get(CockpitMsgSlot* slot, u8 player)
{
    PlayerPad* pad = &Psw[player];

    slot->keys = pad->button_0x2C0.pressed_0x04 & 0x3F;
    slot->keys |= pad->button_0x2C0.held_0x14 & 0xF;
}

/* 0x802DE938 (0x28): whether list index `index` lies on the item box page. */
s32 cockpit_msg_slot_box_ck(CockpitMsgSlot* slot, s16 index)
{
    if (slot->page2 != 0 && index >= 24) {
        return 1;
    }
    return 0;
}

/* 0x802DE960 (0x40): the pouch or box entry of list index `index`. */
IdValue* cockpit_msg_slot_entry_get(CockpitMsgSlot* slot, s16 index)
{
    if (slot->page2 != 0 && index >= 24) {
        return &slot->box[index - 24];
    }
    return &slot->pouch[index];
}

/* 0x802DE9A0 (0x70): whether the rows of the shown page may be picked (not a kind-1 item on the box page). */
s32 cockpit_msg_slot_page_usable_ck(CockpitMsgSlot* slot)
{
    if (GetItemData(slot->item)->kind_0x00 != 1 &&
        cockpit_msg_slot_box_ck(slot, slot->list.columns * slot->list.page) != 0) {
        return 0;
    }
    return 1;
}

/* 0x802DEA10 (0xD8): fills the shown page's rows (their entries and pick state) and marks the cursor row while
 * choosing. */
void cockpit_msg_slot_rows_build(CockpitMsgSlot* slot)
{
    MenuEntry* row = slot->rows;
    s16 index = slot->list.page * slot->list.columns;
    s8 usable = 1;
    s32 i;

    if (cockpit_msg_slot_box_ck(slot, index) != 0) {
        index -= 24;
        usable = cockpit_msg_slot_page_usable_ck(slot);
    }
    for (i = 0; i < slot->list.columns; i++, row++, index++) {
        row->field_0x02 = usable;
        row->selected = 0;
        row->field_0x06 = index;
    }
    if (slot->state >= 4) {
        slot->rows[slot->list.column].selected = 1;
    }
}

/* 0x802DEAE8 (0xAC): builds the yes/no choice of the discard question (menu message row 14), marking `choice`. */
void cockpit_msg_slot_choice_a_build(CockpitMsgSlot* slot, s16 choice)
{
    MenuEntry* entry = slot->choice;
    s32 i = 0;

    do {
        entry->selected = 0;
        entry->field_0x02 = 1;
        entry->field_0x08 = get_menu_msg(14, i);
        entry->field_0x0C = 0;
        i++;
        entry++;
    } while (i < 2);
    if (slot->state != 4) {
        slot->choice[choice].selected = 1;
    } else {
        slot->choice[choice].field_0x0C = 0xE8A40FFF;
    }
}

/* 0x802DEB94 (0x84): builds the yes/no choice of the swap question (menu message row 13), marking `choice`. */
void cockpit_msg_slot_choice_b_build(CockpitMsgSlot* slot, s16 choice)
{
    MenuEntry* entry = slot->choice;
    s32 i = 0;

    do {
        entry->selected = 0;
        entry->field_0x02 = 1;
        entry->field_0x08 = get_menu_msg(13, i);
        entry->field_0x0C = 0;
        i++;
        entry++;
    } while (i < 2);
    slot->choice[choice].selected = 1;
}

/* 0x802DEC18 (0x47C): runs message slot `idx`'s pouch-full dialog: the message, the discard question, the pick of the
 * item to swap out and the swap itself.  Returns 1 while the dialog is up. */
s32 cockpit_msg_slot_update(u8 idx)
{
    CockpitMsgSlot* slot = &cockpit_msg_slots[idx];
    IdValue* entry;
    u8 rows;

    if (slot->state == 0) {
        if (slot->hold != 0) {
            slot->hold--;
        }
        return 0;
    }
    cockpit_msg_slot_keys_get(slot, idx);
    slot->list.input = slot->keys;
    slot->list.step = 0;
    if (slot->state == 4) {
        if (slot->fade != 0) {
            slot->fade--;
        }
    } else if (slot->fade < 3) {
        slot->fade++;
    }
    switch (slot->state) {
    case 0:
        return 0;
    case 1:
        if (slot->msg_id == -1) {
            slot->msg_id = hud_item_msg_push(2, 8, slot->item);
            rows = 24;
            if (slot->page2 != 0) {
                rows = 32;
            }
            if (screen_split_mode_ck() == 0) {
                slot->list.columns = 8;
            } else {
                slot->list.columns = 4;
            }
            slot->list.page = 0;
            slot->list.pages = menu_page_count(rows, slot->list.columns);
            slot->list.column = 0;
        } else {
            if (slot->msg_id == (s8)hud_msg_shown_id_get()) {
                slot->state++;
                slot->msg_delay = 6;
            }
            break;
        }
        return 0;
    case 2:
        if (slot->msg_delay != 0) {
            u8** text = get_str_tbl(4);

            slot->msg_delay--;
            if (slot->msg_delay == 0) {
                hud_msg_sub_set(slot->msg_id, (const char*)text[7]);
            }
        }
        if (slot->msg_id == (s8)hud_msg_shown_id_get()) {
            break;
        }
        slot->choice_index = 0;
        cockpit_msg_slot_choice_a_build(slot, slot->choice_index);
        slot->state++;
    case 3:
        if (slot->keys & 0x10) {
            switch (slot->choice_index) {
            case 0:
                slot->state++;
                sysSE_req(0);
                break;
            case 1:
                slot->item = 0;
                slot->state = 0;
                sysSE_req(0);
                return 1;
            }
        } else if (slot->keys & 3) {
            slot->choice_index = menu_cursor_step(slot->choice_index, 2, slot->keys, 1, 2);
        }
        cockpit_msg_slot_choice_a_build(slot, slot->choice_index);
        break;
    case 4:
        if (slot->keys & 0x10) {
            if (slot->rows[slot->choice_index].field_0x02 != 0) {
                slot->choice_index = 1;
                slot->state++;
                sysSE_req(0);
                cockpit_msg_slot_choice_b_build(slot, slot->choice_index);
            } else {
                sysSE_req(2);
            }
        } else if (slot->keys & 0x20) {
            slot->state = 3;
            sysSE_req(1);
        } else {
            menu_cursor_column_step((MenuListWork*)&slot->list);
        }
        break;
    case 5:
        switch (toggle_word_step_dpad(&slot->choice_index, slot->keys, 1, 2)) {
        default:
            cockpit_msg_slot_choice_b_build(slot, slot->choice_index);
            break;
        case 1:
            entry = cockpit_msg_slot_entry_get(slot, slot->list.column +
                                                         slot->list.columns * slot->list.page);
            if (GameMode_ck() == 2) {
                userdata_item_give(lobby_world_block, entry->id, -100, 1);
                userdata_item_give(lobby_world_block, slot->item, slot->count, 1);
            } else {
                pl_item_add(slot->plw, entry->id, -100);
                pl_item_add(slot->plw, slot->item, slot->count);
            }
            slot->item = 0;
            slot->state = 0;
            return 1;
        case 2:
            cockpit_msg_slot_choice_a_build(slot, slot->choice_index);
            slot->state = 4;
            slot->choice_index = 0;
            cockpit_msg_slot_choice_a_build(slot, slot->choice_index);
            break;
        }
        break;
    }
    cockpit_msg_slot_rows_build(slot);
    return 1;
}

/* 0x802DF094 (0x378): draws message slot 0's dialog on the full screen: the pouch page, the new item's card or the
 * swap card, the choice window and the prompt. */
void cockpit_msg_slot_draw(u32 idx)
{
    CockpitMsgSlot* slot = cockpit_msg_slots;
    s32 grid;
    s32 box;
    s32 flags;
    s32 card;
    s32 choice;
    u8 prompt;
    _mh_ivec2_ pos;
    _SPR_DATA_* text;

    if (slot->state > 1) {
        set_zmode(0, 0, 0);
        set_blendmode(4, 5, 1);
        grid = 0;
        box = 0;
        flags = 0;
        card = 0;
        choice = 0;
        prompt = 0;
        if (slot->list.step & 0x800) {
            flags |= 8;
        }
        if (slot->list.step & 0x400) {
            flags |= 0x10;
        }
        if (cockpit_msg_slot_box_ck(slot, slot->list.page * slot->list.columns) != 0) {
            box = 1;
            flags |= cockpit_msg_slot_page_usable_ck(slot) != 0 ? 1 : 2;
        }
        switch (slot->state) {
        case 2:
            grid = 1;
            flags |= 0x200;
            card = 1;
            prompt = 0;
            break;
        case 3:
            grid = 1;
            flags |= 0x200;
            card = 1;
            choice = 1;
            prompt = 0;
            break;
        case 4:
            grid = 1;
            card = 2;
            choice = 1;
            prompt = 1;
            break;
        case 5:
            flags |= 0x200;
            grid = 1;
            card = 2;
            choice = 2;
            prompt = 2;
            break;
        }
        if (grid != 0) {
            menu_frame_draw_page(box == 0 ? slot->pouch : slot->box, slot->rows, slot->list.page + 1,
                                 slot->list.pages, slot->fade, flags);
        }
        switch (card) {
        case 1:
            get_lsp_data(818, &pos);
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(52), &pos);
            draw_itemicon_item_id_idx(828, slot->item, &pos);
            draw_font_idx(836, (s8*)ItemName(slot->item), 0, &pos);
            text = get_lsp_data(837, 0);
            font_set_size(text->width, text->height);
            flfntSetColor(text->color);
            font_print_ex(pos.x + text->pos.x, pos.y + text->pos.y, 0, (s8*)"%s", ItemExp(slot->item));
            break;
        case 2:
            get_lsp_data(839, &pos);
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(53), &pos);
            draw_itemicon_item_id_idx(850, slot->item, &pos);
            draw_font_idx(853, (s8*)ItemName(slot->item), 0, &pos);
            menu_item_row_draw_by_lsp(854, (u16*)cockpit_msg_slot_entry_get(slot, slot->list.column +
                                                                                     slot->list.columns *
                                                                                         slot->list.page));
            break;
        }
        if (choice != 0) {
            switch (choice) {
            case 1:
            case 2:
                get_lsp_data(855, &pos);
                menu_choice_window_draw(slot->choice, 2, &pos);
                break;
            }
        }
        get_lsp_data(732, &pos);
        draw_window_frame_preset(pos.x, pos.y, 0);
        font_set_size(16, 16);
        font_print_ex(pos.x + 24, pos.y + 16, 0, (s8*)get_menu_msg(46, prompt));
        font_flush();
    }
}

/* 0x802DF40C (0x2D8): draws message slot `idx`'s dialog in its split-screen half: the pouch page, the choice window
 * and the new item's card, placed relative to the half's anchor. */
void cockpit_msg_slot_split_draw(u32 idx)
{
    _mh_ivec2_ anchor;
    _mh_ivec2_ pos;
    _SPR_DATA_* base;
    _SPR_DATA_* at;
    s16 dx;
    s16 dy;
    u16 id;
    CockpitMsgSlot* slot = &cockpit_msg_slots[(u8)idx];
    s32 grid;
    s32 box;
    s32 flags;
    s32 card;
    u32 choice;

    if (slot->state > 1) {
        set_zmode(0, 0, 0);
        set_blendmode(4, 5, 1);
        grid = 0;
        box = 0;
        flags = 0;
        card = 0;
        choice = 0;
        if (slot->list.step & 0x800) {
            flags |= 8;
        }
        if (slot->list.step & 0x400) {
            flags |= 0x10;
        }
        if (cockpit_msg_slot_box_ck(slot, slot->list.page * slot->list.columns) != 0) {
            box = 1;
            flags |= cockpit_msg_slot_page_usable_ck(slot) != 0 ? 1 : 2;
        }
        switch (slot->state) {
        case 2:
            grid = 1;
            flags |= 0x200;
            card = 1;
            break;
        case 3:
            grid = 1;
            flags |= 0x200;
            card = 1;
            choice = 1;
            break;
        case 4:
            grid = 1;
            card = 2;
            choice = 2;
            break;
        case 5:
            flags |= 0x200;
            grid = 1;
            card = 2;
            choice = 1;
            break;
        }
        get_lsp_data(((const u16*)get_menu_lsp_tbl(361))[(u8)idx], &anchor);
        base = get_lsp_data(3936, 0);
        dx = anchor.x - base->pos.x;
        dy = anchor.y - base->pos.y;
        if (choice == 2) {
            id = (u8)idx == 0 ? 4106 : 4125;
            get_lsp_data(id, &pos);
            menu_choice_window_draw(slot->choice, 2, &pos);
            font_flush();
        }
        if (grid != 0) {
            menu_frame_page_draw(box == 0 ? slot->pouch : slot->box, slot->rows, slot->list.page + 1, slot->list.pages,
                                 slot->fade, flags, idx);
        }
        if (choice == 1) {
            id = (u8)idx == 0 ? 4106 : 4125;
            get_lsp_data(id, &pos);
            menu_choice_window_draw(slot->choice, 2, &pos);
        }
        if (card != 0) {
            id = (u8)idx == 0 ? 4091 : 4123;
            at = get_lsp_data(id, 0);
            pos.x = dx + at->pos.x;
            pos.y = dy + at->pos.y;
            draw_sprite_ary((const u16*)get_menu_lsp_tbl(370), &pos);
            draw_itemicon_item_id_idx(4101, slot->item, &pos);
            draw_font_idx(4105, (s8*)ItemName(slot->item), 0, &pos);
        }
    }
}

/* The notice band (0x806BDCB8). */
CockpitNotice cockpit_notice;

/* The money panel's frame sprites (0x80792648). */
u16 money_frame_ids[] = { 0x018C, 0x018D, 0x018E, 0xFFFF };

/* 0x802DF6E4 (0x74): draws the money panel at layout `id`: the save's zenny and the currency sign. */
s32 menu_money_draw(s32 id)
{
    _mh_ivec2_ pos;
    char text[16];

    get_lsp_data(id, &pos);
    draw_sprite_ary(money_frame_ids, &pos);
    sprintf(text, "%d", ((HudLobbyMoney*)lobby_world_block)->money);
    draw_font_idx(394, (s8*)text, 2, &pos);
    draw_font_idx(395, (s8*)"z", 0, &pos);
}

/* The button guide frame and the per-guide button rows (0x80792658.., 0x805D55F0). */
u16 button_guide_frame_ids[] = { 0x0039, 0x003A, 0x003B, 0xFFFF };
u16 button_guide_ids_0[] = { 0x0031, 0x001F, 0xFFFF };
u16 button_guide_ids_1[] = { 0x0032, 0x0021, 0xFFFF };
u16 button_guide_ids_2[] = { 0x0031, 0x0023, 0xFFFF };
u16 button_guide_ids_3[] = { 0x0031, 0x0024, 0xFFFF };
u16 button_guide_ids_4[] = { 0x0032, 0x0025, 0xFFFF };
u16 button_guide_ids_5[] = { 0x0031, 0x002D, 0xFFFF };
u16 button_guide_ids_6[] = { 0x0034, 0x0028, 0xFFFF };
u16 button_guide_ids_7[] = { 0x0030, 0x002E, 0xFFFF };
u16 button_guide_ids_8[] = { 0x0030, 0x002F, 0xFFFF };
u16 button_guide_ids_9[] = { 0x0035, 0x0028, 0xFFFF };
u16 button_guide_ids_10[] = { 0x0033, 0x002E, 0xFFFF };
u16 button_guide_ids_11[] = { 0x0033, 0x002F, 0xFFFF };
u16 button_guide_ids_12[] = { 0x0036, 0x0025, 0xFFFF };
u16 button_guide_ids_13[] = { 0x0033, 0x001F, 0xFFFF };
u16 button_guide_ids_14[] = { 0x0036, 0x0021, 0xFFFF };
u16 button_guide_ids_15[] = { 0x0033, 0x002D, 0xFFFF };
u16 button_guide_ids_16[] = { 0x0033, 0x0023, 0xFFFF };
u16 button_guide_ids_17[] = { 0x0033, 0x0024, 0xFFFF };
u16 button_guide_ids_18[] = { 0x0038, 0x0029, 0xFFFF };
u16* button_guide_tbl[19] = {
    button_guide_ids_0,  button_guide_ids_1,  button_guide_ids_2,  button_guide_ids_3,  button_guide_ids_4,
    button_guide_ids_5,  button_guide_ids_6,  button_guide_ids_7,  button_guide_ids_8,  button_guide_ids_9,
    button_guide_ids_10, button_guide_ids_11, button_guide_ids_12, button_guide_ids_13, button_guide_ids_14,
    button_guide_ids_15, button_guide_ids_16, button_guide_ids_17, button_guide_ids_18,
};

/* 0x802DF758 (0x74): draws button guide `index` at layout `lsp` unless the pointer page is up. */
void draw_lsp_sprite_variant(u32 lsp, u32 index)
{
    _mh_ivec2_ pos;

    if (chk_pointer() == 0) {
        get_lsp_data(lsp, &pos);
        draw_sprite_ary(button_guide_frame_ids, &pos);
        draw_sprite_ary(button_guide_tbl[(u8)index], &pos);
    }
}

/* 0x802DF7CC (0x68): draws button guide `kind` at `pos` unless the pointer page is up. */
void put_button_guide(s32 kind, const _mh_ivec2_* pos)
{
    if (chk_pointer() == 0) {
        draw_sprite_ary(button_guide_frame_ids, pos);
        draw_sprite_ary(button_guide_tbl[(u8)kind], pos);
    }
}

/* 0x802DF834 (0x118): draws button guide `guide` right-aligned to `pos` (moved left by the frame's width). */
void put_button_guide_r(u8 guide, _mh_ivec2_* pos)
{
    u16* id;
    s32 width = 0;
    s16 height = 0;
    _mh_ivec2_ at;
    s16 w;

    for (id = button_guide_frame_ids; *id != 0xFFFF; id++) {
        HudSprWords spr = *(HudSprWords*)get_lsp_data(*id, 0);

        w = spr.spr.width;
        w = w < 0 ? -w : w;
        width += w;
        height = spr.spr.height;
    }
    at.x = pos->x - width;
    at.y = pos->y + height;
    draw_sprite_ary(button_guide_frame_ids, &at);
    draw_sprite_ary(button_guide_tbl[guide], &at);
}

/* The second guide set (0x805D563C.., 0x805D56B4, 0x807926F8..0x80792708). */
u16 button_guide_b_ids_3[] = { 0x004A, 0x003F, 0x004D, 0x0047, 0xFFFF };
u16 button_guide_b_ids_4[] = { 0x004A, 0x0043, 0x004D, 0x0047, 0xFFFF };
u16 button_guide_b_ids_5[] = { 0x004A, 0x0042, 0x004D, 0x0047, 0xFFFF };
u16 button_guide_b_ids_6[] = { 0x004B, 0x0040, 0x004E, 0x0044, 0xFFFF };
u16 button_guide_b_ids_7[] = { 0x004B, 0x0041, 0x004E, 0x0044, 0xFFFF };
u16 button_guide_b_ids_8[] = { 0x004C, 0x0040, 0x004E, 0x0044, 0xFFFF };
u16 button_guide_b_ids_9[] = { 0x004C, 0x0041, 0x004E, 0x0044, 0xFFFF };
u16 button_guide_b_ids_10[] = { 0x004C, 0x0043, 0x004E, 0x0047, 0xFFFF };
u16 button_guide_b_ids_11[] = { 0x004C, 0x0042, 0x004E, 0x0047, 0xFFFF };
u16 button_guide_b_ids_12[] = { 0x004C, 0x003F, 0x004E, 0x0047, 0xFFFF };
u16 button_guide_b_frame_ids[] = { 0x0052, 0x0053, 0x0054, 0xFFFF };
u16 button_guide_b_ids_0[] = { 0xFFFF };
u16 button_guide_b_ids_1[] = { 0xFFFF };
u16 button_guide_b_ids_2[] = { 0xFFFF };
u16* button_guide_b_tbl[13] = {
    button_guide_b_ids_0, button_guide_b_ids_1, button_guide_b_ids_2,  button_guide_b_ids_3,  button_guide_b_ids_4,
    button_guide_b_ids_5, button_guide_b_ids_6, button_guide_b_ids_7,  button_guide_b_ids_8,  button_guide_b_ids_9,
    button_guide_b_ids_10, button_guide_b_ids_11, button_guide_b_ids_12,
};

/* 0x802DF94C (0x58): draws second-set guide `index` at layout `lsp`. */
void put_button_guide_b_idx(u16 lsp, u8 index)
{
    _mh_ivec2_ pos;

    if (chk_pointer() == 0) {
        get_lsp_data(lsp, &pos);
        put_button_guide_b(index, &pos);
    }
}

/* 0x802DF9A4 (0x68): draws second-set guide `kind` at `pos` unless the pointer page is up. */
void put_button_guide_b(s32 kind, const _mh_ivec2_* pos)
{
    if (chk_pointer() == 0) {
        draw_sprite_ary(button_guide_b_frame_ids, pos);
        draw_sprite_ary(button_guide_b_tbl[(u8)kind], pos);
    }
}

/* The third guide set (0x80792710..0x807927A8, 0x805D56E8). */
u16 button_guide_c_frame_ids[] = { 0x006A, 0x006B, 0x006C, 0xFFFF };
u16 button_guide_c_ids_0[] = { 0x0066, 0x005C, 0xFFFF };
u16 button_guide_c_ids_1[] = { 0x0066, 0x005D, 0xFFFF };
u16 button_guide_c_ids_2[] = { 0x0064, 0x0056, 0xFFFF };
u16 button_guide_c_ids_3[] = { 0x0064, 0x0058, 0xFFFF };
u16 button_guide_c_ids_4[] = { 0x0064, 0x0057, 0xFFFF };
u16 button_guide_c_ids_5[] = { 0x0064, 0x0059, 0xFFFF };
u16 button_guide_c_ids_6[] = { 0x0064, 0x005B, 0xFFFF };
u16 button_guide_c_ids_7[] = { 0x0066, 0x005E, 0xFFFF };
u16 button_guide_c_ids_8[] = { 0x0067, 0x005E, 0xFFFF };
u16 button_guide_c_ids_9[] = { 0x0068, 0x005C, 0xFFFF };
u16 button_guide_c_ids_10[] = { 0x0068, 0x005D, 0xFFFF };
u16 button_guide_c_ids_11[] = { 0x0065, 0x0056, 0xFFFF };
u16 button_guide_c_ids_12[] = { 0x0065, 0x0058, 0xFFFF };
u16 button_guide_c_ids_13[] = { 0x0065, 0x0057, 0xFFFF };
u16 button_guide_c_ids_14[] = { 0x0065, 0x0059, 0xFFFF };
u16 button_guide_c_ids_15[] = { 0x0065, 0x005B, 0xFFFF };
u16 button_guide_c_ids_16[] = { 0x0068, 0x0062, 0xFFFF };
u16 button_guide_c_ids_17[] = { 0x0069, 0x0063, 0xFFFF };
u16 button_guide_c_ids_18[] = { 0x0065, 0x0063, 0xFFFF };
u16* button_guide_c_tbl[19] = {
    button_guide_c_ids_0,  button_guide_c_ids_1,  button_guide_c_ids_2,  button_guide_c_ids_3,
    button_guide_c_ids_4,  button_guide_c_ids_5,  button_guide_c_ids_6,  button_guide_c_ids_7,
    button_guide_c_ids_8,  button_guide_c_ids_9,  button_guide_c_ids_10, button_guide_c_ids_11,
    button_guide_c_ids_12, button_guide_c_ids_13, button_guide_c_ids_14, button_guide_c_ids_15,
    button_guide_c_ids_16, button_guide_c_ids_17, button_guide_c_ids_18,
};

/* 0x802DFA0C (0x58): draws third-set guide `index` at layout `lsp`. */
void put_button_guide_c_idx(u32 lsp, u32 index)
{
    _mh_ivec2_ pos;

    if (chk_pointer() == 0) {
        get_lsp_data(lsp, &pos);
        put_button_guide_c((u8)index, &pos);
    }
}

/* 0x802DFA64 (0x68): draws third-set guide `kind` at `pos` unless the pointer page is up. */
void put_button_guide_c(s32 kind, const _mh_ivec2_* pos)
{
    if (chk_pointer() == 0) {
        draw_sprite_ary(button_guide_c_frame_ids, pos);
        draw_sprite_ary(button_guide_c_tbl[(u8)kind], pos);
    }
}

/* 0x802DFACC (0x4C): string `id` of string table 33 (the monster names; 0 past the 41st). */
s8* str_tbl_33_get(u8 id)
{
    u8** tbl = get_str_tbl(33);

    if (id >= 41) {
        id = 0;
    }
    return (s8*)tbl[id];
}

/* 0x802DFB18 (0x4C): string `id` of string table 34 (0 past the 10th). */
s8* str_tbl_34_get(u8 id)
{
    u8** tbl = get_str_tbl(34);

    if (id >= 10) {
        id = 0;
    }
    return (s8*)tbl[id];
}

/* 0x802DFB64 (0xC): shows the quest notice. */
void cockpit_notice_quest_show(void)
{
    cockpit_notice_show(0, 1);
}

/* 0x802DFB70 (0x34): shows the new-mail notice, steps the mailbox list and plays sound 39. */
void cockpitShowNewMail(void)
{
    cockpit_notice_show(1, 1);
    mailbox_row_offset_inc();
    sysSE_req(39);
}

/* 0x802DFBA4 (0x44): shows the note notice in the lobby once the game is ready. */
void cockpit_notice_note_show(void)
{
    if (GameMode_ck() == 2 && game_ready_ck() == 1) {
        cockpit_notice_show(2, 1);
    }
}

/* 0x802DFBE8 (0x34): keeps the lobby notice up this frame (shown when it was not). */
void cockpit_lobby_reset(void)
{
    cockpit_notice.frame_flags |= 8;
    if (!(cockpit_notice.active & 8)) {
        cockpit_notice_show(3, 1);
    }
}

/* 0x802DFC1C (0x50): shows notice `kind` (`fresh` 1 plays its pop-in) and restarts its timer. */
void cockpit_notice_show(u8 kind, u32 fresh)
{
    CockpitNotice* notice = &cockpit_notice;
    u8 bit = 1 << kind;

    notice->active |= bit;
    if (fresh == 1) {
        notice->fresh |= bit;
    }
    notice->timer[kind] = 0;
}

/* 0x802DFC6C (0x8): hides the quest notice. */
void cockpit_notice_quest_clear(void)
{
    cockpit_notice_clear(0);
}

/* 0x802DFC74 (0x8): hides the new-mail notice. */
void hud_mail_notice_clear(void)
{
    cockpit_notice_clear(1);
}

/* 0x802DFC7C (0x8): hides the note notice. */
void ainpc_page_mode2_set(void)
{
    cockpit_notice_clear(2);
}

/* 0x802DFC84 (0x8): hides the lobby notice. */
void cockpit_notice_lobby_clear(void)
{
    cockpit_notice_clear(3);
}

/* 0x802DFC8C (0x48): hides notice `kind`. */
void cockpit_notice_clear(u8 kind)
{
    s32 mask = ~(1 << kind);
    CockpitNotice* notice = &cockpit_notice;

    notice->active &= (u8)mask;
    notice->fresh &= (u8)mask;
    notice->timer[kind] = 0;
}

/* 0x802DFCD4 (0x64): clears the notices and raises the note and new-mail notices that are pending. */
void cockpit_notice_init(void)
{
    memset(&cockpit_notice, 0, 0xE);
    if ((u32)note_timer_lobby_ready_ck() == 1) {
        cockpit_notice_note_show();
    }
    if (game_ready_ck() == 1 && (u32)mailbox_unread_ck() == 1) {
        cockpit_notice_show(1, 1);
    }
}

/* 0x802DFD38 (0x184): runs the notices' timers: a fresh notice pops in over 60 frames, the others slide in turn. */
void cockpit_notice_update(void)
{
    CockpitNotice* notice = &cockpit_notice;
    s32 ofs;
    s32 bit;
    s32 i;

    if (!(cockpit_notice.frame_flags & 8)) {
        cockpit_notice_lobby_clear();
    }
    notice->frame_flags = 0;
    if (notice->active != 0) {
        for (i = 0, ofs = 0; i < 4; i++) {
            bit = 1 << i;
            if (notice->active & bit) {
                if (notice->fresh & bit) {
                    notice->timer[i] += 2;
                    if (notice->timer[i] > 121) {
                        notice->fresh &= ~bit;
                        notice->clock = 0;
                    }
                } else if (notice->fresh != 0) {
                    notice->timer[i] = 20;
                } else {
                    notice->timer[i] = notice->clock + ofs;
                }
                ofs += 10;
            }
        }
        notice->clock++;
    }
}

/* 0x802DFEBC (0xA8): draws the shown notices. */
void draw_lsp_parts(void)
{
    CockpitNotice* notice = &cockpit_notice;

    set_blendmode(4, 5, 1);
    if (notice->active & 1) {
        cockpit_notice_quest_draw(notice->timer[0]);
    }
    if (notice->active & 2) {
        cockpit_notice_icon_draw(1, notice->timer[1]);
    }
    if (notice->active & 4) {
        cockpit_notice_icon_draw(2, notice->timer[2]);
    }
    if (notice->active & 8) {
        cockpit_notice_lobby_draw(0, notice->timer[3]);
    }
}

/* 0x802DFF64 (0x94): draws the mail (1) or note (2) notice at frame `frame` in the ready lobby. */
void cockpit_notice_icon_draw(u8 kind, s16 frame)
{
    _mh_ivec2_ pos;

    get_lsp_data(405, &pos);
    if (GameMode_ck() == 2 && game_ready_ck() == 1) {
        if (kind == 1) {
            draw_sprite_anim_idx(406, frame, &pos);
        }
        if (kind == 2) {
            draw_sprite_anim_idx(407, frame, &pos);
        }
    }
}

/* 0x802DFFF8 (0x54): draws the lobby notice at frame `frame`. */
void cockpit_notice_lobby_draw(u8 kind, s16 frame)
{
    _mh_ivec2_ pos;

    if (GameMode_ck() == 2) {
        get_lsp_data(408, &pos);
        draw_sprite_anim_idx(409, frame, &pos);
    }
}

/* 0x802E004C (0x58): draws the quest notice at frame `frame` (its lobby or quest anchor). */
void cockpit_notice_quest_draw(s16 frame)
{
    _mh_ivec2_ pos;

    get_lsp_data(GameMode_ck() == 2 ? 410 : 413, &pos);
    draw_sprite_anim_idx(411, frame, &pos);
}

/* 0x802E00A4 (0x330): draws a player's name tag over their head (in their colour, or by `rank`), with the arrow and
 * the icon `icon` beside it.  Returns the tag height (-18), or 0 when the player is not shown. */
s32 draw_lsp_element(_PLW* plw, u8 icon, u8 arrow, u16 rank)
{
    s32 camera;
    VEC3 screen;
    VEC3 world;
    s32 x;
    s32 y;
    s8* name;
    s16 half;
    _SPR_DATA_ spr;
    s16 uv0[2];
    s16 uv1[2];
    _mh_ivec2_ pos;
    s32 handle;
    u32 color;

    handle = scn_root_get_current_camera(pRoot);
    word_copy_return_dst(&camera, &handle);
    VEC3_ctor(&screen);
    VEC3_ctor(&world);
    if (game_ready_ck() == 0) {
        return 0;
    }
    if (plw->slot_active == 0) {
        return 0;
    }
    if (pl_eft_draw_ck(plw) == 0) {
        return 0;
    }
    if (plw->area_0x16 != get_now_areano()) {
        return 0;
    }
    chara_icon_pos_get(plw, 2, &world);
    camera_world_to_screen((nw4r::g3d::Camera*)&camera, (u8*)&screen, &world);
    if (screen.z < 0.0f || screen.z > 1.0f) {
        return 0;
    }
    x = screen.x * hud_screen_width_get();
    y = 1.0714285f * screen.y;
    if (get_option_cfg(12) == 0) {
        name = (s8*)plw->hunter_0x5CA.name_0x5CA;
    } else {
        name = (s8*)plw->hunter_0x5CA.id_0x5DB;
    }
    half = (flfntStrLen((char*)name) * 18) / 4;
    if (arrow != 0xFF) {
        spr_data_copy(&spr, get_lsp_data(6326, 0));
        atlas_icon_uv_get(&spr.uv0, &spr.uv1, plw->field_0x002);
        spr.pos.y = 0;
        spr.pos.x = 0;
        pos.x = x - half - spr.width - 2;
        pos.y = (spr.height - 18) / 2 + (y - spr.height);
        draw_sprite(spr, &pos);
    }
    pos.x = x - half;
    pos.y = y - 18;
    font_set_size(18, 18);
    flfntSetColor(0xFF);
    font_locate(pos.x + 2, pos.y + 1);
    flfntFontPuts((char*)name);
    if (rank == 0xFFFF) {
        color = player_color_get(plw->chunk_ofs);
    } else if (rank < 31) {
        color = 0xF0F0F0FF;
    } else {
        color = rank < 999 ? 0xE8A40FFF : 0xF53737FF;
    }
    flfntSetColor(color);
    font_locate(pos.x, pos.y);
    flfntFontPuts((char*)name);
    if (icon != 0xFF && icon != 0) {
        spr_data_copy(&spr, get_lsp_data(317, 0));
        pos.x = x + half + 4;
        pos.y = (spr.height - 18) / 2 + (y - spr.height);
        draw_tex_icon_square(pos.x, pos.y, spr.width, spr.color, chara_icon_tex_tbl[icon]);
    }
    font_flush();
    return -18;
}

/* The tag heights above the head joint (0x805D5734). */
f32 chara_icon_height_tbl[3] = { 90.0f, 84.0f, 78.0f };

/* 0x802E03D4 (0x94): the world point of `plw`'s name tag: its head joint, raised by height `kind`. */
void chara_icon_pos_get(_PLW* plw, u8 kind, VEC3* out)
{
    VEC3 joint;

    copyVec3(out, &plw->vec_0x03C);
    VEC3_ctor(&joint);
    ((HudPlwJoints*)plw->physics_0x13C)->chr_0x04.get_joint_wpos(14, &joint);
    copyVec3(out, &joint);
    out->y += chara_icon_height_tbl[kind];
}

/* 0x802E0468 (0x18): sets player slot `slot`'s transfer-mode icon. */
#pragma peephole on
void setCockpitTransferMode(u8 slot, s32 mode)
{
    cockpit_state.transfer_mode_0x07C[slot] = mode;
}
#pragma peephole off
