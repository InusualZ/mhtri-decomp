/* lobby/lb_quest_ui.cpp - the lobby kitchen screen, the trade screen and the scene effect.
 * RANGE. .text 0x8038F2BC-0x80394158 (55 functions); .data 0x805F0CB8-0x805F1400 (ends with the camera key table),
 *   .sdata 0x807933D8-0x80793470, .sdata2 0x8079C2A8-0x8079C2D0, extab 0x800181F4-0x80018344, extabindex
 *   0x80038154-0x8003834C.  Three groups in address order: the kitchen screen (lobby
 *   screen 0x11, to 0x803928F0), the trade screen (screen 0x12, to 0x80393994) and the scene effect (effect 0x36),
 *   whose tail is `lb_scene_model_slide` and the two helpers after it (0x80394038-0x80394158).
 * FLAGS. `cflags_lobby`; `#pragma pool_data off` (retail loads every table with its own `lis`/`addi`); `#pragma peephole off` file-wide (retail keeps `clrlwi`/`rlwinm` + `cmpwi` and `clrlwi` +
 *   `slwi` unfused; every written row measured better with it off); `#pragma optimization_level 4` around
 *   `lb_trade_msg_draw` (retail's subi + cmplwi switch-range lowering, playbook 108).
 * NAMES. `lb_quest_ui` is the registered GUESS; the groups' names (`lb_kitchen_*`, `lb_trade_*`,
 *   `lb_scene_eft_*`) and every data name are GUESSes from the bodies: the kitchen pays zenny or resource points for a
 *   pair of ingredients (`lb_kitchen_pair_find` keys the pair tables on the two groups) and fills `lb_param_w`'s meal
 *   skills; the trade screen exchanges `NetCtrlWk::getServerNotice`'s offers.  `lb_kitchen_idle_ck` answers whether
 *   the kitchen is outside its menu states 2..4.
 *   GUESS: `lb_kitchen_tri_sum`, `lb_kitchen_pair_index`, `lb_kitchen_pair_seen_set`,
 *   GUESS: `lb_kitchen_pair_seen_ck`, `lb_kitchen_close`, `lb_kitchen_page_set`, `lb_kitchen_row_copy`,
 *   GUESS: `lb_kitchen_course_ck`, `lb_kitchen_pick_reset`, `lb_kitchen_pick_input`,
 *   GUESS: `lb_kitchen_list_page_set`, `lb_kitchen_special_open`, `lb_kitchen_pair_find`,
 *   GUESS: `lb_kitchen_skill_apply`, `lb_kitchen_special_apply`, `lb_kitchen_meal_serve`,
 *   GUESS: `lb_kitchen_frame_draw`, `lb_kitchen_list_draw`, `lb_kitchen_slot_pulse_draw`,
 *   GUESS: `lb_kitchen_skill_text`, `lb_kitchen_pick_draw`, `lb_kitchen_special_pick_draw`,
 *   GUESS: `lb_kitchen_confirm_draw`, `lb_kitchen_pick_screen_draw`, `lb_kitchen_special_list_draw`,
 *   GUESS: `lb_kitchen_msg_draw`, `lb_kitchen_draw_task`, `lb_kitchen_idle_ck`, `lb_trade_open`,
 *   GUESS: `lb_trade_close`, `lb_trade_page_set`, `lb_trade_exchange`, `lb_trade_step`, `lb_trade_frame_draw`,
 *   GUESS: `lb_trade_list_draw`, `lb_trade_cost_draw`, `lb_trade_msg_draw`, `lb_trade_draw_task`,
 *   GUESS: `lb_scene_eft_spawn`, `lb_scene_eft_release`, `lb_scene_eft_step`, `lb_scene_model_slide`,
 *   GUESS: `lb_quest_board_state_next`, `lb_quest_board_effect_retire`
 *   GUESS: `lb_kitchen_open`, `lb_kitchen_courses_roll`, `lb_kitchen_special_input`,
 *   GUESS: `lb_kitchen_bonus_roll`, `lb_kitchen_extra_roll`, `lb_kitchen_step`, `lb_kitchen_course_draw`,
 *   GUESS: `lb_kitchen_special_screen_draw`, `lb_trade_offer_state`, `lb_scene_eft_init`, `lb_scene_eft_move`
 * RESIDUALS. Every row is written.
 *  - `lb_kitchen_open`: retail keeps two row counters for the rolled pairs (one byte offset, one index) and saves two
 *    more registers (`_savegpr_25`/`_restgpr_25` against our `_savegpr_27`/`_restgpr_27`);
 *  - `lb_kitchen_list_page_set`: retail steps a dead per-entry counter (`addi r7`) inside the 3x-unrolled copy;
 *  - `lb_kitchen_courses_roll`, `lb_kitchen_step`: register order in the loops (retail gives the loop index r31/r27);
 *  - `lb_scene_eft_move`: retail schedules the 1.0f `lfs` after the area byte's `lbz`, and its pool word is named by
 *    the map (`lbl_8079C2AC`) where ours is anonymous;
 *  - `lb_trade_offer_state`: retail does not sign-extend `eft052_page_count_ck`'s result (`ef/eft052.h` declares it
 *    `s16`; its body returns `item_slots_room_get`'s value unextended);
 *  - `lb_trade_list_draw`: register order, and retail reloads `choice.cursor` for the pouch test;
 *  - `lb_trade_step`: retail truncates `toggle_word_step`'s last argument (`clrlwi r7`), a `u16` parameter in the
 *    shared header our declaration types wider;
 *  - `lb_kitchen_meal_serve`: the two rare bytes load in the other order and one `lhz` is scheduled after the
 *    `lb_param_w` address;
 *  - `lb_kitchen_skill_text`: retail keeps the comparison limit in r3; ours (`limit = size`, the better of the two
 *    spellings) keeps it in r31;
 *  - `lb_kitchen_pick_draw`: retail adds the 8 into r5 directly, ours through r0;
 *  - `lb_kitchen_skill_apply`: retail truncates `skill` in place (r4), ours into r5;
 *  - `lb_kitchen_skill_text`, `lb_kitchen_pick_draw`, `lb_trade_cost_draw`: their format strings are literals, so
 *    their relocations name our anonymous string symbols where retail's name the map's labels
 *    (`lb_kitchen_skill_none_fmt`, `lb_kitchen_skill_value_fmt`, `lb_kitchen_text_fmt`, `lb_kitchen_cost_fmt`,
 *    `lb_kitchen_skill1_fmt`, `lb_kitchen_skill2_fmt`, `lb_trade_count_fmt`) (same bytes, same addresses); named
 *    `aligned(4)` arrays keep the bytes but cost `lb_kitchen_pick_draw` 2.5 points of register colouring, and an
 *    all-zero `""` array lands in `.sbss`.
 *   flipcheck: `.data`/`.sdata` are emitted byte-identical (tables defined before their first users, strings as
 *   literals); the NPC talk program 0x8038EC44-0x8038F2BC is `lobby/lb_note_talk.cpp` (retail pools 1.0f once per
 *   unit: 0x8079C2A4 for the talk program, 0x8079C2AC for the scene effect); `.text`/extab/extabindex short of the claim.
 * SHAPES. The list draws index their layout tables (`lb_kitchen_row_lsp[i]`) instead of stepping a pointer, which
 *   orders retail's induction registers; locals that retail colours low (`active`, `str`, `filling`) are declared
 *   last. `lb_trade_cost_draw` reaches price `i` as notice word `i + 1` of the offer, the evaluation order retail's
 *   `add` shows (`&offer->costs[i]` measures lower). `lb_kitchen_skill_text`'s non-numeric kinds return from the
 *   switch and share the strong/weak tail after it, which gives case 4 its own `li r3,5; b`.
 */

#pragma pool_data off
#pragma peephole off

#include "types.h"
#include "lobby/lb_quest_ui.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "MSL_C/alloc.h"
#include "ef/fn_800CDB2C.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "ef/eft052.h"
#include "ef/system_core.h"
#include "sound/fn_800DD1F0.h"
#include "sound/fn_800D7F54.h"
#include "sound/set_zmode__FbUcb.h"
#include "sound/se_req.h"
#include "enemy/em020_prog.h"
#include "lobby/lobby_w.h"
#include "lobby/lb_cmd_pressed_ck.h"
#include "lobby/LbStr.h"
#include "lobby/lb_panel_msg_draw.h"
#include "lobby/lb_talk_page_open.h"
#include "lobby/lb_npc.h"
#include "lobby/lb_server_sel_trans.h"
#include "lobby/fn_801E7530.h"
#include "lobby/lb_unlock_cond_ck.h"
#include "fn_80047398/lobby_world_block.h"
#include "fn_80047398.h"
#include "mh3_pad/lb_param_w.h"
#include "mh3_pad/vec3.h"
#include "quest/quest_types.h"
#include "hud/layout.h"
#include "hud/cockpit.h"
#include "font/flfnt.h"
#include "menu/menu_message.h"
#include "menu/ItemExp.h"
#include "menu/menu_row.h"
#include "camera/camera.h"
#include "camera/camera_frame_get.h"
#include "Pl/pl_act_stage_latch_set.h"
#include "Pl/pl_skill.h"
#include "Pl/pl_item_add.h"
#include "Pl/fn_80262940.h"
#include "Pl/pl_coll.h"
#include "Pl/fn_802693C4.h"
#include "Pl/pl_act_step_data.h"
#include "fn_8004CAD8.h"
#include "get_FqResult_work.h"
#include "userdata_item.h"
#include "main.h"
#include "stage/stg_w.h"
#include "Network/network_pat_control.h"
#include "sound/mhchar.h"
#include "stage/get_now_mapno.h"
#include "mh3_pad/system_w.h"
#include "g3d/g3d_calcworld.h"
#include "ef/eft028_set_scaled.h"



/* The model record `lb_scene_eft_move` hands `lb_scene_model_slide`: its step byte and the position the
 * camera keys drive.  size: 0x10 (a view: the model continues) */
typedef struct LbQuestDetailModel {
    /* +0x00 */ u8 step;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ VEC3 pos;
    /* +0x10 */ u8 pad_0x10[0x24];
    /* +0x34 */ u8 visible;       /* `MHchar::field_0x34`: 0 hides the model and stops its step */
} LbQuestDetailModel; /* size: 0x35 (a view of the model's first bytes) */

/* One emitter of a scene model's 0xFF-ended table (`lb_scene_emitter_tbls`): the effect kind, its position, its
 * heading and the base and spread of its random interval in frames. size: 0x18 */
typedef struct LbSceneEmitter {
    /* +0x00 */ u8 kind;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ Vec pos;
    /* +0x10 */ u16 heading;
    /* +0x12 */ s16 base;
    /* +0x14 */ s16 spread;
    /* +0x16 */ u8 pad_0x16[0x2];
} LbSceneEmitter;


extern "C" {

/* The unit's own entry points the bodies below reach before their definitions (or that are unwritten). */
void lb_kitchen_page_set(LbKitchenWork* work, s16 page);
void lb_kitchen_row_copy(LbKitchenRow* dst, const LbKitchenRow* src);
void lb_kitchen_courses_roll(LbKitchenWork* work, s16 course);
void lb_kitchen_list_page_set(LbKitchenWork* work, s16 page);
u16* lb_kitchen_pair_find(u8 kind, u8 a, u8 b);
u16 lb_kitchen_bonus_roll(LbKitchenWork* work, u8 chance);
u16 lb_kitchen_extra_roll(LbKitchenWork* work, u8 chance);
void lb_kitchen_special_apply(LbKitchenWork* work);
void lb_kitchen_list_draw(LbKitchenWork* work);
void lb_kitchen_pick_draw(LbKitchenWork* work);
void lb_kitchen_course_draw(LbKitchenWork* work, bool active);
void lb_kitchen_confirm_draw(LbKitchenWork* work);
void lb_kitchen_special_screen_draw(LbKitchenWork* work);
u8 lb_trade_offer_state(LbTradeOffer* offer, s16 dest);
void lb_trade_page_set(LbTradeWork* work, s16 page);
void lb_trade_draw_task(void);
void lb_kitchen_draw_task(void);
void lb_scene_eft_release(_EFT* self);
void lb_scene_eft_step(_EFT* self);
void lb_scene_eft_init(_EFT* self);
void lb_scene_eft_move(_EFT* self);
u8 lb_scene_model_slide(_EFT* self, LbQuestDetailModel* model, u8 index);
void lb_quest_board_state_next(_EFT* self);
void lb_quest_board_effect_retire(_EFT* self);

/* --------------------------------------------------------------------------------------------- *
 * The kitchen screen.
 * --------------------------------------------------------------------------------------------- */

/* The kitchen's tables. */
s32 lb_kitchen_cost_tbl[6] = {50, 5, 150, 15, 300, 30};
u8 lb_kitchen_special_tbl[12] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x00, 0x00};
s16 lb_kitchen_special_values[11] = {0, 50, 300, 50, 5, 15, 5, 5, 5, 5, 5};
s16 lb_kitchen_bonus_tbl_0[10] = {1, 30, 4, 3, 5, 5, 3, -30, 2, -150};
s16 lb_kitchen_bonus_tbl_1[10] = {1, 40, 4, 3, 5, 10, 3, -30, 2, -150};
s16 lb_kitchen_bonus_tbl_2[10] = {1, 50, 4, 5, 5, 15, 3, -30, 2, -150};
s16* lb_kitchen_bonus_tbls[3] = {lb_kitchen_bonus_tbl_0, lb_kitchen_bonus_tbl_1, lb_kitchen_bonus_tbl_2};
u16 lb_kitchen_pairs_0[66] = {0x0000, 0x0002, 0x0074, 0x0001, 0x0003, 0x0067, 0x0002, 0x0001, 0x0066, 0x0003, 0x0001, 0x0066, 0x0004, 0x0004, 0x0077, 0x0005, 0x0005, 0x0072, 0x0101, 0x0006, 0x006F, 0x0102, 0x0007, 0x006B, 0x0103, 0x0008, 0x006E, 0x0104, 0x0009, 0x0086, 0x0105, 0x000A, 0x0068, 0x0202, 0x000B, 0x006A, 0x0203, 0x000C, 0x008A, 0x0204, 0x000D, 0x0071, 0x0205, 0x0001, 0x0066, 0x0303, 0x000E, 0x0087, 0x0304, 0x0001, 0x0066, 0x0305, 0x000F, 0x0087, 0x0404, 0x0010, 0x0070, 0x0405, 0x0011, 0x0086, 0x0505, 0x8000, 0x008F, 0xFFFF, 0x0001, 0x0066};
u16 lb_kitchen_pairs_1[66] = {0x0000, 0x0012, 0x0074, 0x0001, 0x0013, 0x0078, 0x0002, 0x0014, 0x006D, 0x0003, 0x0001, 0x0066, 0x0004, 0x0015, 0x0077, 0x0005, 0x0016, 0x0072, 0x0101, 0x0017, 0x006F, 0x0102, 0x0018, 0x006B, 0x0103, 0x0008, 0x006E, 0x0104, 0x0019, 0x008E, 0x0105, 0x001A, 0x0073, 0x0202, 0x001B, 0x006A, 0x0203, 0x001C, 0x0088, 0x0204, 0x001D, 0x0071, 0x0205, 0x0001, 0x0066, 0x0303, 0x001E, 0x008B, 0x0304, 0x0001, 0x0066, 0x0305, 0x001F, 0x0088, 0x0404, 0x0020, 0x008B, 0x0405, 0x0001, 0x0066, 0x0505, 0x8001, 0x008F, 0xFFFF, 0x0001, 0x0066};
u16 lb_kitchen_pairs_2[66] = {0x0000, 0x0021, 0x007C, 0x0001, 0x0022, 0x007E, 0x0002, 0x0023, 0x007A, 0x0003, 0x0024, 0x007F, 0x0004, 0x0025, 0x0081, 0x0005, 0x0026, 0x0072, 0x0101, 0x0027, 0x007B, 0x0102, 0x0028, 0x0076, 0x0103, 0x0029, 0x0085, 0x0104, 0x002A, 0x0089, 0x0105, 0x002B, 0x0084, 0x0202, 0x002C, 0x0083, 0x0203, 0x002D, 0x0082, 0x0204, 0x002E, 0x0080, 0x0205, 0x0001, 0x0066, 0x0303, 0x002F, 0x007B, 0x0304, 0x0030, 0x008C, 0x0305, 0x0031, 0x008D, 0x0404, 0x0032, 0x007D, 0x0405, 0x0033, 0x008C, 0x0505, 0x8002, 0x008F, 0xFFFF, 0x0001, 0x0066};
u16* lb_kitchen_pair_tbls[3] = {lb_kitchen_pairs_0, lb_kitchen_pairs_1, lb_kitchen_pairs_2};
u16 lb_kitchen_area_codes[4] = {0x0003, 0x0006, 0x0008, 0x0000};
u8 lb_kitchen_courses_0[12] = {0x09, 0x0B, 0x15, 0x16, 0x18, 0x1F, 0x20, 0x22, 0x2A, 0x00, 0x00, 0x00};
u8 lb_kitchen_courses_1[16] = {0x08, 0x0A, 0x0D, 0x14, 0x17, 0x1D, 0x21, 0x23, 0x24, 0x27, 0x2E, 0x2F, 0x30, 0x00, 0x00, 0x00};
u8 lb_kitchen_courses_2[16] = {0x03, 0x05, 0x07, 0x0C, 0x0E, 0x13, 0x1C, 0x1E, 0x25, 0x26, 0x2B, 0x2D, 0x31, 0x00, 0x00, 0x00};
u8 lb_kitchen_courses_3[12] = {0x04, 0x06, 0x0F, 0x12, 0x19, 0x1A, 0x1B, 0x28, 0x2C, 0x32, 0x00, 0x00};
u8 lb_kitchen_courses_4[8] = {0x01, 0x02, 0x10, 0x11, 0x29, 0x00, 0x00, 0x00};
u8* lb_kitchen_course_lists[5] = {lb_kitchen_courses_0, lb_kitchen_courses_1, lb_kitchen_courses_2, lb_kitchen_courses_3,
                                    lb_kitchen_courses_4};
LbChoiceDef lb_kitchen_choice_defs[3] = {
    {2, 0, 0, 0x11, 0x166, 0, 0, 3},
    {2, 0, 0, 0x13, 0x168, 0, 0, 3},
    {2, 0, 0, 0x15, 0x16A, 0, 0, 3},
};
u16 lb_kitchen_special_holds[4] = {0x01AF, 0x01B0, 0x0000, 0x0000};

/* 0x8038F2BC (0x88): The triangular number below a count - how many pair keys the lower groups take.  The second
 * argument is part of the retail call shape and unused here. */
s32 lb_kitchen_tri_sum(s32 count) {
    s32 sum = 0;
    s32 value = count - 1;
    u32 i;
    for (i = (u16)(count - 1); i != 0; i--) {
        sum += value;
        value--;
    }
    return sum;
}

/* 0x8038F344 (0x54): The pair's bit index: the smaller ingredient plus the keys the larger one's pairs start at. */
u16 lb_kitchen_pair_index(u16 a, u16 b) {
    u16 low;

    if (a < b) {
        low = a;
    } else {
        low = b;
        b = a;
    }
    return low + lb_kitchen_tri_sum(b);
}

/* 0x8038F398 (0x9C): Marks the pair as tried - in the offline set while the game is not online, in the online one
 * afterwards. */
void lb_kitchen_pair_seen_set(u16 a, u16 b) {
    u16 key = lb_kitchen_pair_index(a, b);
    if (game_ready_ck() == 0) {
        Q_UserData* user = (Q_UserData*)lobby_world_block;
        user->kitchen_pairs_0x516C[(u32)key >> 3] |= 1 << (key & 7);
    } else {
        Q_UserData* user = (Q_UserData*)lobby_world_block;
        user->kitchen_pairs_0x525C[(u32)key >> 3] |= 1 << (key & 7);
    }
}

/* 0x8038F434 (0xA4): Whether the pair has been tried. */
int lb_kitchen_pair_seen_ck(u16 a, u16 b) {
    u16 key = lb_kitchen_pair_index(a, b);
    if (game_ready_ck() == 0) {
        Q_UserData* user = (Q_UserData*)lobby_world_block;
        return (user->kitchen_pairs_0x516C[(u32)key >> 3] & (1 << (key & 7))) != 0;
    }
    Q_UserData* user = (Q_UserData*)lobby_world_block;
    return (user->kitchen_pairs_0x525C[(u32)key >> 3] & (1 << (key & 7))) != 0;
}

/* 0x8038F4D8 (0x270): Opens the kitchen for the cook `npc`: clears the screen block, works out the online meal's
 * area, clears the received meal, picks the tier from the unlocks and rolls the twelve ingredients from the seed. */
void lb_kitchen_open(_PLW* npc, s32 arg) {
    LbKitchenWork* work = (LbKitchenWork*)lobby_w.menu_0xAC;
    u32 code;
    u32 area;
    s32 online;
    u16 seed;
    s32 group;
    s32 row;
    s32 base;
    u16 pick;

    memset(work, 0, 0x2000);
    work->npc = npc;
    work->arg_0x0EC = arg;
    if (game_ready_ck() == 1) {
        code = (u8)npc->field_0x0B6;
        area = code >> 2;
        lobby_w.meal_area_0x149 = area;
        if (code == lb_kitchen_area_codes[area]) {
            work->online = 1;
        } else {
            work->online = 2;
        }
    } else {
        work->online = 0;
    }
    lobby_w.meal_received_0x14A = 0;
    lobby_w.meal_skill_0x14B[0] = 0;
    lobby_w.meal_value_0x14E[0] = 0;
    lobby_w.meal_skill_0x14B[1] = 0;
    lobby_w.meal_value_0x14E[1] = 0;
    lobby_w.meal_skill_0x14B[2] = 0;
    lobby_w.meal_value_0x14E[2] = 0;
    lobby_w.meal_bonus_0x154[0] = 0;
    lobby_w.meal_bonus_0x154[1] = 0;
    lobby_w.meal_bonus_0x154[2] = 0;
    lobby_w.meal_bonus_0x154[3] = 0;
    lobby_w.state_0x000 = 17;
    lobby_w.active_0x008 = 1;
    if (game_ready_ck() == 0) {
        online = 0;
        if (lb_unlock_cond_ck(20) != 0) {
            work->tier = 2;
        } else if (lb_unlock_cond_ck(16) != 0) {
            work->tier = 1;
        } else {
            work->tier = 0;
        }
        lb_talk_page_open(2);
        lb_talk_page_mode_set(0);
    } else {
        online = 1;
        if (lb_unlock_cond_ck(33) != 0) {
            work->tier = 2;
        } else if (lb_unlock_cond_ck(31) != 0) {
            work->tier = 1;
        } else {
            work->tier = 0;
        }
    }
    row = 0;
    seed = lobby_w.field_0x036[online];
    base = 0;
    for (group = 0; group < 6; group++) {
        seed = rand_lcg_step(seed);
        pick = seed % 3;
        work->rows[row].id = base + pick % 3;
        work->rows[row].rare = (seed >> 3) & 1;
        work->rows[row].group = group;
        work->rows[row].enabled = 1;
        work->rows[row + 1].id = base + (pick + 1) % 3;
        work->rows[row + 1].rare = (seed >> 4) & 1;
        work->rows[row + 1].group = group;
        work->rows[row + 1].enabled = 1;
        row += 2;
        base += 3;
    }
}

/* 0x8038F748 (0x4): Closes the kitchen screen. */
s32 lb_kitchen_close(void) {
    return lb_panel_close();
}

/* 0x8038F74C (0x138): Shows ingredient page `page`: copies its six rows and marks each one pickable when the meal is
 * affordable and the row is not the first pick. */
void lb_kitchen_page_set(LbKitchenWork* work, s16 page) {
    s32 i;
    u8 enabled;

    work->row_count = 6;
    work->page = page;
    work->page_count = 2;
    memset(work->page_rows, 0, sizeof(work->page_rows));
    if (lb_kitchen_cost_tbl[work->tier * 2] > ((Q_UserData*)lobby_world_block)->zenny_0x18 &&
        (game_ready_ck() == 1 ||
         lb_kitchen_cost_tbl[work->tier * 2 + 1] > ((Q_UserData*)lobby_world_block)->points_0x3F04)) {
        enabled = 0;
    } else {
        enabled = 1;
    }
    for (i = 0; i < 6; i++) {
        lb_kitchen_row_copy(&work->page_rows[i], &work->rows[page * 6 + i]);
        if (work->picks[0].id == work->page_rows[i].id) {
            work->page_rows[i].enabled = 0;
        } else {
            work->page_rows[i].enabled = enabled;
        }
    }
    if (work->row_cursor >= work->row_count) {
        work->row_cursor = work->row_count - 1;
    }
}

/* 0x8038F884 (0x24): Copies one ingredient row's five bytes. */
void lb_kitchen_row_copy(LbKitchenRow* dst, const LbKitchenRow* src) {
    dst->id = src->id;
    dst->rare = src->rare;
    dst->group = src->group;
    dst->enabled = src->enabled;
}

/* 0x8038F8A8 (0xFC): Whether bonus course `course` can be rolled: a handful of courses need an unlock (offline or
 * online) or are offline-only. */
u32 lb_kitchen_course_ck(u8 course) {
    u32 ok = 1;

    switch (course) {
    case 26:
        if (game_ready_ck() == 1) {
            ok = lb_unlock_cond_ck(35);
        } else {
            ok = 0;
        }
        break;
    case 27:
        if (game_ready_ck() == 0) {
            ok = 0;
        }
        break;
    case 47:
        if (game_ready_ck() == 1) {
            ok = lb_unlock_cond_ck(33);
        } else {
            ok = lb_unlock_cond_ck(20);
        }
        break;
    case 48:
        if (game_ready_ck() == 1) {
            ok = lb_unlock_cond_ck(31);
        } else {
            ok = lb_unlock_cond_ck(16);
        }
        break;
    case 25:
        if (game_ready_ck() == 1) {
            ok = 0;
        }
        break;
    }
    return ok;
}

/* 0x8038F9A4 (0x120): Rolls course set `course`: steps the saved seed 37 times per set, then draws each of the four
 * courses and the extra one from its list's candidates that can be rolled now. */
void lb_kitchen_courses_roll(LbKitchenWork* work, s16 course) {
    u8 picks[0x20];
    s32 i;
    s32 count;
    u8* list;
    u8* pick;
    u16 seed;

    work->seed = ((Q_UserData*)lobby_world_block)->kitchen_seed_0x484E;
    for (i = 0; i < course * 37; i++) {
        work->seed = rand_lcg_step(work->seed);
    }
    for (i = 0; i < 5; i++) {
        count = 0;
        list = lb_kitchen_course_lists[i];
        memset(picks, 0, sizeof(picks));
        pick = picks;
        while (*list != 0) {
            if (lb_kitchen_course_ck(*list) == 1) {
                *pick = *list;
                pick++;
                count++;
            }
            list++;
        }
        seed = rand_lcg_step(work->seed);
        work->seed = seed;
        if (i == 4) {
            work->extra_course = picks[seed % count];
        } else {
            work->courses[i] = picks[seed % count];
        }
    }
}

/* 0x8038FAC4 (0x5C): Empties both picks and shows the first ingredient page and the first course roll. */
void lb_kitchen_pick_reset(LbKitchenWork* work) {
    memset(work->picks, 0xFF, sizeof(work->picks));
    work->step = 0;
    work->row_cursor = 0;
    lb_kitchen_page_set(work, 0);
    lb_kitchen_courses_roll(work, 0);
}

/* 0x8038FB20 (0x450): The pick page's input: the ingredient list, then the course choice, then the payment; 1 once
 * the meal is ordered, 2 when the page is left. */
s32 lb_kitchen_pick_input(LbKitchenWork* work) {
    s32 result = 0;
    s16 page;
    s16 course;
    s16 cursor;

    switch (work->step) {
    case 0:
        if (lb_cmd_pressed_ck(16) != 0) {
            if (work->page_rows[work->row_cursor].enabled == 1) {
                if (work->picks[0].id == 0xFFFF) {
                    lb_kitchen_row_copy(&work->picks[0], &work->page_rows[work->row_cursor]);
                    work->page_rows[work->row_cursor].enabled = 0;
                    sysSE_req(0);
                } else {
                    lb_kitchen_row_copy(&work->picks[1], &work->page_rows[work->row_cursor]);
                    work->step = 1;
                    work->course_cursor = 0;
                    lb_kitchen_courses_roll(work, 0);
                    sysSE_req(27);
                }
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(32) != 0) {
            if (work->picks[0].id == 0xFFFF) {
                result = 2;
            } else {
                work->picks[0].id = 0xFFFF;
                lb_kitchen_page_set(work, work->page);
            }
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->row_cursor = menu_cursor_step(work->row_cursor, work->row_count, lb_cmd_repeat_get(), 1, 2);
        } else if (lb_cmd_repeat_ck(12) != 0) {
            page = menu_cursor_step_fixed_tail(work->page, work->page_count, lb_cmd_repeat_get(), 4, 8,
                                               &work->page_moved);
            work->page = page;
            lb_kitchen_page_set(work, page);
        }
        break;
    case 1:
        if (lb_cmd_pressed_ck(16) != 0) {
            work->step = 2;
            work->confirm_cursor = 0;
            work->confirm_enabled[0] = 1;
            work->confirm_enabled[1] = 1;
            work->confirm_enabled[2] = 1;
            if (work->choice.cursor == 0) {
                if (game_ready_ck() == 0) {
                    work->pay_menu = 0;
                    work->confirm_count = 3;
                    if (lb_kitchen_cost_tbl[work->tier * 2] > ((Q_UserData*)lobby_world_block)->zenny_0x18) {
                        work->confirm_enabled[0] = 0;
                    }
                    if (lb_kitchen_cost_tbl[work->tier * 2 + 1] > ((Q_UserData*)lobby_world_block)->points_0x3F04) {
                        work->confirm_enabled[1] = 0;
                    }
                } else {
                    work->pay_menu = 2;
                    work->confirm_count = 2;
                }
            } else {
                work->pay_menu = 1;
                work->confirm_count = 2;
            }
            sysSE_req(0);
        } else if (lb_cmd_pressed_ck(32) != 0) {
            work->step = 0;
            work->picks[1].id = 0xFFFF;
            lb_kitchen_page_set(work, work->page);
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            course = menu_cursor_step(work->course_cursor, 4, lb_cmd_repeat_get(), 1, 2);
            work->course_cursor = course;
            lb_kitchen_courses_roll(work, course);
        }
        break;
    case 2:
        if (lb_cmd_pressed_ck(16) != 0) {
            cursor = work->confirm_cursor;
            if (work->confirm_enabled[cursor] == 1) {
                switch (cursor) {
                default:
                    work->step = 1;
                    sysSE_req(1);
                    break;
                case 1:
                    if (work->pay_menu == 0) {
                        work->payment = 2;
                        result = 1;
                        sysSE_stop(21);
                    } else {
                        work->step = 1;
                        sysSE_req(1);
                    }
                    break;
                case 0:
                    if (work->pay_menu == 1) {
                        work->payment = 0;
                        result = 1;
                        sysSE_req(0);
                    } else {
                        work->payment = 1;
                        result = 1;
                        sysSE_req(9);
                    }
                    break;
                }
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(32) != 0) {
            work->step = 1;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->confirm_cursor = menu_cursor_step(work->confirm_cursor, work->confirm_count, lb_cmd_repeat_get(), 1, 2);
        }
        break;
    }
    return result;
}

/* 0x8038FF70 (0xDC): Fills the special list from page `page` of the special-course table (up to six, 0-ended). */
void lb_kitchen_list_page_set(LbKitchenWork* work, s16 page) {
    u8* course;
    s32 i;

    work->page = page;
    work->page_count = 2;
    work->row_count = 0;
    course = &lb_kitchen_special_tbl[(s16)(page * 6)];
    for (i = 0; i < 6; i++) {
        if (*course == 0) {
            break;
        }
        work->list[work->row_count] = *course;
        work->row_count++;
        course++;
    }
    if (work->list_cursor >= work->row_count) {
        work->list_cursor = work->row_count - 1;
    }
}

/* The item-hold strip's entry `lb_kitchen_special_open` hands `eft052_hold_entry_set`. size: 0x1C */
typedef struct LbKitchenHoldEntry {
    /* +0x00 */ s32 kind;
    /* +0x04 */ s16 id_0x04;
    /* +0x06 */ s16 value_0x06;
    /* +0x08 */ s16 id_0x08;
    /* +0x0A */ s16 value_0x0A;
    /* +0x0C */ s16 count;
    /* +0x0E */ s16 label;
    /* +0x10 */ s16 flag_0x10;
    /* +0x12 */ u8 pad_0x12[0x2];
    /* +0x14 */ u16* holds;
    /* +0x18 */ s8 flag_0x18;
    /* +0x19 */ u8 pad_0x19[0x3];
} LbKitchenHoldEntry;

/* 0x8039004C (0x108): Opens the special-course pages: clears the picks, collects the special courses the player holds
 * and shows them in the item-hold strip. */
void lb_kitchen_special_open(LbKitchenWork* work) {
    LbKitchenHoldEntry entry;
    s32 i;
    s32 count;

    memset(work->special, 0, 6);
    work->step = 0;
    work->special_count = 0;
    work->special_id = 0;
    memset(work->holds, 0, sizeof(work->holds));
    count = 0;
    for (i = 0; i < 2; i++) {
        if (userdata_item_count_total(lb_kitchen_special_holds[i], lobby_world_block) != 0) {
            work->holds[count] = lb_kitchen_special_holds[i];
            count++;
        }
    }
    entry.kind = 0;
    entry.id_0x04 = 0x16C;
    entry.value_0x06 = -1;
    entry.value_0x0A = -1;
    entry.id_0x08 = 0x16D;
    entry.label = 0x13E7;
    entry.flag_0x10 = 0;
    entry.count = 2;
    entry.holds = work->holds;
    entry.flag_0x18 = 0;
    eft052_hold_entry_set(&entry, 1);
    lb_kitchen_list_page_set(work, 0);
}

/* 0x80390154 (0x4A8): The special page's input: the item-hold strip picks the course, then the special list, the
 * course box and the payment box; 1 once the course is ordered, 2 when the page is left. */
s32 lb_kitchen_special_input(LbKitchenWork* work) {
    s32 result = 0;
    u16 entry;
    s16 page;
    s16 course;
    s16 cursor;

    switch (work->step) {
    case 0:
        switch (eft052_hold_step()) {
        case 1:
            eft052_hold_row_get(&work->special_id, NULL);
            memset(work->special, 0, 6);
            work->step = 1;
            work->page = 0;
            work->list_cursor = 0;
            lb_kitchen_list_page_set(work, 0);
            switch (work->special_id) {
            case 0x1B0:
                work->special_count = 3;
                break;
            default:
                work->special_count = 2;
                break;
            }
            sysSE_req(27);
            break;
        case 2:
            result = 2;
            sysSE_req(1);
            break;
        }
        break;
    case 1:
        if (lb_cmd_pressed_ck(16) != 0) {
            entry = work->list[work->list_cursor];
            if (work->special[0] != entry && work->special[1] != entry && work->special[2] != entry) {
                if (work->special[0] == 0) {
                    work->special[0] = entry;
                    sysSE_req(0);
                } else if (work->special[1] == 0) {
                    work->special[1] = entry;
                    if (work->special_count == 2) {
                        work->step = 2;
                        work->course_cursor = 0;
                        lb_kitchen_courses_roll(work, 0);
                        sysSE_req(27);
                    } else {
                        sysSE_req(0);
                    }
                } else if (work->special[2] == 0) {
                    work->special[2] = entry;
                    work->step = 2;
                    work->course_cursor = 0;
                    lb_kitchen_courses_roll(work, 0);
                    sysSE_req(27);
                }
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(32) != 0) {
            if (work->special[2] != 0) {
                work->special[2] = 0;
                lb_kitchen_list_page_set(work, work->page);
            } else if (work->special[1] != 0) {
                work->special[1] = 0;
                lb_kitchen_list_page_set(work, work->page);
            } else if (work->special[0] != 0) {
                work->special[0] = 0;
                lb_kitchen_list_page_set(work, work->page);
            } else {
                work->step = 0;
            }
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->list_cursor = menu_cursor_step(work->list_cursor, work->row_count, lb_cmd_repeat_get(), 1, 2);
        } else if (lb_cmd_repeat_ck(12) != 0) {
            page = menu_cursor_step_fixed_tail(work->page, work->page_count, lb_cmd_repeat_get(), 4, 8,
                                               &work->page_moved);
            work->page = page;
            lb_kitchen_list_page_set(work, page);
        }
        break;
    case 2:
        if (lb_cmd_pressed_ck(16) != 0) {
            work->step = 3;
            work->confirm_cursor = 0;
            work->pay_menu = 1;
            work->confirm_count = 2;
            sysSE_req(0);
            work->confirm_enabled[0] = 1;
            work->confirm_enabled[1] = 1;
            work->confirm_enabled[2] = 1;
        } else if (lb_cmd_pressed_ck(32) != 0) {
            work->step = 1;
            if (work->special_count == 2) {
                work->special[1] = 0;
            } else {
                work->special[2] = 0;
            }
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            course = menu_cursor_step(work->course_cursor, 4, lb_cmd_repeat_get(), 1, 2);
            work->course_cursor = course;
            lb_kitchen_courses_roll(work, course);
        }
        break;
    case 3:
        if (lb_cmd_pressed_ck(16) != 0) {
            cursor = work->confirm_cursor;
            if (work->confirm_enabled[cursor] == 1) {
                if (cursor != 0) {
                    work->step = 2;
                    sysSE_req(1);
                } else {
                    work->payment = 0;
                    result = 1;
                    sysSE_req(0);
                }
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(32) != 0) {
            work->step = 2;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->confirm_cursor = menu_cursor_step(work->confirm_cursor, work->confirm_count, lb_cmd_repeat_get(), 1, 2);
        }
        break;
    }
    return result;
}

/* 0x803905FC (0x60): Finds the pair (`a`, `b`) in tier `kind`'s pair table: 6-byte records keyed by the two groups
 * (smaller one high), ended by 0xFFFF, whose address the not-found answer is. */
u16* lb_kitchen_pair_find(u8 kind, u8 a, u8 b) {
    u16* p = lb_kitchen_pair_tbls[kind];
    u16 key;
    if (a < b) {
        key = (u16)((a << 8) | b);
    } else {
        key = (u16)((b << 8) | a);
    }
    while (*p != 0xFFFF) {
        if (key == *p) {
            return p;
        }
        p += 3;
    }
    return p;
}

/* 0x8039065C (0x11C): Rolls one bonus course with `chance` percent: takes a random one of the rolled courses still
 * left and closes the gap behind it; 0 when the roll fails. */
u16 lb_kitchen_bonus_roll(LbKitchenWork* work, u8 chance) {
    s32 count = 0;
    u16 bonus = 0;
    u16 seed;
    u16 index;

    work->seed = rand_lcg_step(work->seed);
    seed = work->seed;
    if (seed % 100 < chance) {
        if (work->courses[0] != 0) {
            count = 1;
            if (work->courses[1] != 0) {
                count = 2;
                if (work->courses[2] != 0) {
                    count = 3;
                    if (work->courses[3] != 0) {
                        count = 4;
                    }
                }
            }
        }
        seed = rand_lcg_step(seed);
        work->seed = seed;
        index = seed % count;
        bonus = work->courses[index];
        memcpy(&work->courses[index], &work->courses[index + 1], (4 - (index + 1)) * 2);
        work->courses[3] = 0;
    }
    return bonus;
}

/* 0x80390778 (0x7C): The extra course with `chance` percent, 0 when the roll fails. */
u16 lb_kitchen_extra_roll(LbKitchenWork* work, u8 chance) {
    u16 seed = rand_lcg_step(work->seed);

    work->seed = seed;
    if (seed % 100 < chance) {
        return work->extra_course;
    }
    return 0;
}

/* 0x803907F4 (0x134): Copies a skill row's two skills into the meal parameters: rows 0x8000.. pick two of the bonus
 * table's five rows by the roll seed, the others read the meal skill table. */
void lb_kitchen_skill_apply(LbKitchenWork* work, u16 skill) {
    s16* table;
    s16* pair;
    u16 seed;
    s16 first;
    s16 second;
    MealSkill* row;

    if ((skill & 0x8000) != 0) {
        table = lb_kitchen_bonus_tbls[(u8)skill];
        seed = work->seed;
        first = (seed >> 2) % 5;
        pair = &table[first * 2];
        lb_param_w.flag_0x0C[0] = pair[0];
        lb_param_w.value_0x10[0] = pair[1];
        second = (first + (seed >> 3) % 4 + 1) % 5;
        pair = &table[second * 2];
        lb_param_w.flag_0x0C[1] = pair[0];
        lb_param_w.value_0x10[1] = pair[1];
        lb_param_w.flag_0x0C[2] = 0;
        lb_param_w.value_0x10[2] = 0;
        return;
    }
    row = &meal_skill_tbl[skill];
    lb_param_w.flag_0x0C[0] = row->kind_a;
    lb_param_w.value_0x10[0] = row->value_a;
    lb_param_w.flag_0x0C[1] = row->kind_b;
    lb_param_w.value_0x10[1] = row->value_b;
    lb_param_w.flag_0x0C[2] = 0;
    lb_param_w.value_0x10[2] = 0;
}

/* 0x80390928 (0x5C): Copies the three special picks into the meal parameters with each one's value. */
void lb_kitchen_special_apply(LbKitchenWork* work) {
    for (s32 i = 0; i < 3; i++) {
        lb_param_w.flag_0x0C[i] = work->special[i];
        s16 value = lb_kitchen_special_values[work->special[i]];
        lb_param_w.value_0x10[i] = value;
    }
}

/* 0x80390984 (0x59C): Serves the meal: pays for it, applies the pair's (or the special course's) skills, rolls the
 * bonus courses, hands the result to the cook and the meal parameters, and re-draws the roll seed. */
void lb_kitchen_meal_serve(LbKitchenWork* work) {
    u8 count = 0;
    u16* pair;
    s16 value;
    u16* bonus;
    MealSkill* skill;

    lobby_w.kitchen_busy_0x054 = 1;
    sysSE_stop(9);
    work->bonuses[0] = 0;
    work->bonuses[1] = 0;
    work->bonuses[2] = 0;
    work->bonuses[3] = 0;
    work->message = 0x66;
    switch (work->choice.cursor) {
    case 0:
        pair = lb_kitchen_pair_find(work->tier, work->picks[0].group, work->picks[1].group);
        if (work->picks[0].group != 5 || work->picks[1].group != 5) {
            lb_kitchen_pair_seen_set(work->picks[0].id, work->picks[1].id);
        }
        skill = &meal_skill_tbl[pair[1]];
        work->meal = pair[2];
        work->skill = pair[1];
        value = skill->value_a;
        if (value < 0) {
            work->act = 12;
        } else if (value > 0) {
            work->act = 15;
        } else {
            work->act = 14;
        }
        work->message = pair[2];
        if (work->message == 0x8F) {
            work->act = 36;
        }
        lb_kitchen_skill_apply(work, work->skill);
        switch (work->payment) {
        case 1:
            score_add_clamped(-lb_kitchen_cost_tbl[work->tier * 2], &((Q_UserData*)lobby_world_block)->zenny_0x18);
            break;
        case 2:
            userdata_zenny_add(-lb_kitchen_cost_tbl[work->tier * 2 + 1]);
            break;
        }
        switch (work->picks[1].rare + work->picks[0].rare) {
        case 2:
            work->bonuses[0] = lb_kitchen_bonus_roll(work, 100);
            if (work->bonuses[0] != 0) {
                count = 1;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 80);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 60);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_extra_roll(work, 30);
            break;
        case 1:
            work->bonuses[0] = lb_kitchen_bonus_roll(work, 100);
            if (work->bonuses[0] != 0) {
                count = 1;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 75);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 0);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_extra_roll(work, 10);
            break;
        case 0:
            work->bonuses[0] = lb_kitchen_bonus_roll(work, 80);
            if (work->bonuses[0] != 0) {
                count = 1;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 0);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 0);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_extra_roll(work, 5);
            break;
        }
        break;
    case 1:
        switch (work->special_id) {
        case 0x1B0:
            work->bonuses[0] = lb_kitchen_bonus_roll(work, 100);
            if (work->bonuses[0] != 0) {
                count = 1;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 75);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 50);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_extra_roll(work, 15);
            if (*bonus == 0) {
                *bonus = lb_kitchen_bonus_roll(work, 15);
            }
            break;
        default:
            work->bonuses[0] = lb_kitchen_bonus_roll(work, 90);
            if (work->bonuses[0] != 0) {
                count = 1;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 50);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_bonus_roll(work, 25);
            if (*bonus != 0) {
                count++;
            }
            bonus = &work->bonuses[count];
            *bonus = lb_kitchen_extra_roll(work, 10);
            if (*bonus == 0) {
                *bonus = lb_kitchen_bonus_roll(work, 10);
            }
            break;
        }
        lb_kitchen_special_apply(work);
        work->act = 15;
        work->message = 0x7A;
        eft052_page_count_add(work->special_id, 1);
        break;
    }
    lb_param_w.value_0x16 = work->bonuses[0];
    work->npc->deco_skill_id[0] = work->bonuses[0];
    lb_param_w.value_0x18 = work->bonuses[1];
    work->npc->deco_skill_id[1] = work->bonuses[1];
    lb_param_w.value_0x1A = work->bonuses[2];
    work->npc->deco_skill_id[2] = work->bonuses[2];
    lb_param_w.value_0x1C = work->bonuses[3];
    work->npc->deco_skill_id[3] = work->bonuses[3];
    if (game_ready_ck() == 1 && work->choice.cursor == 1 && work->online == 1) {
        meal_result_send(lobby_w.meal_area_0x149);
    }
    ((Q_UserData*)lobby_world_block)->kitchen_seed_0x484E = ran_suu(1);
}

/* 0x80390F20 (0x5B4): The kitchen screen's frame step: the opening talk page and choice, the pick and special pages,
 * the guest's wait for the host's meal, the eating motion with its messages, and the close; then the draw callback. */
void lb_kitchen_step(void) {
    LbKitchenWork* work = (LbKitchenWork*)lobby_w.menu_0xAC;
    char text[0x40];
    void* name;
    s32 i;

    work->page_moved = 0;
    switch (work->state) {
    case 0:
        if (game_ready_ck() == 0) {
            work->state = 1;
            lb_talk_page_mode_set(0);
        } else {
            work->state = 2;
            lb_choice_init(&work->choice, &lb_kitchen_choice_defs[work->online], 0, 0);
            work->choice.se = -1;
            sysSE_stop(41);
        }
        break;
    case 1:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            if (lb_talk_page_value_get() == 0) {
                work->state = 2;
                lb_choice_init(&work->choice, lb_kitchen_choice_defs, 0, 0);
                work->choice.se = -1;
                camera_talk_lock_set(1);
                lb_act_latch_set(work->npc, 4);
                sysSE_stop(41);
            } else {
                work->state = 7;
                lb_talk_page_open(1);
                sysSE_req(1);
            }
        }
        break;
    case 2:
        ainpc_page_hold_set();
        switch (lb_choice_step(&work->choice)) {
        case 1:
            work->step = 0;
            switch (work->choice.cursor) {
            case 0:
                work->state = 4;
                lb_kitchen_pick_reset(work);
                sysSE_req(27);
                break;
            case 1:
                if (work->online == 2) {
                    work->state = 5;
                    lb_npc_act_set(work->npc, 0, 16, 32);
                    sysSE_req(0);
                } else {
                    work->state = 3;
                    lb_kitchen_special_open(work);
                    sysSE_req(5);
                }
                break;
            }
            break;
        case 2:
            work->state = 7;
            if (game_ready_ck() == 0) {
                lb_npc_act_set(work->npc, 0, 5, 32);
                lb_talk_page_open(1);
                camera_talk_lock_set(0);
            }
            break;
        }
        break;
    case 3:
        ainpc_page_hold_set();
        switch (lb_kitchen_special_input(work)) {
        case 1:
            work->state = 6;
            work->tier = 2;
            lb_npc_act_set(work->npc, 0, (u16)(work->tier + 9), 32);
            lb_kitchen_meal_serve(work);
            break;
        case 2:
            work->state = 2;
            break;
        }
        break;
    case 4:
        ainpc_page_hold_set();
        switch (lb_kitchen_pick_input(work)) {
        case 1:
            work->state = 6;
            lb_npc_act_set(work->npc, 0, (u16)(work->tier + 9), 32);
            lb_kitchen_meal_serve(work);
            break;
        case 2:
            work->state = 2;
            break;
        }
        break;
    case 5:
        if (lobby_w.meal_received_0x14A != 0) {
            sysSE_stop(9);
            work->state = 6;
            work->tier = 2;
            work->act = 15;
            work->message = 0x7A;
            lobby_w.kitchen_busy_0x054 = 1;
            lb_npc_act_set(work->npc, 0, (u16)(work->tier + 9), 32);
            lb_param_w.flag_0x0C[0] = lobby_w.meal_skill_0x14B[0];
            lb_param_w.value_0x10[0] = lobby_w.meal_value_0x14E[0];
            lb_param_w.flag_0x0C[1] = lobby_w.meal_skill_0x14B[1];
            lb_param_w.value_0x10[1] = lobby_w.meal_value_0x14E[1];
            lb_param_w.flag_0x0C[2] = lobby_w.meal_skill_0x14B[2];
            lb_param_w.value_0x10[2] = lobby_w.meal_value_0x14E[2];
            work->bonuses[0] = lobby_w.meal_bonus_0x154[0];
            work->bonuses[1] = lobby_w.meal_bonus_0x154[1];
            work->bonuses[2] = lobby_w.meal_bonus_0x154[2];
            work->bonuses[3] = lobby_w.meal_bonus_0x154[3];
        } else if (lb_cmd_pressed_ck(32) != 0) {
            work->state = 7;
            lb_npc_act_set(work->npc, 0, 5, 32);
        }
        break;
    case 6:
        if (Get_motion_no(work->npc) != 622) {
            work->state = 7;
            lb_npc_act_set(work->npc, 0, work->act, 32);
            hud_msg_push(0, (const char*)LbStr(0, work->message));
            for (i = 0; i < 3; i++) {
                if (lb_param_w.flag_0x0C[i] != 0 && lb_param_w.value_0x10[i] != 0) {
                    if (lb_param_w.value_0x10[i] > 0) {
                        name = LbStr(0, lb_param_w.flag_0x0C[i] + 89);
                        sprintf(text, (const char*)LbStr(0, 145), name);
                    } else {
                        name = LbStr(0, lb_param_w.flag_0x0C[i] + 89);
                        sprintf(text, (const char*)LbStr(0, 146), name);
                    }
                    hud_msg_push(0, text);
                }
            }
            for (i = 0; i < 4; i++) {
                lb_param_w.slots_0x16[i] = work->bonuses[i];
                if (work->bonuses[i] != 0) {
                    name = (void*)str_tbl_course_get(work->bonuses[i]);
                    sprintf(text, (const char*)LbStr(0, 144), name);
                    hud_msg_push(0, text);
                }
            }
            camera_talk_reset();
            if (game_ready_ck() == 0) {
                lb_talk_page_open(1);
                lb_npc_talk_mode_set(lb_npc_find(13), 4);
            }
        }
        break;
    case 7:
        if (game_ready_ck() == 0) {
            if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
                lb_kitchen_close();
            }
        } else {
            lb_kitchen_close();
        }
        break;
    }
    if (work->state > 1 && work->state != 7) {
        subTransSet((u32)lb_kitchen_draw_task, 0, NULL);
    }
}

/* The kitchen screen's sprite tables. */
u16 lb_kitchen_frame_ids[6] = {0x20AF, 0x20B0, 0x20AD, 0x20AE, 0xFFFF, 0x0000};
u16 lb_kitchen_frame_wide_ids[6] = {0x20B4, 0x20B5, 0x20B2, 0x20B3, 0xFFFF, 0x0000};
u16 lb_kitchen_title_ids[14] = {0x20BC, 0x20BD, 0x20B9, 0x20BA, 0x20BB, 0x20BE, 0x20BF, 0x20C0, 0x20C1, 0x20C2, 0x20B7, 0x20B8, 0xFFFF, 0x0000};
u16 lb_kitchen_list_ids[10] = {0x20F8, 0x20F9, 0x20FA, 0x20F6, 0x20F7, 0x20F2, 0x20F3, 0x20F4, 0x20F5, 0xFFFF};
u16 lb_kitchen_row_lsp[6] = {0x20FB, 0x20FC, 0x20FD, 0x20FE, 0x20FF, 0x2100};
u16 lb_kitchen_rare_lsp[6] = {0x2101, 0x2102, 0x2103, 0x2104, 0x2105, 0x2106};
u16 lb_kitchen_pick_ids[18] = {0x211B, 0x211C, 0x211D, 0x211E, 0x211F, 0x2120, 0x2121, 0x2122, 0x2123, 0x2124, 0x2125, 0x2129, 0x212A, 0x2126, 0x2127, 0x2128, 0x212B, 0xFFFF};
u16 lb_kitchen_slot_ids[8] = {0x213B, 0x213C, 0x213D, 0x213E, 0x213F, 0x2140, 0xFFFF, 0x0000};
u16 lb_kitchen_slot_pulse_ids[8] = {0x213B, 0x213C, 0x213D, 0x213E, 0x213F, 0x2140, 0xFFFF, 0x0000};
u16 lb_kitchen_slot_frame_ids[4] = {0x2138, 0x2139, 0x213A, 0xFFFF};
u16 lb_kitchen_slot_lsp[4] = {0x2148, 0x2149, 0x0000, 0x0000};
u16 lb_kitchen_slot_label_ids[2] = {0x2136, 0x2137};

/* 0x803914D4 (0x9C): Draws the kitchen's frame (its wide-screen variant), its title and the money and points. */
void lb_kitchen_frame_draw(void) {
    _mh_ivec2_ pos;

    if (ck_WideMode() != 0) {
        get_lsp_data(0x20B1, &pos);
        draw_sprite_ary(lb_kitchen_frame_wide_ids, &pos);
    } else {
        get_lsp_data(0x20AC, &pos);
        draw_sprite_ary(lb_kitchen_frame_ids, &pos);
    }
    get_lsp_data(0x20B6, &pos);
    draw_sprite_ary(lb_kitchen_title_ids, &pos);
    menu_money_draw(0x20ED);
    if (game_ready_ck() == 0) {
        lb_points_draw(0x20EC);
    }
}

/* 0x80391570 (0x200): Draws the ingredient page: each row's name in its pick/enabled colour, the rare mark and the
 * page arrows. */
void lb_kitchen_list_draw(LbKitchenWork* work) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    LbKitchenRow* row;
    u16* lsp;
    u16* rare;
    s32 i;
    u32 color;
    bool cursor;
    bool enabled;
    bool active = work->step == 0;
    u16 name;

    get_lsp_data(0x20EF, &base);
    draw_sprite_ary(lb_kitchen_list_ids, &base);
    draw_sprite_idx(0x20F0, &base);
    lb_page_arrow_draw(0x20EE, work->page, work->page_count, work->page_moved, active);
    for (i = 0, row = work->page_rows, lsp = lb_kitchen_row_lsp, rare = lb_kitchen_rare_lsp; i < 6; i++) {
        cursor = i == work->row_cursor;
        enabled = row->enabled != 0;
        if (work->picks[0].id == row->id || work->picks[1].id == row->id) {
            color = GetMenuFontColor(true, true, false, false);
        } else {
            color = GetMenuFontColor(enabled, cursor, active, false);
        }
        switch (work->tier) {
        case 0:
            name = row->id + 23;
            break;
        case 1:
            name = row->id + 41;
            break;
        case 2:
            name = row->id + 59;
            break;
        }
        if (!active) {
            cursor = false;
        }
        get_lsp_data(*lsp, &pos);
        pos.x += base.x;
        pos.y += base.y;
        lb_list_row_draw((s8*)LbStr(0, name), cursor, &pos, color, 13);
        if (row->rare != 0) {
            get_lsp_data(*rare, &pos);
            pos.x += base.x;
            pos.y += base.y;
            draw_sprite_idx(0x2108, &pos);
        }
        row++;
        lsp++;
        rare++;
    }
}

/* 0x80391770 (0x10): Draws the pulsing highlight of the slot being filled. */
void lb_kitchen_slot_pulse_draw(_mh_ivec2_* pos) {
    lb_sprite_pulse_draw(lb_kitchen_slot_pulse_ids, pos);
}

/* 0x80391780 (0x1BC): Writes a skill's effect text: nothing, its value (a sixth of it for kind 2), or the
 * strong/medium/weak word its value's size picks. */
void lb_kitchen_skill_text(char* out, u8 kind, u16 str, s16 value) {
    s16 size = abs(value);
    s32 limit = size;

    switch (kind) {
    case 0:
        sprintf(out, "");
        return;
    case 1:
    case 3:
        sprintf(out, "%s%d", LbStr(1, str), size);
        return;
    case 2:
        sprintf(out, "%s%d", LbStr(1, str), size / 6);
        return;
    case 4:
        limit = 5;
        break;
    case 5:
        if (value >= 15) {
            sprintf(out, "%s", LbStr(0, 0x93));
        } else if (value >= 10) {
            sprintf(out, "%s", LbStr(0, 0x94));
        } else {
            sprintf(out, "%s", LbStr(0, 0x95));
        }
        return;
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        limit = 5;
        break;
    }
    if (value >= limit) {
        sprintf(out, "%s", LbStr(0, 0x93));
    } else {
        sprintf(out, "%s", LbStr(0, 0x95));
    }
}


/* 0x8039193C (0x418): Draws the pick panel: the meal's costs, the two pick slots (the one being filled pulsing) and,
 * once the first pick is in, what the pair with the row under the cursor makes. */
void lb_kitchen_pick_draw(LbKitchenWork* work) {
    char text[0x40];
    char first[0x10];
    char second[0x10];
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    LbKitchenRow* pick;
    u16* label;
    u16* lsp;
    s32 i;
    s32 filling;
    u16 name;
    LbKitchenRow* row;
    void* str;
    MealSkill* skill;

    get_lsp_data(0x211A, &base);
    draw_sprite_ary(lb_kitchen_pick_ids, &base);
    sprintf(text, "%d%s", lb_kitchen_cost_tbl[work->tier * 2], LbStr(1, 2));
    draw_font_idx(0x212D, (s8*)text, 6, &base);
    if (game_ready_ck() == 0) {
        sprintf(text, "%d%s", lb_kitchen_cost_tbl[work->tier * 2 + 1], LbStr(1, 44));
        draw_font_idx(0x212C, (s8*)text, 6, &base);
    }
    get_lsp_data(0x212E, &base);
    uv_pair_copy(&pos, &base);
    if (work->picks[0].id == 0xFFFF) {
        filling = 0;
    } else if (work->picks[1].id == 0xFFFF) {
        filling = 1;
    } else {
        filling = -1;
    }
    for (i = 0, pick = work->picks, label = lb_kitchen_slot_label_ids, lsp = lb_kitchen_slot_lsp; i < 2; i++) {
        draw_sprite_ary(lb_kitchen_slot_ids, &pos);
        if (filling == i) {
            lb_kitchen_slot_pulse_draw(&pos);
        }
        draw_sprite_ary(lb_kitchen_slot_frame_ids, &pos);
        draw_sprite_idx(*label, &pos);
        if (pick->id != 0xFFFF) {
            if (pick->rare != 0) {
                draw_sprite_idx(0x2147, &pos);
            }
            switch (work->tier) {
            case 0:
                name = pick->id + 23;
                break;
            case 1:
                name = pick->id + 41;
                break;
            case 2:
                name = pick->id + 59;
                break;
            }
            draw_font_idx(0x212F, (s8*)LbStr(0, name), 4, &pos);
        }
        get_lsp_data(*lsp, &pos);
        pos.x += base.x;
        pos.y += base.y;
        pick++;
        label++;
        lsp++;
    }
    if (work->picks[0].id != 0xFFFF) {
        draw_sprite_ary(lb_kitchen_slot_ids, &pos);
        draw_sprite_ary(lb_kitchen_slot_frame_ids, &pos);
        draw_sprite_idx(0x2132, &pos);
        row = &work->page_rows[work->row_cursor];
        if (work->picks[0].id == row->id) {
            sprintf(text, "%s", LbStr(0, 100));
        } else if (lb_kitchen_pair_seen_ck(work->picks[0].id, row->id) != 0) {
            skill = &meal_skill_tbl[lb_kitchen_pair_find(work->tier, work->picks[0].group, row->group)[1]];
            if (skill->kind_a == 0) {
                sprintf(text, "%s", LbStr(0, 89));
            } else {
                lb_kitchen_skill_text(first, skill->kind_a, (skill->value_a < 0) + 8, skill->value_a);
                if (skill->kind_b == 0) {
                    sprintf(text, "%s%s", LbStr(0, skill->kind_a + 89), first);
                } else {
                    lb_kitchen_skill_text(second, skill->kind_b, (skill->value_b < 0) + 8, skill->value_b);
                    str = LbStr(0, skill->kind_b + 89);
                    sprintf(text, "%s%s %s%s", LbStr(0, skill->kind_a + 89), first, str, second);
                }
            }
        } else {
            sprintf(text, "%s", LbStr(0, 101));
        }
        draw_font_idx(0x2131, (s8*)text, 5, &pos);
    }
}

/* The kitchen boxes' sprite tables. */
u16 lb_kitchen_course_ids[20] = {0x214B, 0x214C, 0x214D, 0x214E, 0x214F, 0x2150, 0x2151, 0x2152, 0x2153, 0x2154, 0x2155, 0x2156, 0x2157, 0x2158, 0x2159, 0x215A, 0x215B, 0x215C, 0xFFFF, 0x0000};
u16 lb_kitchen_course_row_ids[6] = {0x215F, 0x2160, 0x2161, 0x2162, 0xFFFF, 0x0000};
u16 lb_kitchen_course_cursor_ids[6] = {0x2166, 0x2163, 0x2164, 0x2165, 0xFFFF, 0x0000};
u16 lb_kitchen_bonus_ids[16] = {0x216B, 0x216C, 0x216D, 0x216E, 0x216F, 0x2170, 0x2171, 0x2172, 0x2173, 0x2174, 0x2175, 0x2176, 0x2177, 0x2178, 0xFFFF, 0x0000};
u16 lb_kitchen_confirm_ids[8] = {0x2182, 0x2183, 0x2184, 0x2185, 0x2186, 0x2187, 0x2188, 0xFFFF};
u16 lb_kitchen_confirm_row_ids[6] = {0x218B, 0x218C, 0x218D, 0x218E, 0x218F, 0xFFFF};
u16 lb_kitchen_confirm_cursor_ids[6] = {0x2190, 0x2191, 0x2192, 0x2193, 0xFFFF, 0x0000};
u16 lb_kitchen_choice_sprites[12] = {0x20CE, 0x20CF, 0x20D0, 0x20CA, 0x20CB, 0x20CC, 0x20CD, 0x20C6, 0x20C7, 0x20C8, 0x20C9, 0xFFFF};
u16 lb_kitchen_special_label_ids[3] = {0x2133, 0x2134, 0x2135};
u16 lb_kitchen_course_lsp[4] = {0x2167, 0x2168, 0x2169, 0x0000};
u16 lb_kitchen_bonus_frame_ids[4] = {0x217B, 0x217C, 0x217D, 0xFFFF};
u16 lb_kitchen_bonus_lsp[4] = {0x217E, 0x217F, 0x2180, 0x0000};
u16 lb_kitchen_confirm_lsp[4] = {0x2194, 0x2195, 0x0000, 0x0000};
u16 lb_kitchen_choice_ids[4] = {0x20D1, 0x20D2, 0x0000, 0x0000};

/* 0x80391D54 (0x1A0): Draws the special pick panel: one slot per pick the course takes, the slot being filled
 * pulsing, and each pick's name. */
void lb_kitchen_special_pick_draw(LbKitchenWork* work) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    u16* label;
    u16* lsp;
    s32 i;
    s32 filling;

    get_lsp_data(0x211A, &base);
    draw_sprite_ary(lb_kitchen_pick_ids, &base);
    draw_font_idx(0x212D, (s8*)LbStr(0, 0x58), 4, &base);
    get_lsp_data(0x212E, &base);
    uv_pair_copy(&pos, &base);
    if (work->special[0] == 0) {
        filling = 0;
    } else if (work->special[1] == 0) {
        filling = 1;
    } else if (work->special[2] == 0 && work->special_count == 3) {
        filling = 2;
    } else {
        filling = -1;
    }
    for (i = 0, label = lb_kitchen_special_label_ids, lsp = lb_kitchen_slot_lsp; i < work->special_count; i++) {
        draw_sprite_ary(lb_kitchen_slot_ids, &pos);
        if (filling == i) {
            lb_kitchen_slot_pulse_draw(&pos);
        }
        draw_sprite_ary(lb_kitchen_slot_frame_ids, &pos);
        draw_sprite_idx(*label, &pos);
        if (work->special[i] != 0) {
            draw_font_idx(0x212F, (s8*)LbStr(0, work->special[i] + 0x59), 4, &pos);
        }
        get_lsp_data(*lsp, &pos);
        pos.x += base.x;
        pos.y += base.y;
        label++;
        lsp++;
    }
}

/* 0x80391EF4 (0x2B0): Draws the course box (the four course rows, the cursor's lit while `active`) and the bonus
 * box: each rolled course's name with its help record. */
void lb_kitchen_course_draw(LbKitchenWork* work, bool active) {
    _SPR_DATA_ spr;
    PlaceRec place;
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    bool cursor;
    u32 i;
    u32 flags;
    u32 color;
    s16 x;
    s16 y;
    s16 width;

    get_lsp_data(0x214A, &base);
    draw_sprite_ary(lb_kitchen_course_ids, &base);
    get_lsp_data(0x215D, &base);
    draw_sprite_ary(lb_kitchen_course_row_ids, &base);
    uv_pair_copy(&pos, &base);
    for (i = 0; i < 4; i++) {
        if (i == work->course_cursor) {
            cursor = true;
            flags = 5;
        } else {
            cursor = false;
            flags = 1;
        }
        color = GetMenuFontColor(true, cursor, active, false);
        draw_sprite_ary(lb_kitchen_course_row_ids, &pos);
        spr_data_copy(&spr, get_lsp_data(0x215E, NULL));
        spr.color = color;
        draw_font(spr, (s8*)LbStr(0, i + 77), flags, &pos);
        if (active && cursor) {
            put_menu_cursor(lb_kitchen_course_cursor_ids, 0, &pos);
        }
        get_lsp_data(lb_kitchen_course_lsp[i], &pos);
        pos.x += base.x;
        pos.y += base.y;
    }
    get_lsp_data(0x216A, &base);
    draw_sprite_ary(lb_kitchen_bonus_ids, &base);
    get_lsp_data(0x2179, &base);
    uv_pair_copy(&pos, &base);
    for (i = 0; i < 4; i++) {
        draw_sprite_ary(lb_kitchen_bonus_frame_ids, &pos);
        spr_data_copy(&spr, get_lsp_data(0x217A, NULL));
        draw_font(spr, (s8*)str_tbl_course_get(work->courses[i]), 1, &pos);
        x = menu_text_center_x((char*)str_tbl_course_get(work->courses[i]), (s16)(spr.pos.x + pos.x), spr.width);
        y = spr.pos.y + pos.y;
        width = (spr.width * flfntStrLen((char*)str_tbl_course_get(work->courses[i]))) / 2;
        place_rec_course_set(&place, work->courses[i]);
        place_rec_alloc(&place, x, y, width, spr.height, 0, 9);
        get_lsp_data(lb_kitchen_bonus_lsp[i], &pos);
        pos.x += base.x;
        pos.y += base.y;
    }
    ainpc_page_mark_a_draw(0x2196, NULL);
}

/* 0x803921A4 (0x184): Draws the payment box: one row per payment the menu offers, the cursor's row lit. */
void lb_kitchen_confirm_draw(LbKitchenWork* work) {
    _SPR_DATA_ spr;
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    bool cursor;
    u32 i;
    u32 flags;
    u32 color;
    u16 str;

    get_lsp_data(0x2181, &base);
    draw_sprite_anim_ary(lb_kitchen_confirm_ids, work->confirm_count, &base);
    switch (work->pay_menu) {
    default:
        str = 0x51;
        break;
    case 1:
        str = 0x54;
        break;
    case 2:
        str = 0x56;
        break;
    }

    get_lsp_data(0x2189, &base);
    uv_pair_copy(&pos, &base);
    for (i = 0; i < (u32)work->confirm_count; i++) {
        if (i == work->confirm_cursor) {
            cursor = 1;
            flags = 5;
        } else {
            cursor = 0;
            flags = 1;
        }
        color = GetMenuFontColor(work->confirm_enabled[i], cursor, true, false);
        draw_sprite_ary(lb_kitchen_confirm_row_ids, &pos);
        spr_data_copy(&spr, get_lsp_data(0x218A, NULL));
        spr.color = color;
        draw_font(spr, (s8*)LbStr(0, str), flags, &pos);
        if (cursor != 0) {
            put_menu_cursor(lb_kitchen_confirm_cursor_ids, 0, &pos);
        }
        get_lsp_data(lb_kitchen_confirm_lsp[i], &pos);
        pos.x += base.x;
        pos.y += base.y;
        str++;
    }
}

/* 0x80392328 (0x6C): Draws the pick screen: the list, the picks and, past the list, the course and payment boxes. */
void lb_kitchen_pick_screen_draw(LbKitchenWork* work) {
    lb_kitchen_list_draw(work);
    lb_kitchen_pick_draw(work);
    if (work->step == 2) {
        lb_kitchen_confirm_draw(work);
        lb_kitchen_course_draw(work, 0);
    } else if (work->step != 0) {
        lb_kitchen_course_draw(work, 1);
    }
}

/* 0x80392394 (0x170): Draws the special list: each course's name, lit when picked, and the page arrows. */
void lb_kitchen_special_list_draw(LbKitchenWork* work) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    s32 i;
    bool cursor;
    bool picked;
    u32 color;
    bool active = work->step == 1;

    get_lsp_data(0x20EF, &base);
    draw_sprite_ary(lb_kitchen_list_ids, &base);
    draw_sprite_idx(0x20F1, &base);
    lb_page_arrow_draw(0x20EE, work->page, work->page_count, work->page_moved, active);
    for (i = 0; i < work->row_count; i++) {
        cursor = i == work->list_cursor;
        if (work->special[0] == work->list[i] || work->special[1] == work->list[i] ||
            work->special[2] == work->list[i]) {
            picked = true;
        } else {
            picked = false;
        }
        color = GetMenuFontColor(true, cursor, active, picked);
        get_lsp_data(lb_kitchen_row_lsp[i], &pos);
        pos.x += base.x;
        pos.y += base.y;
        if (!active) {
            cursor = false;
        }
        lb_list_row_draw((s8*)LbStr(0, work->list[i] + 0x59), cursor, &pos, color, 13);
    }
}

/* 0x80392504 (0x9C): Draws the special screen: the item-hold strip before a course is picked, then the list and the
 * picks, the course box and the payment box as the steps advance. */
void lb_kitchen_special_screen_draw(LbKitchenWork* work) {
    switch (work->step) {
    default:
        eft052_hold_draw(0);
        break;
    case 1:
        lb_kitchen_special_list_draw(work);
        lb_kitchen_special_pick_draw(work);
        break;
    case 2:
        lb_kitchen_special_list_draw(work);
        lb_kitchen_special_pick_draw(work);
        lb_kitchen_course_draw(work, 1);
        break;
    case 3:
        lb_kitchen_special_list_draw(work);
        lb_kitchen_special_pick_draw(work);
        lb_kitchen_course_draw(work, 0);
        lb_kitchen_confirm_draw(work);
        break;
    }
}

/* 0x803925A0 (0x25C): Draws the help panel's message and its second line for the current step. */
void lb_kitchen_msg_draw(LbKitchenWork* work) {
    u32 msg = 0xFFFF;
    u16 sub = 0xFFFF;
    s32 color = 2;
    s32 line = 1;
    LbKitchenRow* row;
    u16 pick;

    switch (work->state) {
    case 4:
        switch (work->step) {
        case 0:
            pick = work->picks[0].id;
            msg = (pick == 0xFFFF) ? 0x16E : 0x16F;
            row = &work->page_rows[work->row_cursor];
            if (row->enabled == 0) {
                if (pick == row->id) {
                    sub = 0x171;
                } else if (game_ready_ck() == 1) {
                    sub = 0x175;
                    line = 2;
                } else {
                    sub = 0x170;
                }
            }
            break;
        case 1:
            msg = 0x172;
            sub = 0x173;
            color = 5;
            break;
        case 2:
            switch (work->confirm_cursor) {
            case 0:
                msg = 0x174;
                if (work->confirm_enabled[work->confirm_cursor] == 0) {
                    sub = 0x175;
                    line = 2;
                }
                break;
            case 1:
                if (game_ready_ck() == 1) {
                    msg = 0x178;
                } else {
                    msg = 0x176;
                    if (work->confirm_enabled[work->confirm_cursor] == 0) {
                        sub = 0x177;
                        line = 2;
                    }
                }
                break;
            case 2:
                msg = 0x178;
                break;
            }
            break;
        }
        break;
    case 3:
        switch (work->step) {
        case 1:
            if (work->special[0] == 0) {
                msg = 0x179;
            } else {
                msg = (work->special[1] == 0) ? 0x17A : 0x17B;
            }
            if (work->special[0] == work->list[work->list_cursor] ||
                work->special[1] == work->list[work->list_cursor]) {
                sub = 0x17C;
            }
            break;
        case 2:
            msg = 0x172;
            break;
        case 3:
            switch (work->confirm_cursor) {
            case 0:
                msg = 0x17D;
                break;
            case 1:
                msg = 0x178;
                break;
            }
            break;
        }
        break;
    }
    if ((u16)msg != 0xFFFF) {
        lb_panel_msg_draw(0x1877, msg);
        if (sub != 0xFFFF) {
            lb_panel_line_draw(0x1877, sub, color, line);
        }
    }
}

/* 0x803927FC (0xCC): The kitchen's draw callback: the frame, then the opening choice, the pick screen or the special
 * screen, then the help panel. */
void lb_kitchen_draw_task(void) {
    LbKitchenWork* work = (LbKitchenWork*)lobby_w.menu_0xAC;
    _mh_ivec2_ pos;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    lb_kitchen_frame_draw();
    switch (work->state) {
    case 2:
        lb_choice_draw(&work->choice, 0x20C4, lb_kitchen_choice_sprites, lb_kitchen_choice_ids, 0x1877, 0);
        get_lsp_data(0x20C4, &pos);
        draw_sprite_idx(0x20C5, &pos);
        break;
    case 4:
        lb_kitchen_pick_screen_draw(work);
        break;
    case 3:
        lb_kitchen_special_screen_draw(work);
        break;
    }
    lb_kitchen_msg_draw(work);
}

/* 0x803928C8 (0x28): Whether the kitchen is outside its menu states (2..4). */
u32 lb_kitchen_idle_ck(void) {
    return (u32)(((LbKitchenWork*)lobby_w.menu_0xAC)->state - 2) > 2;
}

/* --------------------------------------------------------------------------------------------- *
 * The trade screen.
 * --------------------------------------------------------------------------------------------- */

/* The trade screen's tables. */
LbChoiceDef lb_trade_choice_def = {2, 0, 0, 0x96, 0x20, 0, 27, 3};
u16 lb_trade_frame_ids[6] = {0x21A7, 0x21A8, 0x21A5, 0x21A6, 0xFFFF, 0x0000};
u16 lb_trade_frame_wide_ids[6] = {0x21AC, 0x21AD, 0x21AA, 0x21AB, 0xFFFF, 0x0000};
u16 lb_trade_title_ids[14] = {0x219D, 0x219E, 0x219A, 0x219B, 0x219C, 0x219F, 0x21A0, 0x21A1, 0x21A2, 0x21A3, 0x2198, 0x2199, 0xFFFF, 0x0000};
u16 lb_trade_list_ids[10] = {0x21BE, 0x21BF, 0x21C0, 0x21B6, 0x21B7, 0x21B8, 0x21B9, 0x21BA, 0x21BB, 0xFFFF};
u16 lb_trade_row_lsp[6] = {0x21C3, 0x21C4, 0x21C5, 0x21C6, 0x21C7, 0x0000};
u16 lb_trade_detail_ids[20] = {0x21CA, 0x21E7, 0x21CE, 0x21CF, 0x21D0, 0x21D1, 0x21D2, 0x21D3, 0x21D4, 0x21D5, 0x21D6, 0x21D7, 0x21D8, 0x21D9, 0x21DA, 0x21CB, 0x21CC, 0x21CD, 0xFFFF, 0x0000};
u16 lb_trade_pouch_ids[6] = {0x21E0, 0x21E1, 0x21E2, 0x21DF, 0xFFFF, 0x0000};
u16 lb_trade_box_ids[6] = {0x21DC, 0x21DD, 0x21DE, 0x21DB, 0xFFFF, 0x0000};
u16 lb_trade_detail_lsp[8] = {0x21E3, 0x21E9, 0x21EA, 0x21E8, 0x21E5, 0x21E6, 0x0000, 0x0000};
u16 lb_trade_cost_ids[24] = {0x21ED, 0x21EE, 0x21EF, 0x21F0, 0x21F1, 0x21F2, 0x21F3, 0x21F4, 0x21F5, 0x21F6, 0x21F7, 0x21F8, 0x21F9, 0x21FA, 0x21FB, 0x21FC, 0x21FD, 0x21FE, 0x21FF, 0x2200, 0x2201, 0x2202, 0x2203, 0xFFFF};
u16 lb_trade_cost_cursor_ids[16] = {0x2204, 0x2205, 0x2206, 0x2207, 0x2208, 0x2209, 0x220A, 0x220B, 0x220C, 0x220D, 0x220E, 0x220F, 0x2210, 0xFFFF, 0x0000, 0x0000};
u16 lb_trade_cost_row_ids[16] = {0x2214, 0x2215, 0x2216, 0x2217, 0x2218, 0x2219, 0x221A, 0x221B, 0x221C, 0x2227, 0x2228, 0x2229, 0x222A, 0x222B, 0x222C, 0xFFFF};
u16 lb_trade_label_ids[4] = {0x21B2, 0x21B3, 0x21B4, 0x21B5};
u16 lb_trade_choice_ids[2] = {0x21C1, 0x21C2};
u16 lb_trade_cost_lsp[3] = {0x2230, 0x2231, 0x0009};

/* 0x803928F0 (0x70): Opens the trade screen: clears the whole screen block, files the opener's records and puts the
 * lobby into screen 0x12. */
void lb_trade_open(s32 arg, LbTradeSource* source) {
    LbTradeWork* work = (LbTradeWork*)lobby_w.menu_0xAC;

    memset(work, 0, 0x2000);
    work->arg_0x058 = arg;
    work->source = source;
    lobby_w.state_0x000 = 0x12;
    lobby_w.active_0x008 = 1;
    lb_talk_page_open(0);
}

/* 0x80392960 (0x4): Closes the trade screen. */
s32 lb_trade_close(void) {
    return lb_panel_close();
}

/* 0x80392964 (0xE8): The state bits of one offer: 1 when its stock (box or pouch, `dest`) is gone, 4 when it is short,
 * 2 when the player can pay none of its prices. */
u8 lb_trade_offer_state(LbTradeOffer* offer, s16 dest) {
    s32 i;
    u8 state = 0;
    s32 stock = eft052_page_count_ck(offer->item.item, dest);
    u8 payable;

    if (dest == 1) {
        if (stock < offer->item.count) {
            state |= 1;
        }
    } else if (stock < 0) {
        state |= 1;
    } else if (stock < offer->item.count) {
        state |= 4;
    }
    payable = 0;
    for (i = 0; i < 3; i++) {
        if (offer->costs[i].item != 0 &&
            userdata_item_count_total(offer->costs[i].item, lobby_world_block) >= offer->costs[i].count) {
            payable++;
        }
    }
    if (payable == 0) {
        state |= 2;
    }
    return state;
}

/* 0x80392A4C (0x17C): Fills trade page `page` (five offers of the ten, or fifteen during the event) with each offer's
 * state and the prices the player can pay. */
void lb_trade_page_set(LbTradeWork* work, s16 page) {
    s32 first = page * 5;
    s32 last = first + 4;
    s32 count;
    s32 i;
    s32 j;
    s16 index;
    LbTradeOffer* offer;

    work->row_count = 0;
    memset(work->rows, 0, sizeof(work->rows));
    index = 0;
    offer = (LbTradeOffer*)NetCtrlWk::getServerNotice();
    count = lobby_w.field_0x15E == 1 ? 15 : 10;
    for (i = 0; i < count; i++) {
        if (i >= first && i <= last) {
            work->rows[work->row_count].index = index;
            work->rows[work->row_count].state = lb_trade_offer_state(offer, work->choice.cursor);
            for (j = 0; j < 3; j++) {
                if (userdata_item_count_total(offer->costs[j].item, lobby_world_block) < offer->costs[j].count) {
                    work->rows[work->row_count].affordable[j] = 0;
                } else {
                    work->rows[work->row_count].affordable[j] = 1;
                }
            }
            work->row_count++;
        }
        index++;
        offer++;
    }
    if (work->row_cursor >= work->row_count) {
        work->row_cursor = work->row_count - 1;
    }
    work->page = page;
    work->page_count = menu_page_count(index, 5);
}

/* 0x80392BC8 (0x100): Makes the picked exchange: takes the price, gives the item to the pouch or the box and
 * refreshes the page; 1 while the same exchange can be made again. */
s32 lb_trade_exchange(LbTradeWork* work) {
    LbTradeOffer* offer;
    LbTradeCost item;
    LbTradeCost cost;
    u8 stored[4];

    offer = (LbTradeOffer*)&((u32*)NetCtrlWk::getServerNotice())[work->rows[work->row_cursor].index * 4];
    item_pair_copy(&item, &offer->item);
    item_pair_copy(&cost, &offer->costs[work->cost_cursor]);
    work->hold = 4;
    eft052_page_count_add(cost.item, cost.count);
    if (work->choice.cursor == 0) {
        userdata_item_give(lobby_world_block, item.item, item.count, 1);
    } else {
        item_box_store(item.item, item.count, stored);
    }
    lb_trade_page_set(work, work->page);
    if (work->rows[work->row_cursor].state == 0 && work->rows[work->row_cursor].affordable[work->cost_cursor] == 1) {
        return 1;
    }
    return 0;
}

/* 0x80392CC8 (0x478): The trade screen's frame step: the opening choice, the offer list, the price list, the yes/no
 * and the held repeat; then the draw callback. */
void lb_trade_step(void) {
    LbTradeWork* work = (LbTradeWork*)lobby_w.menu_0xAC;
    u32* notice;
    LbTradeOffer* offer;
    s32 again;

    if (work->hold != 0) {
        work->hold--;
    }
    work->page_moved = 0;
    switch (work->state) {
    case 0:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            work->state++;
            lb_choice_init(&work->choice, &lb_trade_choice_def, 0, 0);
            camera_talk_lock_set(1);
            sysSE_stop(41);
        }
        break;
    case 1:
        ainpc_page_hold_set();
        switch (lb_choice_step(&work->choice)) {
        case 1:
            work->state = 2;
            work->row_cursor = 0;
            lb_trade_page_set(work, 0);
            break;
        case 2:
            work->state = 6;
            lb_talk_page_open(1);
            camera_talk_lock_set(0);
            break;
        }
        break;
    case 2:
        ainpc_page_hold_set();
        if (lb_cmd_pressed_ck(16) != 0) {
            if (work->rows[work->row_cursor].state == 0) {
                notice = (u32*)NetCtrlWk::getServerNotice();
                offer = (LbTradeOffer*)&notice[work->rows[work->row_cursor].index * 4];
                if (offer->costs[2].item != 0) {
                    work->cost_count = 3;
                } else if (offer->costs[1].item != 0) {
                    work->cost_count = 2;
                } else {
                    work->cost_count = 1;
                }
                work->cost_cursor = 0;
                work->state = 3;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(32) != 0) {
            work->state = 1;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->row_cursor = menu_cursor_step(work->row_cursor, work->row_count, lb_cmd_repeat_get(), 1, 2);
        } else if (lb_cmd_repeat_ck(12) != 0) {
            work->page = menu_cursor_step_fixed_tail(work->page, work->page_count, lb_cmd_repeat_get(), 4, 8,
                                                     &work->page_moved);
            lb_trade_page_set(work, work->page);
        }
        break;
    case 3:
        ainpc_page_hold_set();
        if (lb_cmd_pressed_ck(16) != 0) {
            if (work->rows[work->row_cursor].affordable[work->cost_cursor] == 1) {
                work->state = 4;
                work->repeat = 0;
                work->yes_no = 0;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(32) != 0) {
            work->state = 2;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->cost_cursor = menu_cursor_step(work->cost_cursor, work->cost_count, lb_cmd_repeat_get(), 1, 2);
        }
        break;
    case 4:
        ainpc_page_hold_set();
        switch (toggle_word_step(&work->yes_no, lb_cmd_repeat_get(), 4, 8, 0xFFFF)) {
        case 1:
            sysSE_stop(27);
            work->repeat = 16;
            if (lb_trade_exchange(work) == 0) {
                work->state = 2;
            } else {
                work->state = 5;
            }
            break;
        case 2:
            work->state = 3;
            break;
        }
        break;
    case 5:
        ainpc_page_hold_set();
        if (lb_cmd_held_ck(16) != 0) {
            if (--work->repeat <= 0) {
                work->repeat = 5;
                again = lb_trade_exchange(work);
                sysSE_stop(27);
                if (again == 0) {
                    work->state = 2;
                    sysSE_req(1);
                }
            }
        } else {
            work->state = 3;
        }
        break;
    case 6:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            lb_trade_close();
        }
        break;
    }
    if (work->state != 0 && work->state != 6) {
        subTransSet((u32)lb_trade_draw_task, 0, NULL);
    }
}

/* 0x80393140 (0x80): Draws the trade screen's frame (its wide-screen variant) and its title. */
void lb_trade_frame_draw(void) {
    _mh_ivec2_ pos;

    if (ck_WideMode() != 0) {
        get_lsp_data(0x21A9, &pos);
        draw_sprite_ary(lb_trade_frame_wide_ids, &pos);
    } else {
        get_lsp_data(0x21A4, &pos);
        draw_sprite_ary(lb_trade_frame_ids, &pos);
    }
    get_lsp_data(0x2197, &pos);
    draw_sprite_ary(lb_trade_title_ids, &pos);
}

/* 0x803931C0 (0x228): Draws the offer list, the picked offer's item panel and text, the box/pouch tabs and the page
 * arrows. */
void lb_trade_list_draw(LbTradeWork* work) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    u16 item;
    s32 box_mark = 0;
    s32 pouch_mark = 0;
    bool active = work->state == 2;
    u32* notice;
    s32 i;
    LbTradeOffer* offer;
    s32 cursor;
    s32 enabled;

    get_lsp_data(0x21B1, &base);
    draw_sprite_anim_ary(lb_trade_list_ids, 5, &base);
    draw_sprite_idx(lb_trade_label_ids[1], &base);
    notice = (u32*)NetCtrlWk::getServerNotice();
    for (i = 0; i < work->row_count; i++) {
        offer = (LbTradeOffer*)&notice[work->rows[i].index * 4];
        if (i == work->row_cursor) {
            cursor = 1;
            item = offer->item.item;
        } else {
            cursor = 0;
        }
        enabled = work->rows[i].state == 0;
        get_lsp_data(lb_trade_row_lsp[i], &pos);
        pos.x += base.x;
        pos.y += base.y;
        lb_item_cell_draw(offer->item.item, offer->item.count, &pos, enabled, cursor, active, 0);
    }
    if (work->hold != 0) {
        if (work->choice.cursor == 0) {
            box_mark = 2;
        }
        if (work->choice.cursor != 0) {
            pouch_mark = 2;
        }
    }
    get_lsp_data(0x21C9, &base);
    draw_sprite_ary(lb_trade_detail_ids, &base);
    lb_item_detail_draw(0x21C9, item, lb_trade_detail_lsp, box_mark, pouch_mark);
    draw_sprite_anim_ary(lb_trade_box_ids, work->choice.cursor == 1, &base);
    draw_sprite_anim_ary(lb_trade_pouch_ids, work->choice.cursor == 0, &base);
    draw_font_idx(0x21E4, (s8*)ItemExp(item), 0, &base);
    get_lsp_data(0x21B1, &base);
    draw_sprite_anim_idx(0x21BB, 5, &base);
    lb_page_arrow_draw(0x21B0, work->page, work->page_count, work->page_moved, active);
}

/* 0x803933E8 (0x368): Draws the picked offer's prices: each price's item cell, its help record, the count it asks and
 * the count held, dimmed when the player holds too few. */
void lb_trade_cost_draw(LbTradeWork* work) {
    _SPR_DATA_ spr;
    char text[0x10];
    PlaceRec place;
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    LbTradeCost* cost;
    LbTradeOffer* offer;
    s32 i;
    s32 cursor;
    s32 lit;
    _SPR_DATA_* rect;
    bool active = work->state == 3;

    get_lsp_data(0x21EC, &base);
    draw_sprite_ary(lb_trade_cost_ids, &base);
    get_lsp_data(0x2213, &base);
    uv_pair_copy(&pos, &base);
    for (i = 0; i < 3; i++) {
        offer = (LbTradeOffer*)&((u32*)NetCtrlWk::getServerNotice())[work->rows[work->row_cursor].index * 4];
        cost = (LbTradeCost*)((u32*)offer + i + 1);

        if (cost->item != 0) {
            if (active) {
                draw_sprite_anim_ary(lb_trade_cost_row_ids, 0, &pos);
            } else {
                draw_sprite_anim_ary(lb_trade_cost_row_ids, 1, &pos);
            }
            if (i == work->cost_cursor && work->state != 2) {
                cursor = 1;
            } else {
                cursor = 0;
            }
            if (!active && work->rows[work->row_cursor].affordable[i] == 1) {
                lit = -1;
            } else {
                lit = 0;
            }
            if (work->state != 2) {
                lit = 0;
            }
            lb_item_cell_draw_wide(cost->item, lit, &pos, work->rows[work->row_cursor].affordable[i], cursor, active,
                                   0x280);
            place_rec_trade_set(&place, work->source->id, offer->item.item, offer->item.count, cost->item,
                                cost->count);
            rect = get_lsp_data(0x2213, NULL);
            place_rec_alloc(&place, pos.x, pos.y, rect->width, rect->height, 4, 14);
            sprintf(text, "%d", cost->count);
            spr_data_copy(&spr, get_lsp_data(0x222E, NULL));
            if (work->rows[work->row_cursor].affordable[i] == 0) {
                spr.color = 0x646464FF;
            }
            draw_font(spr, (s8*)text, 1, &pos);
            sprintf(text, "%d", userdata_item_count_total(cost->item, lobby_world_block));
            if (cursor != 0 && work->hold != 0) {
                draw_font_anim_idx(0x222F, 2, (s8*)text, 2, &pos);
            } else {
                spr_data_copy(&spr, get_lsp_data(0x222F, NULL));
                if (work->rows[work->row_cursor].affordable[i] == 0) {
                    spr.color = 0x646464FF;
                }
                draw_font(spr, (s8*)text, 2, &pos);
            }
        }
        get_lsp_data(lb_trade_cost_lsp[i], &pos);
        pos.x += base.x;
        pos.y += base.y;
    }
    font_flush();
    get_lsp_data(0x21EC, &base);
    if (work->state == 2) {
        draw_sprite_ary(lb_trade_cost_cursor_ids, &base);
    }
    draw_sprite_idx(0x2211, &base);
    if (work->state == 2) {
        draw_sprite_idx(0x2212, &base);
    }
    ainpc_page_mark_b_draw(0x2232, NULL);
}

#pragma optimization_level 4
/* 0x80393750 (0x16C): Draws the help panel's message and its second line for the current step (the yes/no while an
 * exchange runs). */
void lb_trade_msg_draw(LbTradeWork* work) {
    u16 msg = 0xFFFF;
    u16 sub = 0xFFFF;
    s32 color = 2;
    s32 line = 1;
    u8 state;
    s16 dest;

    switch (work->state) {
    case 2:
        dest = work->choice.cursor;
        if (dest == 0) {
            msg = 0x22;
        } else {
            msg = 0x2A;
            line = 2;
        }
        state = work->rows[work->row_cursor].state;
        if ((state & 2) != 0) {
            sub = 0x25;
        } else if ((state & 1) != 0) {
            if (dest == 0) {
                sub = 0x23;
            } else {
                sub = 0x24;
            }
        } else if ((state & 4) != 0) {
            sub = 0x31;
        }
        break;
    case 3:
        msg = 0x26;
        if (work->rows[work->row_cursor].affordable[work->cost_cursor] == 0) {
            sub = 0x27;
        }
        break;
    case 4:
    case 5:
        msg = 0x28;
        sub = 0x29;
        color = 5;
        lb_panel_yes_no_draw(0x1879, work->yes_no);
        break;
    }
    if (msg != 0xFFFF) {
        lb_panel_msg_draw(0x1877, msg);
        if (sub != 0xFFFF) {
            lb_panel_line_draw(0x1877, sub, color, line);
        }
    }
}

#pragma optimization_level reset
/* 0x803938BC (0xD8): The trade screen's draw callback: the frame, the opening choice or the list and prices, then
 * the help panel. */
void lb_trade_draw_task(void) {
    LbTradeWork* work = (LbTradeWork*)lobby_w.menu_0xAC;
    _mh_ivec2_ pos;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    lb_trade_frame_draw();
    if ((u32)(work->state - 2) > 3) {
        switch (work->state) {
        case 1:
            lb_choice_draw(&work->choice, 0x21B1, lb_trade_list_ids, lb_trade_choice_ids, 0x1877, 0);
            get_lsp_data(0x21B1, &pos);
            draw_sprite_idx(0x21BC, &pos);
            draw_sprite_idx(0x21BD, &pos);
            draw_sprite_idx(lb_trade_label_ids[0], &pos);
            break;
        }
    } else {
        lb_trade_list_draw(work);
        lb_trade_cost_draw(work);
    }
    lb_trade_msg_draw(work);
}

/* --------------------------------------------------------------------------------------------- *
 * The scene effect.
 * --------------------------------------------------------------------------------------------- */

/* The scene effect's model positions and emitter tables. */
Vec lb_scene_model_pos[5] = {
    {-374.0f, -40.0f, 1114.0f},
    {490.0f, -40.0f, 1853.0f},
    {-503.0f, -40.0f, 1820.0f},
    {-2960.0f, -40.0f, 370.0f},
    {-2960.0f, -40.0f, 370.0f},
};
Vec lb_scene_model_target[5] = {
    {-4150.0f, -40.0f, 2500.0f},
    {1300.0f, -40.0f, 5600.0f},
    {-4300.0f, -40.0f, 3400.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
};
LbSceneEmitter lb_scene_emitters_0[2] = {
    {0x0E, {0, 0, 0}, {-470.0f, -14.0f, 1070.0f}, 0x1111, 18, 12},
    {0xFF, {0, 0, 0}, {0.0f, 0.0f, 0.0f}, 0x0000, 0, 0},
};
LbSceneEmitter lb_scene_emitters_1[2] = {
    {0x0E, {0, 0, 0}, {500.0f, -14.0f, 1960.0f}, 0xC667, 18, 12},
    {0xFF, {0, 0, 0}, {0.0f, 0.0f, 0.0f}, 0x0000, 0, 0},
};
LbSceneEmitter lb_scene_emitters_2[3] = {
    {0x0E, {0, 0, 0}, {-560.0f, -14.0f, 1820.0f}, 0x0E39, 18, 12},
    {0x0F, {0, 0, 0}, {-530.0f, -24.0f, 1831.0f}, 0x1111, 70, 30},
    {0xFF, {0, 0, 0}, {0.0f, 0.0f, 0.0f}, 0x0000, 0, 0},
};
LbSceneEmitter lb_scene_emitters_3[5] = {
    {0x0E, {0, 0, 0}, {-2980.0f, -14.0f, 620.0f}, 0x0000, 18, 12},
    {0x0E, {0, 0, 0}, {-2080.0f, -14.0f, 500.0f}, 0x0B61, 18, 12},
    {0x0F, {0, 0, 0}, {-2100.0f, -24.0f, 410.0f}, 0x0E39, 70, 30},
    {0x0F, {0, 0, 0}, {-2700.0f, -24.0f, 510.0f}, 0x0000, 70, 30},
    {0xFF, {0, 0, 0}, {0.0f, 0.0f, 0.0f}, 0x0000, 0, 0},
};
LbSceneEmitter* lb_scene_emitter_tbls[6] = {
    lb_scene_emitters_0, lb_scene_emitters_1, lb_scene_emitters_2, lb_scene_emitters_3, NULL, NULL,
};

/* 0x80393994 (0xDC): Spawns the scene effect: five pooled models, released again when the pool runs short. */
void lb_scene_eft_spawn(void) {
    _EFT* self = (_EFT*)eft_res_slot_get(0x4C);
    LbSceneEftWork* work;
    s32 i;

    if (self == NULL) {
        return;
    }
    self->release_0x40 = lb_scene_eft_release;
    work = (LbSceneEftWork*)self->work_0x38;
    work->count = 5;
    self->area_0x44 = get_now_areano();
    for (i = 0; i < work->count; i++) {
        work->models[i] = (MHchar*)eft_res_model_get();
        if (work->models[i] == NULL) {
            eft_res_slot_release(self);
            return;
        }
    }
    self->source_0x30 = NULL;
    self->dispatch_0x34 = lb_scene_eft_step;
    self->field_0x03 = 0x36;
    eft_state_flags_set(self, 8, 0);
}

/* 0x80393A70 (0x7C): Releases the scene effect's models. */
void lb_scene_eft_release(_EFT* self) {
    s32 i;
    LbSceneEftWork* work = (LbSceneEftWork*)self->work_0x38;
    for (i = 0; i < work->count; i++) {
        fn_800E26C4(work->models[i]);
        fn_800F8A44((void*)&work->models[i], 1);
    }
}

/* 0x80393AEC (0x3C): Runs the arm the effect's state byte selects. */
void lb_scene_eft_step(_EFT* self) {
    switch (self->state_0x05) {
    case 0:
        lb_scene_eft_init(self);
        break;
    case 1:
        lb_scene_eft_move(self);
        break;
    case 2:
        lb_quest_board_state_next(self);
        break;
    case 3:
        lb_quest_board_effect_retire(self);
        break;
    }
}

/* 0x80393B28 (0x224): Sets the scene effect up: binds the five models (retires the effect if one cannot be made),
 * hides the ones whose lobby part is not open, places them, shows each one's own node and arms its timers. */
void lb_scene_eft_init(_EFT* self) {
    LbSceneEftWork* work = (LbSceneEftWork*)self->work_0x38;
    Vec* pos;
    s32 i;
    u32 nodes;
    u32 node;

    self->state_0x05++;
    for (i = 0; i < work->count; i++) {
        if (res_eft_model_create(work->models[i], 147, 43) == NULL) {
            lb_quest_board_effect_retire(self);
            return;
        }
    }
    self->flag_0x01 = 1;
    self->field_0x10 = 0;
    switch (get_now_mapno()) {
    case 22:
        if (self->area_0x44 == 0) {
            pos = lb_scene_model_pos;
        }
        break;
    }
    nodes = mhchar_node_count(work->models[0]);
    for (i = 0; i < work->count; i++) {
        switch (i) {
        case 0:
            if (lb_part_slot_open_ck(0) == 0) {
                work->models[i]->field_0x34 = 0;
            }
            break;
        case 1:
            if (lb_part_slot_open_ck(1) == 0) {
                work->models[i]->field_0x34 = 0;
            }
            break;
        case 2:
            if (lb_part_slot_open_ck(2) == 0) {
                work->models[i]->field_0x34 = 0;
            }
            break;
        case 3:
        case 4:
            if (kujira_event_over_ck() == 0) {
                work->models[i]->field_0x34 = 0;
            }
            break;
        }
        work->models[i]->pos_0x04.x = pos->x;
        work->models[i]->pos_0x04.y = pos->y;
        work->models[i]->pos_0x04.z = pos->z;
        pos++;
        work->models[i]->ready = 0;
        work->models[i]->frame_init(0, 0, 0.0f, 0, 1.0f);
        for (node = 0; node < nodes; node++) {
            if ((s32)node != i + 1) {
                work->models[i]->setVisibility(node, false);
            }
        }
        work->frames[i] = 0;
        work->timers[i][0] = 30;
        work->timers[i][1] = 30;
        work->timers[i][2] = 30;
        work->timers[i][3] = 30;
    }
    lb_scene_eft_move(self);
}

/* 0x80393D4C (0x2EC): Steps the scene effect's models: models 3 and 4 slide by the camera keys, the others fire their
 * emitters once the lobby part is up, model 1 travels to its target leaving a trail; then each model moves. */
void lb_scene_eft_move(_EFT* self) {
    s32 i;
    LbSceneEftWork* work = (LbSceneEftWork*)self->work_0x38;
    _CP_VECTOR rot;
    VEC3 from;
    VEC3 to;
    VEC3 pos;
    VEC3 step;
    LbQuestDetailModel* view;
    LbSceneEmitter* emitter;
    s32 j;
    s32 voice;

    rot.x = 0;
    rot.y = 0;
    rot.z = 0;
    VEC3_ctor(&from);
    VEC3_ctor(&to);
    VEC3_ctor(&pos);
    for (i = 0; i < work->count; i++) {
        view = (LbQuestDetailModel*)work->models[i];
        emitter = lb_scene_emitter_tbls[i];
        if (view->visible == 0) {
            continue;
        }
        if ((u32)(i - 3) <= 1) {
            if (lb_scene_model_slide(self, view, i) == 1) {
                continue;
            }
        } else {
            switch (view->step) {
            case 0:
                if (lobby_w.slots_0x07D[i] == 2) {
                    view->step++;
                    switch (i) {
                    default:
                        voice = 0;
                        break;
                    case 1:
                        voice = 2;
                        break;
                    case 2:
                        voice = 1;
                        break;
                    }
                    se_point_req(voice, &view->pos);
                }
                if (emitter != NULL) {
                    for (j = 0; emitter->kind != 0xFF; emitter++, j++) {
                        if (system_w.field_0x0c % work->timers[i][j] == 0) {
                            work->timers[i][j] = emitter->base + ran_suu(1) % emitter->spread;
                            rot.y = emitter->heading;
                            vec_to_mh_vec3(&pos, &emitter->pos);
                            eft028_set_scaled(emitter->kind, &pos, &rot, 1.0f, self->area_0x44, 0);
                        }
                    }
                }
                break;
            case 1:
                vec_to_mh_vec3(&from, &lb_scene_model_pos[i]);
                vec_to_mh_vec3(&to, &lb_scene_model_target[i]);
                subVec3(&step, &to, &from);
                copyVec3(&from, &step);
                vec3_scale_inv(&from, 1350.0f);
                addVec3To(&view->pos, &from);
                rot.y = calcVecAng2(&to, &from);
                if (++work->frames[i] > 1350) {
                    view->step++;
                }
                if (--work->timers[i][0] <= 0) {
                    work->timers[i][0] = 8;
                    eft028_set_scaled(12, &view->pos, &rot, 1.0f, self->area_0x44, 0);
                }
                break;
            case 2:
                continue;
            }
        }
        work->models[i]->move(0);
        eft_res_models_spawn(self, (void**)&work->models[i], 2, 1, 0);
    }
}

/* The camera model's key-frame table `lb_scene_model_slide` reads: six (frame, value) pairs. */
f32 lb_quest_detail_camera_keys[12] = {
    0.0f, -3160.0f, 82.0f, -3080.9f, 144.0f, -3035.2f, 212.0f, -2997.3f, 310.0f, -2960.0f, -1.0f, -2960.0f,
};

/* 0x80394038 (0x10C): Steps one of the scene effect's models: it waits for the lobby's camera mode 2, slides back when
 * the camera work stops, and for model 3 follows the key table by the camera's frame; 1 for model 4. */
u8 lb_scene_model_slide(_EFT* self, LbQuestDetailModel* model, u8 index) {
    switch (model->step) {
    case 0:
        if (lobby_w.field_0x003 == 2) {
            model->step++;
            setVector3(&model->pos, -3160.0f, -40.0f, 370.0f);
        }
        /* fall through */
    case 1:
        if (camera_work_ck() == 0) {
            setVector3(&model->pos, -2960.0f, -40.0f, 370.0f);
            model->step = 0;
            return 0;
        }
        if (index == 3) {
            model->pos.x = getKeyData(lb_quest_detail_camera_keys, (f32)(camera_frame_get(1) * 2));
        } else if (index == 4) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x80394144 (0x10): Advances the work's step byte.  GUESS: the step is what `lb_scene_eft_step`'s arm 2 runs. */
void lb_quest_board_state_next(_EFT* self) {
    self->state_0x05++;
}

/* 0x80394154 (0x4): Retires one pooled effect runtime record. */
void lb_quest_board_effect_retire(_EFT* self) {
    eft_res_slot_release(self);
}

}  /* extern "C" */
