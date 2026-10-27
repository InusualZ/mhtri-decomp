/* lobby/fn_8030121C.cpp - the whale trader's screen (lobby screen 11: the trade kinds menu, the two carts, the offer
 *   list, the hold box and the help panel), then the enemy effect spawner (`eft_em_*`, type 0x27), the debris effect
 *   (`eft_em_tint_*`, type 0x28) and the area effect's release and dispatcher (type 0x29, `eft041_*`).
 * RANGE. .text 0x8030121C-0x803066F0 (52 functions, defined in address order); .data 0x805DAFE8-0x805DBFB8 (the sprite
 *   tables, `eft_em_res_ids`/`_groups`, `eft_em_key_rgb_a..c`, then three jump tables), .sdata 0x80792A58-0x80792B70,
 *   .sdata2 0x8079AD98-0x8079ADE0, extab, extabindex.  `.data` and `.sdata` are emitted byte-identical by the tables
 *   defined at the top of the file.
 * FLAGS. `cflags_lobby`; file-scope `#pragma peephole off` and `pool_data off` (each table addressed by its own
 *   `lis`/`addi`, as in `lobby/fn_80212810.cpp`), `optimization_level 4` (retail's switches test case ranges with
 *   `subi`+`cmplwi`, playbook 108: `lb_screen11_draw`, `lb_barter_help_draw`, `eft_em_move`) and `fp_contract off`
 *   (retail keeps `fmuls`+`fadds` in `eft_em_tint_create`).
 * NAMES. No `__FILE__` string and no runtime-dump name covers the range.  GUESS (from each body, its callers and the
 *   effect type it seeds): every `lb_barter_*`/`lb_kujira_*`/`lb_box_*`/`lb_screen11_draw` function (the trader of
 *   `lobby/fn_802FA9A0.cpp`'s `lb_barter_*` family), `eft_em_*`, `eft_em_tint_*` ("tint" from the spawn's colour word;
 *   the pieces are flung debris models), `eft041_*`, and the tables `lb_*_ids`, `lb_barter_count_fmt`, `eft_em_res_*`,
 *   `eft_em_key_rgb_*`, `eft_tint_*`.  Registry names: `se_req_pos_impact`, `eft041_model_create/_move/_end/_kill`.
 * RESIDUALS.
 *   - .sdata2 is 56 bytes against 72: the range is two or more retail TUs (seam request lobby-fn-8030121c#1), whose
 *     pools each hold 0.0f and the signed int->float double; one source pools each value once.
 *   - `eft_em_tint_model_create`: the speed table's row pointer is loaded before the `0.0f` arguments in retail, after
 *     them in ours, and the case-0 visibility loops colour `part`/`end` and the piece pointer differently.
 *   - `eft_em_move` (`work`/`enemy` registers swapped), `eft_em_model_create` (`group`/`i` swapped), `lb_kujira_menu_draw`,
 *     `lb_barter_list_draw`, `lb_kujira_menu_step` (the ship-bit toggle's registers): register colouring that no
 *     declaration order reaches.
 *   - `lb_barter_cart_draw`: retail keeps a `clrlwi` on the first `get_lsp_data` id (a phi of two constants).
 *   - `lb_barter_gauge_draw`: the two share computations take one stack temporary in retail, two in ours (frame 0x60
 *     against 0x70).
 *   - foreign generated names this file cannot respell: none.
 *   flipcheck: .sdata2 (above) and .text short of byte-identical.
 * SHAPES. Tables before their users, all at the top (retail's `.data` holds every table ahead of the jump tables).  A
 *   one-value signed test is a one-case `switch` (`eft_em_create`'s id 44).  The camera height is read straight off
 *   `get_camera_pos()` (a named copy costs a frame slot).  The debris pieces are indexed (`work->pieces_0x04[i]`), not
 *   walked by pointer.  `GetMenuFontColor`'s selection flags are `bool` locals.  The item cell drawers are called
 *   through `s32`-parameter cast views (retail passes the raw words).  `lb_screen11_step`'s choice switch has its
 *   `default` body first.  A default-then-override assignment is a conditional expression (playbook 119), and a
 *   `subfc`/`subfe` compare plus `addi` is a ternary between consecutive constants (playbook 240).  The info rows'
 *   item is a `u16[2]` record (id, count) whose count is zeroed.  `lb_barter_pay_arrow_draw` takes the work it does
 *   not read (retail's caller passes it).  `eft_em_tint_create` takes its float before the colour and frame count.
 */
#include "types.h"
#include "ef.h"
#include "ef/eft_res.h"
#include "ef/push_eft_effect_heap_num.h"
#include "ef/effect.h"
#include "ef/fn_800CDB2C.h"
#include "hud/layout.h"
#include "stage/stg_w.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/printf.h"
#include "lobby/fn_802FA9A0.h"
#include "lobby/fn_8030121C.h"
#include "lobby/LbStr.h"
#include "lobby/lb_list_init.h"
#include "lobby/lb_cmd_pressed_ck.h"
#include "lobby/lb_talk_page_open.h"
#include "lobby/lb_cursor_prompt_draw.h"
#include "lobby/lb_button_prompt_draw.h"
#include "lobby/lb_text_center_print.h"
#include "lobby/lb_item_cell_draw_layout1.h"
#include "lobby/lb_item_cell_draw_layout8.h"
#include "lobby/lb_cancel_prompt_draw.h"
#include "lobby/lb_item_box_count_draw.h"
#include "lobby/lb_tr_flag_get.h"
#include "lobby/lb_tr_flag_data.h"
#include "ef/eft052.h"
#include "ef/eft052_item_get_open.h"
#include "menu/menu_message.h"
#include "menu/put_menu_cursor.h"
#include "menu/menu_item_row_draw.h"
#include "menu/get_pop_dat_ptr.h"
#include "hud/cockpit.h"
#include "hud/put_button_guide_c.h"
#include "hud/put_button_guide_c_idx.h"
#include "hud/draw_sprite_blink_idx.h"
#include "sound/fn_800D7F54.h"
#include "sound/set_zmode__FbUcb.h"
#include "camera/camera.h"
#include "font/flfnt.h"
#include "fn_80047398.h"
#include "menu/menu_item.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/em_common.h"
#include "enemy/fn_8012BDF4.h"
#include "stage/get_now_mapno.h"
#include "mh3_pad/vec3.h"
#include "nw4r/math.h"
#include "ef/mtx34_trans_add.h"
#include "ef/mtx34_trans_get.h"
#include "ef/eft001.h"
#include "rotLocalMatX.h"
#include "fn_8004CAD8.h"
#include "fn_8004CAD8/mtx.h"
#include "lobby/lb_server_sel_trans.h"
#include "sound/se.h"
#include "g3d/g3d_calcworld.h"
#include "sound/se_req_pos_impact.h"
#include "ef/eft041_model_create.h"
#include "ef/eft041_move.h"
#include "ef/eft041_end.h"
#include "ef/eft041_kill.h"
#include "stage/get_stg_eft_col.h"
#include "setScaleMat34W.h"
#include "enemy/fn_8011D448.h"
#include "gx.h"

/* `main.h` cannot be included here: its `_GXChannelID` enum clashes with the one `ef.h` pulls
 * in.  `ck_WideMode` is a C++ free function (`ck_WideMode__Fv`), so the declaration is at C++ scope. */
int ck_WideMode(void);

/* The effect-41/42 spawners' pooled work block (`_EFT::work_0x38`) as this band views it.  The count
 * and the two parallel arrays the machines walk: the `effect` slots at +0x04 (stride 0x8, handed to
 * `push_eft_effect_heap_num` / `eft_res_models_release` as a block) and the four g3d part handles at +0x0C
 * (stride 0x10).  size: 0x20 (approximate: the pooled block is larger). */
typedef struct Eft8030EffectSlot {
    /* +0x000 */ nw4r::ef::Effect* effect;
    /* +0x004 */ u8 pad_0x004[0x4];
} Eft8030EffectSlot; /* size: 0x8 */

typedef struct Eft8030ModelSlot {
    /* +0x000 */ void* parts_0x000[4];
} Eft8030ModelSlot; /* size: 0x10 */

typedef struct Eft8030Work {
    /* +0x000 */ s32 count;
    /* +0x004 */ Eft8030EffectSlot slots_0x004[1];
    /* +0x00C */ Eft8030ModelSlot models_0x00C[1];
    /* +0x01C */ u8* source_0x01C;
} Eft8030Work; /* size: 0x20 (approximate: the pooled block is larger) */

/* The enemy effect's pooled work block (`_EFT::work_0x38`, the 0x20 bytes `eft_res_slot_get` hands out): one effect,
 * its scale, the joint it follows, the offset from the joint and the step's parameter (an angle or a colour word).
 * size: 0x20 */
typedef struct EftEmWork {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ nw4r::ef::Effect* effect_0x04;
    /* +0x08 */ f32 scale_0x08;
    /* +0x0C */ u32 joint_0x0C;
    /* +0x10 */ nw4r::math::VEC3 offset_0x10;
    /* +0x1C */ u32 param_0x1C;
} EftEmWork; /* size: 0x20 */

/* One debris piece (`EftTintWork::pieces_0x04`): its fade state and alpha, its model, its velocity and size.
 * size: 0x18 */
typedef struct EftTintPiece {
    /* +0x00 */ u8 fade_0x00;     /* 1 once the record's timer is under 10 */
    /* +0x01 */ u8 alpha_0x01;
    /* +0x02 */ u8 pad_0x02[2];
    /* +0x04 */ MHchar* model_0x04;
    /* +0x08 */ VEC3 vel_0x08;
    /* +0x14 */ f32 size_0x14;
} EftTintPiece; /* size: 0x18 */

/* The debris effect's pooled work block (`_EFT::work_0x38`, the 0xE8 bytes `eft_res_slot_get` hands out): the
 * pieces, the effect, the joint, the fall step, the scale, the offset and the tint.  size: 0xE8 */
typedef struct EftTintWork {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ EftTintPiece pieces_0x04[8];
    /* +0xC4 */ s32 effect_count_0xC4;
    /* +0xC8 */ nw4r::ef::Effect* effect_0xC8;
    /* +0xCC */ u32 joint_0xCC;
    /* +0xD0 */ f32 fall_0xD0;
    /* +0xD4 */ f32 scale_0xD4;
    /* +0xD8 */ nw4r::math::VEC3 offset_0xD8;
    /* +0xE4 */ _GXColor color_0xE4;
} EftTintWork; /* size: 0xE8 */

#pragma peephole off
#pragma pool_data off
#pragma optimization_level 4
#pragma fp_contract off

extern "C" {

/* ------------------------------------------------------------------------------------------------
 * The trade screen's sprite tables (`.data`, then the short ones in `.sdata`), in address order.
 * ------------------------------------------------------------------------------------------------ */
/* 0x805DAFE8 */ u16 lb_barter_bg_ids[24] = {0x2302, 0x2303, 0x2304, 0x2309, 0x230A, 0x230B, 0x230C, 0x230D, 0x230E, 0x230F, 0x2310, 0x2311, 0x2312, 0x2313, 0x2314, 0x2315, 0x2305, 0x2306, 0x2307, 0x2308, 0x2300, 0x2301, 0xFFFF, 0x0000};
/* 0x805DB018 */ u16 lb_barter_title_ids[6] = {0x2319, 0x231A, 0x2317, 0x2318, 0xFFFF, 0x0000};
/* 0x805DB024 */ u16 lb_barter_title_wide_ids[6] = {0x231E, 0x231F, 0x231C, 0x231D, 0xFFFF, 0x0000};
/* 0x805DB030 */ u16 lb_barter_tab_frame_ids[12] = {0x23DA, 0x23DB, 0x23DC, 0x23DD, 0x23DE, 0x23DF, 0x23E0, 0x23E1, 0x23E2, 0x23E3, 0x23E4, 0xFFFF};
/* 0x805DB048 */ u16 lb_barter_tab_row_ids[6] = {0x2337, 0x2338, 0x2339, 0x2336, 0xFFFF, 0x0000};
/* 0x805DB054 */ u16 lb_barter_tab_cursor_ids[6] = {0x233D, 0x233A, 0x233B, 0x233C, 0xFFFF, 0x0000};
/* 0x805DB060 */ u16 lb_barter_tab_bg_ids[10] = {0x23EE, 0x23EF, 0x23E9, 0x23EA, 0x23EB, 0x23EC, 0x23ED, 0x23E7, 0x23E8, 0xFFFF};
/* 0x805DB074 */ u16 lb_barter_tab_panel_ids[10] = {0x2394, 0x2395, 0x2396, 0x2397, 0x2398, 0x2399, 0x239A, 0x239B, 0xFFFF, 0x0000};
/* 0x805DB088 */ u16 lb_barter_cart_frame_ids[16] = {0x23BF, 0x23C0, 0x23C1, 0x23C2, 0x23C3, 0x23C4, 0x23C5, 0x23C6, 0x23C7, 0x23C8, 0x23C9, 0x23CA, 0xFFFF, 0x0000, 0x0000, 0x0000};
/* 0x805DB0A8 */ u16 lb_barter_cart_bg_ids[16] = {0x2375, 0x2376, 0x2377, 0x2378, 0x2379, 0x237A, 0x237B, 0x237C, 0x237D, 0x236F, 0x2370, 0x2371, 0x2372, 0x2373, 0x2374, 0xFFFF};
/* 0x805DB0C8 */ u16 lb_barter_cart_cell_ids[10] = {0x237E, 0x237F, 0x2380, 0x2381, 0x2382, 0x2383, 0x2384, 0x2385, 0x2386, 0x2387};
/* 0x805DB0DC */ u16 lb_barter_cart_cursor_ids[6] = {0x2352, 0x234F, 0x2350, 0x2351, 0x2351, 0xFFFF};
/* 0x805DB0E8 */ u16 lb_barter_gauge_frame_ids[16] = {0x2359, 0x235A, 0x235B, 0x235C, 0x235D, 0x235E, 0x235F, 0x2360, 0x2363, 0x2362, 0x2355, 0x2356, 0x2357, 0x2358, 0xFFFF, 0x0000};
/* 0x805DB108 */ u16 lb_barter_pip_on_ids[6] = {0x2369, 0x236A, 0x236B, 0x236C, 0x236D, 0xFFFF};
/* 0x805DB114 */ u16 lb_barter_pip_ids[6] = {0x2364, 0x2365, 0x2366, 0x2367, 0x2368, 0x0000};
/* 0x805DB120 */ u16 lb_barter_arrow_ids[10] = {0x23CE, 0x23CF, 0x23D0, 0x23D1, 0x23D2, 0x23D3, 0x23D4, 0x23D5, 0xFFFF, 0x0000};
/* 0x805DB134 */ u16 lb_barter_row_frame_ids[10] = {0x240F, 0x2410, 0x2411, 0x2412, 0x2413, 0x2414, 0x2415, 0x2416, 0x2417, 0xFFFF};
/* 0x805DB148 */ u16 lb_barter_row_gauge_ids[6] = {0x240B, 0x240C, 0x240D, 0x240E, 0x2405, 0xFFFF};
/* 0x805DB154 */ u16 lb_barter_row_pip_ids[6] = {0x2406, 0x2407, 0x2408, 0x2409, 0x240A, 0xFFFF};
/* 0x805DB160 */ u16 lb_barter_row_pip_frame_ids[6] = {0x240B, 0x240C, 0x240D, 0x240E, 0xFFFF, 0x0000};
/* 0x805DB16C */ u16 lb_barter_list_frame_ids[18] = {0x2424, 0x2425, 0x2426, 0x2427, 0x2428, 0x2429, 0x241B, 0x241C, 0x241D, 0x2422, 0x2423, 0x241E, 0x241F, 0x2420, 0x2421, 0x2419, 0x241A, 0xFFFF};
/* 0x805DB190 */ u16 lb_barter_list_row_ids[6] = {0x242A, 0x242B, 0x242C, 0x242D, 0x242E, 0x0000};
/* 0x805DB19C */ u16 lb_barter_goods_frame_ids[10] = {0x0096, 0x0097, 0x0098, 0x0099, 0x009D, 0x0093, 0x0094, 0x0095, 0xFFFF, 0x0000};
/* 0x805DB1B0 */ u16 lb_box_cell_ids[12] = {0x2484, 0x2485, 0x2486, 0x2487, 0x2488, 0x2489, 0x248A, 0x248B, 0x248C, 0x248D, 0xFFFF, 0x0000};
/* 0x805DB1C8 */ u16 lb_box_frame_ids[8] = {0x248F, 0x2490, 0x2491, 0x2492, 0x2493, 0x2494, 0xFFFF, 0x0000};
/* 0x805DB1D8 */ u16 lb_screen11_choice_ids[12] = {0x232A, 0x232B, 0x232C, 0x2326, 0x2327, 0x2328, 0x2329, 0x2322, 0x2323, 0x2324, 0x2325, 0xFFFF};
/* 0x805DB1F0 */ u16 lb_kujira_box_bg_ids[26] = {0x2481, 0x2482, 0x2477, 0x246B, 0x246A, 0x246D, 0x246E, 0x246F, 0x2470, 0x2471, 0x2472, 0x2473, 0x2474, 0x2475, 0x2476, 0x2478, 0x2479, 0x247A, 0x247B, 0x247C, 0x247D, 0x247E, 0x247F, 0x2480, 0x246C, 0xFFFF};
/* 0x805DB224 */ u16 lb_barter_got_bg_ids[26] = {0x243C, 0x243D, 0x243E, 0x243F, 0x2440, 0x2441, 0x2442, 0x2443, 0x2444, 0x2445, 0x2446, 0x2447, 0x2448, 0x2449, 0x2431, 0x2432, 0x2433, 0x2434, 0x2435, 0x2436, 0x2437, 0x2438, 0x2439, 0x243A, 0x243B, 0xFFFF};
/* 0x805DB258 */ u16 lb_barter_got_text_ids[6] = {0x244B, 0x2450, 0x2451, 0x2452, 0xFFFF, 0x0000};
/* 0x805DB264 */ u16 lb_barter_got_title_ids[10] = {0x245E, 0x245D, 0x245F, 0x2460, 0x2461, 0x2462, 0x2463, 0x2464, 0x2465, 0xFFFF};
/* 0x805DB278 */ u16 lb_kujira_menu_panel_ids[8] = {0x23A9, 0x23AA, 0x23AB, 0x23AC, 0x23AD, 0x23AE, 0x23AF, 0xFFFF};
/* 0x805DB288 */ u16 lb_kujira_kind_row_ids[6] = {0x2341, 0x2342, 0x2343, 0x2340, 0xFFFF, 0x0000};
/* 0x805DB294 */ u16 lb_kujira_kind_cursor_ids[6] = {0x2349, 0x2346, 0x2347, 0x2348, 0xFFFF, 0x0000};
/* 0x805DB2A0 */ u16 lb_kujira_days_ids[16] = {0x22F8, 0x22F0, 0x22F1, 0x22F2, 0x22F3, 0x22F4, 0x22F5, 0x22F6, 0x22F7, 0x22F9, 0x22FA, 0x22FB, 0x22FC, 0x22FD, 0x22FE, 0xFFFF};
/* 0x80792A58 */ u16 lb_barter_cart_blink_ids[4] = {0x23B7, 0x23B8, 0xFFFF, 0x0000};
/* 0x80792A60 */ u16 lb_barter_cart_blink2_ids[4] = {0x23B9, 0x23BA, 0xFFFF, 0x0000};
/* 0x80792A68 */ u16 lb_barter_ready_ids[2] = {0x23BB, 0xFFFF};
/* 0x80792A6C */ char lb_barter_count_fmt[3] = "%d";
/* 0x80792A70 */ u16 lb_barter_cart_cover_ids[4] = {0x23BD, 0x23BE, 0xFFFF, 0x0000};
/* 0x80792A78 */ u16 lb_screen11_choice_rows[4] = {0x232D, 0x232E, 0x232F, 0x0000};
/* 0x80792A80 */ u16 lb_barter_got_icon_ids[4] = {0x2459, 0x2457, 0x2458, 0xFFFF};
/* 0x80792A88 */ u16 lb_kujira_kind_count_ids[4] = {0x23A2, 0x23A2, 0x23A1, 0x23A0};
/* 0x80792A90 */ u16 lb_kujira_kind_row_pos_ids[4] = {0x23B0, 0x23B1, 0x23B2, 0x0000};
/* 0x80792A98 */ u16 lb_kujira_kind_name_ids[4] = {0x238C, 0x2390, 0x2391, 0xFFFF};

/* The enemy effect's resource id and group of each effect id (`eft_em_model_create`). */
/* 0x805DB2C0 */ u16 eft_em_res_ids[210] = {
    0x014B, 0x014C, 0x0143, 0x01AB, 0x01DE, 0x01DF, 0x01E0, 0x01E1, 0x01E2, 0x01E5, 0x01E9, 0x01EA,
    0x01FB, 0x01FC, 0x01F9, 0x01FA, 0x0207, 0x020D, 0x0238, 0x0239, 0x0283, 0x0284, 0x0284, 0x0284,
    0x01E4, 0x02B2, 0x02B3, 0x02BB, 0x02DC, 0x02DD, 0x02DE, 0x033A, 0x033C, 0x033D, 0x0347, 0x0348,
    0x0360, 0x03AD, 0x03AE, 0x03B0, 0x03B1, 0x03B6, 0x03B2, 0x03B3, 0x03B4, 0x03F4, 0x03F4, 0x03F4,
    0x03ED, 0x03EE, 0x03EF, 0x040A, 0x040B, 0x040C, 0x0417, 0x0423, 0x0424, 0x045E, 0x045F, 0x0460,
    0x0461, 0x0462, 0x0463, 0x0464, 0x0465, 0x0466, 0x0467, 0x03AF, 0x0484, 0x0485, 0x048A, 0x04BC,
    0x04CC, 0x04D4, 0x04D5, 0x04D6, 0x04ED, 0x04EF, 0x04F0, 0x050F, 0x0515, 0x052D, 0x052E, 0x052F,
    0x00FF, 0x055C, 0x05DB, 0x05DC, 0x05DF, 0x05E0, 0x05E4, 0x05EC, 0x05F0, 0x05F2, 0x05F1, 0x05F9,
    0x05FD, 0x05FF, 0x0606, 0x0607, 0x0621, 0x0530, 0x0531, 0x0534, 0x0628, 0x063D, 0x063E, 0x063D,
    0x063E, 0x0685, 0x0686, 0x0687, 0x0688, 0x0689, 0x068A, 0x068B, 0x068C, 0x0691, 0x0692, 0x069A,
    0x069B, 0x06A8, 0x06A9, 0x06AB, 0x06AF, 0x06B0, 0x06B5, 0x06C8, 0x0735, 0x0736, 0x0737, 0x0738,
    0x0739, 0x073A, 0x073B, 0x073E, 0x0791, 0x0792, 0x0806, 0x0807, 0x0808, 0x0809, 0x080A, 0x080B,
    0x0883, 0x088B, 0x0896, 0x0897, 0x0898, 0x0899, 0x089A, 0x089B, 0x089C, 0x089D, 0x098D, 0x098E,
    0x098F, 0x09F8, 0x09F9, 0x09FA, 0x09FE, 0x09FF, 0x0A00, 0x0A01, 0x0A02, 0x0A03, 0x0A04, 0x0A05,
    0x0A06, 0x0A07, 0x0A0D, 0x0A0F, 0x0A0A, 0x0A12, 0x0A17, 0x0A18, 0x0A1B, 0x0A1C, 0x0A1E, 0x0A1F,
    0x0A20, 0x0A22, 0x0A25, 0x0600, 0x0806, 0x0A29, 0x0A38, 0x0A39, 0x0A3A, 0x0A3B, 0x0A3C, 0x0A3D,
    0x0A3E, 0x0A3F, 0x0A40, 0x0A41, 0x01A9, 0x01AB, 0x0894, 0x0A42, 0x0A44, 0x0A45, 0x0A46, 0x0A47,
    0x01A9, 0x048D, 0x0A49, 0x0A40, 0x014C, 0x03ED,
};
/* 0x805DB464 */ u16 eft_em_res_groups[210] = {
    0x0022, 0x0022, 0x0024, 0x0024, 0x0032, 0x0032, 0x0032, 0x0032, 0x0032, 0x0033, 0x0033, 0x0033,
    0x0033, 0x0033, 0x0033, 0x0033, 0x0026, 0x0026, 0x0033, 0x0033, 0x0032, 0x0032, 0x0032, 0x0032,
    0x0033, 0x0042, 0x0042, 0x0043, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0042, 0x0042,
    0x004C, 0x0043, 0x0043, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0032, 0x0032, 0x0032,
    0x0024, 0x0024, 0x0024, 0x0022, 0x0022, 0x0032, 0x0043, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045,
    0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x005B, 0x005B, 0x004C, 0x005E,
    0x0043, 0x0026, 0x0026, 0x0026, 0x005B, 0x0022, 0x0022, 0x005A, 0x004C, 0x0062, 0x0062, 0x0062,
    0x0062, 0x005B, 0x0042, 0x0042, 0x0042, 0x0042, 0x005A, 0x006F, 0x0045, 0x0045, 0x0045, 0x0070,
    0x0070, 0x0070, 0x0071, 0x0071, 0x0071, 0x0062, 0x0062, 0x0062, 0x006F, 0x0042, 0x0042, 0x0042,
    0x0042, 0x005E, 0x005E, 0x005E, 0x006F, 0x006F, 0x006F, 0x006F, 0x006F, 0x0042, 0x0042, 0x0071,
    0x0071, 0x0043, 0x0043, 0x0043, 0x0072, 0x0072, 0x006F, 0x005B, 0x0043, 0x0043, 0x005B, 0x005B,
    0x006F, 0x0071, 0x0071, 0x0071, 0x0070, 0x0070, 0x006F, 0x006F, 0x006F, 0x006F, 0x006F, 0x005B,
    0x005B, 0x00A1, 0x0070, 0x0070, 0x0070, 0x0070, 0x0070, 0x0070, 0x0070, 0x0070, 0x005B, 0x005B,
    0x005B, 0x006F, 0x006F, 0x006F, 0x006F, 0x006F, 0x006F, 0x006F, 0x006F, 0x0062, 0x0062, 0x0070,
    0x0070, 0x00A1, 0x00A1, 0x00A1, 0x00A1, 0x006F, 0x0071, 0x00A1, 0x0062, 0x0062, 0x0070, 0x0070,
    0x0070, 0x005A, 0x0062, 0x0070, 0x006F, 0x0070, 0x0071, 0x0071, 0x00A1, 0x0070, 0x0070, 0x0070,
    0x0070, 0x0070, 0x0070, 0x0070, 0x0022, 0x0022, 0x0070, 0x0043, 0x0070, 0x0070, 0x005B, 0x005B,
    0x0022, 0x004C, 0x006F, 0x0070, 0x0022, 0x0024,
};

/* The colour key strips of the effects 21-23 and 45-47 (`eftGetKeyRGB`: frame, R, G, B per key). */
/* 0x805DB608 */ u8 eft_em_key_rgb_a[16] = {0x00, 0x96, 0xFF, 0xDF, 0x05, 0x7F, 0xFF, 0x9F, 0x0A, 0x64, 0xBF, 0x7F, 0xFF, 0x64, 0xBF, 0x7F};
/* 0x805DB618 */ u8 eft_em_key_rgb_b[16] = {0x00, 0xFF, 0x60, 0x00, 0x05, 0xFF, 0x8C, 0x00, 0x0A, 0xFF, 0x58, 0x00, 0xFF, 0xFF, 0x58, 0x00};
/* 0x805DB628 */ u8 eft_em_key_rgb_c[16] = {0x00, 0xFF, 0x40, 0x40, 0x05, 0xFF, 0x20, 0x20, 0x0A, 0xFF, 0x00, 0x00, 0xFF, 0x64, 0x00, 0x00};

/* The debris effect's tables (`.sdata`): per variant (`kind` 0 the plain pieces, 1 the splash variant) and per effect
 * id, the launch speed and the fall step of the two piece groups, the drag of each group, the piece sizes, the spin
 * masks, the piece counts, the effect and model ids and the model part ranges. */
/* 0x80792AA0 */ f32 eft_tint_speed_a0[2] = {40.0f, 40.0f};
/* 0x80792AA8 */ f32 eft_tint_speed_b0[2] = {80.0f, 80.0f};
/* 0x80792AB0 */ f32 eft_tint_fall_a0[2] = {3.0f, 5.0f};
/* 0x80792AB8 */ f32 eft_tint_fall_b0[2] = {4.0f, 6.0f};
/* 0x80792AC0 */ f32 eft_tint_speed_a1[2] = {30.0f, 0.0f};
/* 0x80792AC8 */ f32 eft_tint_speed_b1[2] = {40.0f, 0.0f};
/* 0x80792AD0 */ f32 eft_tint_fall_a1[2] = {0.5f, 0.0f};
/* 0x80792AD8 */ f32 eft_tint_fall_b1[2] = {0.5f, 1.0f};
/* 0x80792AE0 */ f32 eft_tint_drag_a0[2] = {0.9f, 0.93f};
/* 0x80792AE8 */ f32 eft_tint_drag_b0[2] = {0.86f, 0.97f};
/* 0x80792AF0 */ f32 eft_tint_drag_a1[2] = {0.9f, 0.0f};
/* 0x80792AF8 */ f32 eft_tint_drag_b1[2] = {0.9f, 0.0f};
/* 0x80792B00 */ f32* eft_tint_drag_a[2] = {eft_tint_drag_a0, eft_tint_drag_a1};
/* 0x80792B08 */ f32* eft_tint_drag_b[2] = {eft_tint_drag_b0, eft_tint_drag_b1};
/* 0x80792B10 */ f32* eft_tint_speed_a[2] = {eft_tint_speed_a0, eft_tint_speed_a1};
/* 0x80792B18 */ f32* eft_tint_speed_b[2] = {eft_tint_speed_b0, eft_tint_speed_b1};
/* 0x80792B20 */ f32* eft_tint_fall_a[2] = {eft_tint_fall_a0, eft_tint_fall_a1};
/* 0x80792B28 */ f32* eft_tint_fall_b[2] = {eft_tint_fall_b0, eft_tint_fall_b1};
/* 0x80792B30 */ f32 eft_tint_sizes[2] = {1.2f, 0.5f};
/* 0x80792B38 */ u16 eft_tint_spin_mask0[2] = {0x1FFF, 0x0FFF};
/* 0x80792B3C */ u16 eft_tint_spin_mask1[2] = {0x0FFF, 0x0000};
/* 0x80792B40 */ u16* eft_tint_spin_masks[2] = {eft_tint_spin_mask0, eft_tint_spin_mask1};
/* 0x80792B48 */ u8 eft_tint_piece_counts[4] = {8, 6, 0, 0};
/* 0x80792B4C */ u8 eft_tint_group_a_counts[4] = {4, 3, 0, 0};
/* 0x80792B50 */ u16 eft_tint_res_ids[4] = {0x0171, 0x01E8, 0x0616, 0x0000};
/* 0x80792B58 */ u8 eft_tint_res_groups[4] = {0x2B, 0x33, 0x71, 0x00};
/* 0x80792B5C */ u8 eft_tint_model_ids[4] = {0x36, 0x3C, 0x36, 0x00};
/* 0x80792B60 */ s8 eft_tint_part_first[4] = {1, 1, 1, 0};
/* 0x80792B64 */ s8 eft_tint_part_end[4] = {3, 5, 3, 0};
/* 0x80792B68 */ s8 eft_tint_part_mid[8] = {2, 3, 2, 0, 0, 0, 0, 0};

/* The range's own entry points, declared here because the callers precede the definitions. */
void lb_screen11_draw(void);
_EFT* eft_em_create(struct _ENEMY_WORK* self, u8 id, u32 joint, nw4r::math::VEC3* pos, f32 scale);
void eft_em_release(_EFT* self);
void eft_em_dispatch(_EFT* self);
void eft_em_model_create(_EFT* self);
void eft_em_move(_EFT* self);
void eft_em_end(_EFT* self);
void eft_em_kill(_EFT* self);
_EFT* eft_em_tint_create(struct _ENEMY_WORK* self, u8 id, u32 joint, nw4r::math::VEC3* pos, f32 scale, u32 color,
                         s32 life);
void eft_em_tint_release(_EFT* self);
void eft_em_tint_dispatch(_EFT* self);
void eft_em_tint_model_create(_EFT* self);
void eft_em_tint_move(_EFT* self);
void eft_em_tint_end(_EFT* self);
void eft_em_tint_kill(_EFT* self);
void eft041_release(_EFT* self);
void eft041_dispatch(_EFT* self);

/* `stage/fn_802B2AA0.h` declares it, but its `eft_vec_ctor` clashes with `ef.h`'s, so the declaration is the same
 * spelling here. */
u8* stage_area_word_get(u8 kind);


/* ------------------------------------------------------------------------------------------------
 * The trade screen (lobby screen 11): steps and draws
 * ------------------------------------------------------------------------------------------------ */

/* 0x8030121C (0x4F0): steps the whale trader's menu - the trade kinds, the gift item and the confirmation. */
s32 lb_kujira_menu_step(LbBarterWork* work) {
    s32 result = 0;
    LbKujiraWork* kujira;
    u16 row;

    switch (work->step_0x001) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            switch (work->menu_0x009[work->cursor_0x032]) {
            case 4: {
                s16 count = 0;
                s16 i;
                for (i = 0; i < work->menu_count_0x036; i++) {
                    if (work->ship_bits_0x003 & (1 << i)) {
                        count++;
                    }
                }
                if (count != 0) {
                    work->step_0x001 = 2;
                    work->dialog_0x050 = 0;
                    sysSE_req(0);
                } else {
                    sysSE_req(2);
                }
                break;
            }
            case 3:
                lb_barter_hold_open(work);
                work->step_0x001 = 1;
                sysSE_req(5);
                break;
            default:
                lb_barter_refs_kind_build(work, work->menu_0x009[work->cursor_0x032]);
                break;
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            if (work->field_0x006 == 0) {
                result = 2;
            } else {
                work->step_0x001 = 3;
                work->dialog_0x050 = 0;
            }
            sysSE_req(1);
        } else if (lb_cmd_pressed_ck(0x80) != 0) {
            u8* kind = &work->menu_0x009[work->cursor_0x032];
            if (*kind <= 2) {
                work->step_0x001 = 4;
                work->cursor_b_0x042 = 0;
                lb_barter_refs_kind_build(work, *kind);
                lb_barter_page_fill_kind(work, 0);
                sysSE_req(5);
            }
        } else if (lb_cmd_repeat_ck(3) != 0) {
            s16 count = 0;
            s16 i;
            for (i = 0; i < work->menu_count_0x036; i++) {
                if (work->ship_bits_0x003 & (1 << i)) {
                    count++;
                }
            }
            if (count != 0) {
                work->cursor_0x032 = menu_cursor_step(work->cursor_0x032, work->rows_0x034, lb_cmd_repeat_get(), 1, 2);
            } else {
                work->cursor_0x032 = menu_cursor_step(work->cursor_0x032, work->rows_0x034 - 1, lb_cmd_repeat_get(), 1, 2);
            }
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            u8 kind = work->menu_0x009[work->cursor_0x032];
            if (kind <= 2) {
                s32 bit = 1 << kind;
                if (bit & work->ship_bits_0x003) {
                    work->ship_bits_0x003 = work->ship_bits_0x003 & ~bit;
                } else {
                    work->ship_bits_0x003 |= bit;
                }
                work->field_0x006 = 1;
                work->moved_0x044 = lb_cmd_repeat_get() & 0xC;
                sysSE_req(6);
            }
        }
        break;
    case 4:
        if (lb_cmd_pressed_ck(0xA0) != 0) {
            work->step_0x001 = 0;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            work->cursor_b_0x042 = menu_cursor_step(work->cursor_b_0x042, work->shown_0x04C, lb_cmd_repeat_get(), 1, 2);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            work->page_0x03C = menu_cursor_step_fixed_tail(work->page_0x03C, work->pages_0x03E, lb_cmd_repeat_get(), 4, 8,
                                                           &work->moved_0x044);
            lb_barter_page_fill_kind(work, work->page_0x03C);
        }
        break;
    case 1:
        switch (eft052_hold_step()) {
        case 1:
            eft052_hold_row_get(&row, NULL);
            if (work->item_0x004 == row) {
                work->item_0x004 = 0;
            } else {
                work->item_0x004 = row;
            }
            work->step_0x001 = 0;
            work->field_0x006 = 1;
            break;
        case 2:
            work->step_0x001 = 0;
            break;
        }
        break;
    case 2:
        switch (((s32 (*)(u32*, u16, s32, s32, u16))toggle_word_step)(&work->dialog_0x050, lobby_cmd_trig_get(), 4, 8,
                                                                    0xFFFF)) {
        case 1:
            result = 1;
            kujira = &lobby_world_block->work_0x4832;
            kujira->field_0x03 = 1;
            kujira->item_0x06 = work->item_0x004;
            eft052_page_count_add(work->item_0x004, 1);
            kujira->bits_0x01 = work->ship_bits_0x003;
            lb_list_init((LbList*)&work->mode_0x010, 3, 0, 0x183, 0x13B, 0, -1, 3, 0);
            work->mode_0x010 = 2;
            sysSE_stop(0x2C);
            break;
        case 2:
            work->step_0x001 = 0;
            break;
        }
        break;
    case 3:
        switch (lb_yes_no_step(&work->dialog_0x050)) {
        case 1:
            result = 2;
            break;
        case 2:
            work->step_0x001 = 0;
            break;
        }
        break;
    }
    return result;
}

/* 0x8030170C (0x40): opens the trade itself - the offer references for `mask`, then the stock reset. */
void lb_barter_trade_open(LbBarterWork* work, u8* mask) {
    work->step_0x001 = 0;
    work->ready_0x058 = 0;
    lb_barter_refs_build(work, mask);
    lb_barter_reset(work);
}

/* 0x8030174C (0x2D4): steps lobby screen 11 - the trader's choice list, the two trades, the whale menu and the close. */
void lb_screen11_step(void) {
    LbBarterWork* work = lobby_w.menu_0x0AC;
    LbKujiraWork* kujira = &lobby_world_block->work_0x4832;

    work->moved_0x044 = 0;
    switch (work->type_0x000) {
    case 0:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            work->type_0x000++;
            if (kujira->field_0x03 == 0) {
                lb_list_init((LbList*)&work->mode_0x010, 3, 0, 0x180, 0x138, 0, -1, 3, 0);
            } else {
                lb_list_init((LbList*)&work->mode_0x010, 3, 0, 0x183, 0x13B, 0, -1, 3, 0);
            }
            camera_talk_lock_set(1);
            sysSE_stop(0x29);
        }
        break;
    case 1:
        ainpc_page_hold_set();
        switch (lb_choice_step((LbList*)&work->mode_0x010)) {
        case 1:
            work->step_0x001 = 0;
            work->tab_0x030 = 0;
            sysSE_stop(0xE);
            switch (work->mode_0x010) {
            default:
                work->type_0x000 = 2;
                lb_barter_restart(work, &kujira->flag_0x00);
                break;
            case 1:
                work->type_0x000 = 3;
                lb_barter_trade_open(work, &kujira->flag_0x00);
                break;
            case 2:
                if (kujira->field_0x03 == 0) {
                    work->type_0x000 = 4;
                } else {
                    work->type_0x000 = 5;
                }
                lb_barter_menu_init(work);
                break;
            }
            break;
        case 2:
            work->type_0x000 = 6;
            lb_talk_page_open(1);
            camera_talk_lock_set(0);
            break;
        }
        break;
    case 3:
        ainpc_page_hold_set();
        if ((u32)(lb_barter_step(work, &kujira->flag_0x00) - 1) <= 1) {
            work->type_0x000 = 1;
        }
        break;
    case 2:
        ainpc_page_hold_set();
        if ((u32)(lb_barter_step(work, &kujira->flag_0x00) - 1) <= 1) {
            work->type_0x000 = 1;
        }
        break;
    case 4:
        ainpc_page_hold_set();
        if ((u32)(lb_kujira_menu_step(work) - 1) <= 1) {
            work->type_0x000 = 1;
        }
        break;
    case 5:
        ainpc_page_hold_set();
        if (lb_cmd_pressed_ck(0x20) != 0) {
            sysSE_req(1);
            work->type_0x000 = 1;
        }
        break;
    case 6:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            lb_barter_close();
        }
        break;
    default:
        if (lb_cmd_pressed_ck(0x20) != 0) {
            lb_barter_close();
            sysSE_req(1);
        }
        break;
    }
    if (work->type_0x000 != 0 && work->type_0x000 != 6) {
        subTransSet((u32)lb_screen11_draw, 0, NULL);
    }
}

/* 0x80301A20 (0x98): draws the screen's background and title (the wide-screen title in 16:9). */
void lb_barter_bg_draw(LbBarterWork* work) {
    _mh_ivec2_ pos;

    if (ck_WideMode()) {
        get_lsp_data(0x231B, &pos);
        draw_sprite_ary(lb_barter_title_wide_ids, &pos);
    } else {
        get_lsp_data(0x2316, &pos);
        draw_sprite_ary(lb_barter_title_ids, &pos);
    }
    if (work->type_0x000 <= 1) {
        get_lsp_data(0x22FF, &pos);
        draw_sprite_ary(lb_barter_bg_ids, &pos);
    }
}

/* 0x80301AB8 (0x338): draws the trade's tab column - the two cart tabs and the settle prompt; `active` 1 while the
 * tabs take the cursor. */
void lb_barter_tabs_draw(LbBarterWork* work, u32 active) {
    _mh_ivec2_ base;
    _mh_ivec2_ panel;
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    bool sel;
    u16 str;
    u32 flags;

    get_lsp_data(0x23E6, &base);
    draw_sprite_anim_ary(lb_barter_tab_bg_ids, work->mode_0x010 != 0, &base);
    get_lsp_data(0x23D9, &base);
    draw_sprite_anim_ary(lb_barter_tab_frame_ids, 0, &base);
    get_lsp_data(0x23E5, &panel);
    panel.x += base.x;
    panel.y += base.y;
    draw_sprite_ary(lb_barter_tab_panel_ids, &panel);

    get_lsp_data(0x239C, &pos);
    pos.x += panel.x;
    pos.y += panel.y;
    draw_sprite_anim_ary(lb_barter_tab_row_ids, 0, &pos);
    if (active == 1) {
        if (work->tab_0x030 == 0) {
            sel = 1;
        } else {
            sel = 0;
        }
    } else {
        sel = 0;
    }
    str = work->mode_0x010 == 0 ? 0x187 : 0x189;
    spr_data_copy(&spr, get_lsp_data(0x2335, NULL));
    spr.color = GetMenuFontColor(1, sel, 1, 0);
    if (work->tab_0x030 == 0 && active == 1) {
        put_menu_cursor(lb_barter_tab_cursor_ids, 0, &pos);
        flags = 5;
    } else {
        flags = 1;
    }
    draw_font(spr, (s8*)LbStr(0, str), flags, &pos);

    get_lsp_data(0x239D, &pos);
    pos.x += panel.x;
    pos.y += panel.y;
    draw_sprite_anim_ary(lb_barter_tab_row_ids, 1, &pos);
    if (active == 1) {
        if (work->tab_0x030 == 1) {
            sel = 1;
        } else {
            sel = 0;
        }
    } else {
        sel = 0;
    }
    str = work->mode_0x010 == 0 ? 0x186 : 0x188;
    spr_data_copy(&spr, get_lsp_data(0x2335, NULL));
    spr.color = GetMenuFontColor(1, sel, 1, 0);
    if (work->tab_0x030 == 1 && active == 1) {
        put_menu_cursor(lb_barter_tab_cursor_ids, 0, &pos);
        flags = 5;
    } else {
        flags = 1;
    }
    draw_font(spr, (s8*)LbStr(0, str), flags, &pos);

    get_lsp_data(0x239E, &pos);
    pos.x += panel.x;
    pos.y += panel.y;
    if (work->tab_0x030 == 2 && active == 1) {
        flags = 1;
    } else {
        flags = lb_barter_settle_ok_ck(work) == 0 ? 2 : 0;
    }
    lb_cancel_prompt_draw(flags, &pos);
}

/* 0x80301DF0 (0x164): draws one cart cell - its frame, the item icon (or the points icon) and the count. */
void lb_barter_cart_cell_draw(LbBarterCartRow* row, u8 index, u8 side, u16 part, s32 cursor, s32 active,
                              _mh_ivec2_* base) {
    _SPR_DATA_ spr;
    char buf[16];
    _mh_ivec2_ pos;
    u16 on;
    u16 flip;

    sprite_frame_apply(&spr, lb_barter_cart_cell_ids[index], part, NULL);
    uv_pair_copy(&pos, &spr);
    pos.x += base->x;
    pos.y += base->y;
    flip = side != 0;
    if (cursor != 0 && active != 0) {
        on = 1;
    } else {
        on = 0;
    }
    draw_sprite_anim_idx(0x234D, on, &pos);
    draw_sprite_anim_idx(0x234E, flip, &pos);
    if (cursor != 0 && active != 0) {
        put_menu_cursor(lb_barter_cart_cursor_ids, 0, &pos);
    }
    if (row->flag_0x06 == 0) {
        draw_itemicon_anim_idx(0x234C, on, row->item_0x00, &pos);
    } else {
        draw_tex_icon_idx(0x234C, 0x22, -1, &pos);
    }
    sprintf(buf, lb_barter_count_fmt, row->count_0x02);
    draw_font_anim_idx(0x234B, on, (s8*)buf, 1, &pos);
}

/* 0x80301F54 (0x19C): draws the points gauge of a cart (the trader pays at most 1000) with the selected row's share
 * in red. */
void lb_barter_gauge_draw(LbBarterWork* work, LbBarterCartRow* cart, LbBarterCartRow* sel, u16 part, _mh_ivec2_* pos) {
    _SPR_DATA_ spr;
    u16 total;
    s16 width;
    s16 w;
    u16 price;

    total = lb_barter_cart_total(work, cart);
    draw_sprite_anim_ary(lb_barter_gauge_frame_ids, part, pos);
    spr_data_copy(&spr, get_lsp_data(0x2361, NULL));
    width = spr.width;
    w = 0.001f * (f32)(total * width);
    if (spr.width >= w) {
        spr.width = w;
    }
    spr_anim_draw(&spr, part, pos);
    if (sel != NULL) {
        price = sel->price_0x04;
        if (total > 1000) {
            w = 0.001f * (f32)((u16)(1000 - (u16)(total - price)) * width);
        } else {
            w = 0.001f * (f32)(price * width);
        }
        if (width >= w) {
            spr.pos.x += (s16)(spr.width - w);
            spr.width = w;
            spr.color = 0xC03030FF;
        }
        draw_sprite(spr, pos);
    }
}

/* 0x803020F0 (0x138): draws the five points pips of a cart: `count` lit, the cart's total, the selected row's share
 * in red. */
void lb_barter_pips_draw(LbBarterWork* work, LbBarterCartRow* cart, LbBarterCartRow* sel, u16 part, u16 count,
                         _mh_ivec2_* pos) {
    _SPR_DATA_ spr;
    u16 share = 0;
    u16 total;
    u16 i;

    total = lb_barter_cart_total(work, cart);
    if (sel != NULL) {
        share = sel->price_0x04;
    }
    draw_sprite_anim_ary(lb_barter_gauge_frame_ids, part, pos);
    for (i = 0; i < count; i++) {
        draw_sprite_anim_idx(lb_barter_pip_on_ids[i], part, pos);
    }
    for (i = 0; i < total - share; i++) {
        draw_sprite_anim_idx(lb_barter_pip_ids[i], part, pos);
    }
    for (; i < total; i++) {
        sprite_frame_apply(&spr, lb_barter_pip_ids[i], part, NULL);
        spr.color = 0xC03030FF;
        draw_sprite(spr, pos);
    }
}

/* 0x80302228 (0x4C): draws the settle-ready mark while the trade can be settled. */
void lb_barter_ready_draw(LbBarterWork* work) {
    _mh_ivec2_ pos;

    get_lsp_data(0x23B6, &pos);
    if (work->ready_0x058 == 1) {
        draw_sprite_ary(lb_barter_ready_ids, &pos);
    }
}

/* 0x80302274 (0x98): draws one cart's frame, and its cover while the trade is idle. */
void lb_barter_cart_frame_draw(LbBarterWork* work, u8 side) {
    _mh_ivec2_ pos;
    u16 part;
    u16 id;

    if (side == 0) {
        part = 0;
        id = 0x23B6;
    } else {
        part = 1;
        id = 0x23D7;
    }
    get_lsp_data(id, &pos);
    draw_sprite_anim_ary(lb_barter_cart_frame_ids, part, &pos);
    switch (work->step_0x001) {
    case 3:
    case 5:
    case 0:
        draw_sprite_anim_ary(lb_barter_cart_cover_ids, part, &pos);
        break;
    }
}

/* 0x8030230C (0x448): draws one cart (`side` 0 the goods paid, 1 the goods taken): its cells, its gauge and the cart
 * cursor's prompt. */
void lb_barter_cart_draw(LbBarterWork* work, u8 side, s32 active) {
    _mh_ivec2_ base;
    _mh_ivec2_ gauge;
    _mh_ivec2_ origin;
    LbBarterCartRow* sel = NULL;
    LbBarterCartRow* cart;
    s32 index;
    u32 part;
    u32 id;
    u8 i;

    index = work->cart_row_0x038 + work->cart_col_0x03A * 5;
    get_lsp_data(side == 0 ? 0x23B6 : 0x23D7, &base);
    if (work->type_0x000 == 2) {
        if (side == 0) {
            part = 0;
        } else {
            part = 2;
        }
    } else {
        part = side == 0 ? 1 : 3;
    }
    get_lsp_data(0x23CB, &origin);
    origin.x += base.x;
    origin.y += base.y;
    draw_sprite_anim_ary(lb_barter_cart_bg_ids, part, &origin);
    cart = side == 0 ? work->pay_0x7D4 : work->get_0x784;
    if (work->type_0x000 == 2) {
        for (i = 0; i < 10; i++) {
            s32 cursor;
            if (index == i && active != 0) {
                cursor = 1;
                sel = &cart[i];
            } else {
                cursor = 0;
            }
            lb_barter_cart_cell_draw(&cart[i], i, side, 0, cursor, active, &origin);
        }
    } else if (side == 0) {
        s32 cursor;
        _SPR_DATA_ win;
        if (index == 0 && active != 0) {
            cursor = 1;
            sel = cart;
        } else {
            cursor = 0;
        }
        sprite_frame_apply(&win, 0x238A, 1, NULL);
        win.pos.x += origin.x;
        win.pos.y += origin.y;
        lb_window_draw(cart->item_0x00, 0, &win.pos, 1, cursor, 1, 0x80);
    } else {
        for (i = 0; i < 5; i++) {
            s32 cursor;
            if (index == i && active != 0) {
                cursor = 1;
                sel = &cart[i];
            } else {
                cursor = 0;
            }
            lb_barter_cart_cell_draw(&cart[i], i, side, 1, cursor, active, &origin);
        }
    }
    if (work->type_0x000 == 2) {
        if (side == 0) {
            part = 0;
            id = 0x2388;
        } else {
            part = 2;
            id = 0x2389;
        }
    } else if (side == 0) {
        part = 1;
        id = 0x2388;
    } else {
        part = 3;
        id = 0x2389;
    }
    get_lsp_data(id, &gauge);
    gauge.x += origin.x;
    gauge.y += origin.y;
    if (work->type_0x000 == 2) {
        lb_barter_gauge_draw(work, cart, sel, part, &gauge);
    } else {
        lb_barter_pips_draw(work, cart, sel, part, lb_barter_cart_total(work, work->pay_0x7D4), &gauge);
    }
    if (work->shown_0x04C != work->cursor_0x032 && chk_pointer() == 0) {
        get_lsp_data(0x23CC, &gauge);
        gauge.x += origin.x;
        gauge.y += origin.y;
        switch (work->step_0x001) {
        case 1:
            draw_sprite_anim_ary(lb_barter_cart_blink_ids, (u32)work->frames_0x054 % 40, &base);
            if (work->tab_0x030 == 1) {
                if (lb_barter_entry_flags(work, work->get_0x784, work->cursor_0x032) == 0) {
                    lb_button_prompt_draw(work->blink_0x070, &gauge);
                }
            } else if (lb_barter_entry_flags(work, work->pay_0x7D4, work->cursor_0x032) == 0) {
                lb_button_prompt_draw(work->blink_0x070, &gauge);
            }
            break;
        case 2:
            draw_sprite_anim_ary(lb_barter_cart_blink2_ids, (u32)work->frames_0x054 % 40, &base);
            if (cart[index].count_0x02 != 0) {
                lb_button_prompt_draw(work->blink_0x070, &gauge);
            }
            break;
        }
    }
}

/* 0x80302754 (0x3C): draws the paid cart's arrow. */
void lb_barter_pay_arrow_draw(LbBarterWork* work) {
    _mh_ivec2_ pos;

    get_lsp_data(0x23D8, &pos);
    draw_sprite_anim_ary(lb_barter_arrow_ids, 0, &pos);
}

/* 0x80302790 (0xB4): draws the paid cart's gauge beside the offer list. */
void lb_barter_pay_gauge_draw(LbBarterWork* work) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;

    get_lsp_data(0x23D8, &base);
    get_lsp_data(0x23D6, &pos);
    pos.x += base.x;
    pos.y += base.y;
    if (work->type_0x000 == 2) {
        lb_barter_gauge_draw(work, work->pay_0x7D4, NULL, 0, &pos);
    } else {
        lb_barter_pips_draw(work, work->pay_0x7D4, NULL, 1, lb_barter_cart_total(work, work->pay_0x7D4), &pos);
    }
}

/* 0x80302844 (0x3C): draws the taken cart's arrow. */
void lb_barter_get_arrow_draw(void) {
    _mh_ivec2_ pos;

    get_lsp_data(0x23CD, &pos);
    draw_sprite_anim_ary(lb_barter_arrow_ids, 1, &pos);
}

/* 0x80302880 (0xB4): draws the taken cart's gauge beside the offer list. */
void lb_barter_get_gauge_draw(LbBarterWork* work) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;

    get_lsp_data(0x23CD, &base);
    get_lsp_data(0x23D6, &pos);
    pos.x += base.x;
    pos.y += base.y;
    if (work->type_0x000 == 2) {
        lb_barter_gauge_draw(work, work->get_0x784, NULL, 2, &pos);
    } else {
        lb_barter_pips_draw(work, work->get_0x784, NULL, 3, lb_barter_cart_total(work, work->pay_0x7D4), &pos);
    }
}

/* 0x80302934 (0x2D8): draws one offer row - the item cell, then the price gauge and its rate mark (`type` 2) or the
 * price pips. */
void lb_barter_row_draw(LbBarterWork* work, LbBarterEntry* row, u8 type, s16 tab, s32 enabled, s32 sel, s32 active,
                        _mh_ivec2_* base) {
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    _mh_ivec2_ pip;
    u16 price = row->price_0x02;
    u16 mark;
    s16 w;

    draw_sprite_ary(lb_barter_row_frame_ids, base);
    get_lsp_data(0x23FE, &pos);
    pos.x += base->x;
    pos.y += base->y;
    if (row->flag_0x09 == 0) {
        ((s32 (*)(u16, s32, _mh_ivec2_*, s32, s32, s32, s32))lb_item_cell_draw_layout1)(row->item_0x00, row->count_0x04,
                                                                                    &pos, enabled, sel, active, 0x40);
    } else {
        ((void (*)(u16, _mh_ivec2_*, s32, s32, s32, u16))lb_item_cell_draw_layout8)(row->item_0x00, &pos, enabled, sel,
                                                                                   active, 0xC1);
    }
    if (type == 2) {
        if (tab == 0) {
            if (lb_barter_wanted_ck(work, row->item_0x00) != 0) {
                mark = 2;
            } else {
                mark = 0;
            }
        } else if (row->rate_0x08 < 100) {
            mark = 2;
        } else {
            mark = row->rate_0x08 > 100 ? 3 : 1;
        }
        draw_sprite_anim_ary(lb_barter_row_gauge_ids, 0, &pos);
        spr_data_copy(&spr, get_lsp_data(0x2404, NULL));
        w = 0.001f * (f32)(price * spr.width);
        if (spr.width >= w) {
            spr.width = w;
        }
        spr_anim_draw(&spr, mark, &pos);
        if (sel == 0) {
            spr_data_copy(&spr, get_lsp_data(0x2403, NULL));
            spr.width = w;
            spr_anim_draw(&spr, mark, &pos);
        }
        if (mark == 2) {
            draw_sprite_idx(0x2401, &pos);
            draw_sprite_idx(0x23FF, &pos);
        } else if (mark == 3) {
            draw_sprite_idx(0x2402, &pos);
            draw_sprite_idx(0x2400, &pos);
        }
    } else {
        s16 i;
        u16* ids;
        if (tab == 0) {
            spr_data_copy(&spr, get_lsp_data(0x2406, NULL));
        } else {
            spr_data_copy(&spr, get_lsp_data(0x240A, NULL));
        }
        draw_sprite_anim_ary(lb_barter_row_pip_frame_ids, 1, &pos);
        for (i = 0, ids = lb_barter_row_pip_ids; i < row->price_0x02; ids++, i++) {
            get_lsp_data(*ids, &pip);
            uv_pair_copy(&spr, &pip);
            draw_sprite(spr, &pos);
        }
    }
}

/* 0x80302C0C (0x398): draws the offer list - its frame, the page arrows, the rows and the button guide. */
void lb_barter_list_draw(LbBarterWork* work, s32 active) {
    _mh_ivec2_ base;
    u16* ids;
    _mh_ivec2_ pos;
    LbBarterEntry* row;
    s32 i;
    u32 type;
    u8 tab;
    s32 enabled;
    s32 sel;
    u16 guide;

    get_lsp_data(0x2418, &base);
    if (work->type_0x000 == 4) {
        draw_sprite_anim_ary(lb_barter_list_frame_ids, 1, &base);
    } else {
        draw_sprite_anim_ary(lb_barter_list_frame_ids, work->tab_0x030, &base);
    }
    lb_page_arrow_draw(0x23FC, work->page_0x03C, work->pages_0x03E, work->moved_0x044, active);
    if (work->type_0x000 != 4) {
        get_lsp_data(0x242F, &pos);
        pos.x += base.x;
        pos.y += base.y;
        lb_cursor_prompt_draw(work->shown_0x04C == work->cursor_0x032, (s16*)&pos);
        type = work->type_0x000;
        tab = work->tab_0x030;
    } else {
        type = work->page_0x03C < work->pages_a_0x040 ? 2 : 3;
        enabled = 1;
        tab = 1;
    }
    if (work->shown_0x04C == 0) {
        lb_text_center_print(0x2426, (s8*)LbStr(3, 0x14C), 2, 0x12, &base, 0);
    } else {
        for (i = 0, ids = lb_barter_list_row_ids, row = work->page_0x448; i < work->shown_0x04C; ids++, row++, i++) {
            get_lsp_data(*ids, &pos);
            pos.x += base.x;
            pos.y += base.y;
            if (work->type_0x000 == 4) {
                if (i == work->cursor_b_0x042) {
                    sel = 1;
                } else {
                    sel = 0;
                }
            } else {
                sel = i == work->cursor_0x032;
                if (work->tab_0x030 == 1) {
                    if (lb_barter_entry_flags(work, work->get_0x784, i) == 0) {
                        enabled = 1;
                    } else {
                        enabled = 0;
                    }
                } else {
                    enabled = lb_barter_entry_flags(work, work->pay_0x7D4, i) == 0;
                }
            }
            lb_barter_row_draw(work, row, type, tab, enabled, sel, active, &pos);
        }
    }
    if (work->type_0x000 != 4 && chk_pointer() == 0) {
        guide = 0xFFFF;
        if (work->tab_0x030 == 1) {
            if (work->step_0x001 == 1) {
                if (lb_barter_cart_total(work, work->get_0x784) != 0 && work->shown_0x04C != work->cursor_0x032) {
                    guide = 0x23F1;
                    if (get_option_cfg(7) == 0) {
                        tab = 4;
                    } else {
                        tab = 0xD;
                    }
                }
            } else {
                guide = 0x23F1;
                if (get_option_cfg(7) == 0) {
                    tab = 5;
                } else {
                    tab = 0xE;
                }
            }
        } else if (work->step_0x001 == 1) {
            if (lb_barter_cart_total(work, work->pay_0x7D4) != 0 && work->shown_0x04C != work->cursor_0x032) {
                guide = 0x23F0;
                if (get_option_cfg(7) == 0) {
                    tab = 2;
                } else {
                    tab = 0xB;
                }
            }
        } else {
            guide = 0x23F0;
            tab = get_option_cfg(7) == 0 ? 3 : 0xC;
        }
        if (guide != 0xFFFF) {
            put_button_guide_c_idx(guide, tab);
        }
    }
}

/* 0x80302FA4 (0xEC): draws the info window of a trade-goods item: its `_tr_flag` name and description. */
void lb_barter_goods_info_draw(u16 item, _SPR_DATA_* frame, _mh_ivec2_* pos) {
    LbTrFlag* goods = lb_tr_flag_get(item);
    _SPR_DATA_* text;

    put_frame_dialog(pos->x, pos->y, frame->width, frame->height, 0, 0x141228B2);
    draw_sprite_ary(lb_barter_goods_frame_ids, pos);
    draw_tex_icon_idx(0x9E, 0x22, -1, pos);
    draw_font_idx(0xA2, goods->name_0x04, 1, pos);
    text = get_lsp_data(0xA3, NULL);
    font_set_size(text->width, text->height);
    font_print_ex(pos->x + text->pos.x, pos->y + text->pos.y, 0, goods->text_0x08);
}

/* 0x80303090 (0x140): draws the info window of the row or cart cell under the cursor. */
void lb_barter_item_info_draw(LbBarterWork* work) {
    _SPR_DATA_ spr;
    u16 item[2];
    _mh_ivec2_ pos;
    s32 index = work->cart_row_0x038 + work->cart_col_0x03A * 5;
    s32 goods = 0;

    sprite_frame_apply(&spr, 0x2331, work->tab_0x030 != 0, &pos);
    item[1] = 0;
    if (work->step_0x001 == 1) {
        if (work->cursor_0x032 == work->rows_0x034 - 1) {
            return;
        }
        if (work->page_0x448[work->cursor_0x032].flag_0x09 == 1) {
            goods = 1;
        }
        item[0] = work->page_0x448[work->cursor_0x032].item_0x00;
    } else if (work->tab_0x030 == 0) {
        if (work->pay_0x7D4[index].flag_0x06 == 1) {
            goods = 1;
        }
        item[0] = work->pay_0x7D4[index].item_0x00;
    } else {
        if (work->get_0x784[index].flag_0x06 == 1) {
            goods = 1;
        }
        item[0] = work->get_0x784[index].item_0x00;
    }
    if (goods == 0) {
        menu_item_row_draw_pages(item, &pos);
    } else {
        lb_barter_goods_info_draw(item[0], &spr, &pos);
    }
}

/* 0x803031D0 (0x9C): draws the info window of the whale menu's list row under the cursor. */
void lb_kujira_item_info_draw(LbBarterWork* work) {
    _SPR_DATA_ spr;
    u16 item[2];
    _mh_ivec2_ pos;
    s32 goods = 0;

    sprite_frame_apply(&spr, 0x2331, 1, &pos);
    item[1] = 0;
    if (work->page_0x448[work->cursor_b_0x042].flag_0x09 == 1) {
        goods = 1;
    }
    item[0] = work->page_0x448[work->cursor_b_0x042].item_0x00;
    if (goods == 0) {
        menu_item_row_draw(item, &pos);
    } else {
        lb_barter_goods_info_draw(item[0], &spr, &pos);
    }
}

/* 0x8030326C (0x80): returns the position of hold cell `index % 10`, offset by (+4, +2). */
void lb_box_cell_pos_get(s32 index, _mh_ivec2_* out) {
    _mh_ivec2_ pos;

    get_lsp_data(lb_box_cell_ids[index % 10], &pos);
    out->x = pos.x + 4;
    out->y = pos.y + 2;
}

/* 0x803032EC (0x294): draws the ten hold cells of the item box with their icons, the pointed cell's blink and the
 * picked cell's marks. */
void lb_box_cells_draw(Eft052ItemBox* box) {
    _mh_ivec2_ cell;
    _mh_ivec2_ icon;
    _mh_ivec2_ picked;
    _SPR_DATA_ frame;
    _mh_ivec2_ base;
    _SPR_DATA_ item;
    _SPR_DATA_ mark;
    s32 i;
    s32 found;

    get_lsp_data(0x248E, &base);
    draw_sprite_anim_ary(lb_box_frame_ids, 0, &base);
    found = 0;
    spr_data_copy(&item, get_lsp_data(0x1307, NULL));
    uv_pair_copy(&icon, &item);
    for (i = 0; i < 10; i++) {
        lb_box_cell_pos_get(i, &cell);
        spr_data_copy(&frame, get_lsp_data(0x248D, NULL));
        uv_pair_copy(&frame, &cell);
        item.pos.x = cell.x + icon.x;
        item.pos.y = cell.y + icon.y;
        draw_sprite(frame, &base);
        if (box->cursor_0x0E == i) {
            found = 1;
            uv_pair_copy(&picked, &cell);
        }
        if ((u8)(box->state_0x00 + 0xFF) <= 1 && chk_pointer() == 0 && box->hover_0x10 == i) {
            spr_data_copy(&mark, get_lsp_data(0x1304, NULL));
            mark.pos.x += cell.x;
            mark.pos.y += cell.y;
            set_blendmode(4, 1, 1);
            spr_anim_draw(&mark, box->frame_0x08, &base);
            set_blendmode(4, 5, 1);
        }
        if (box->cells_0x24[i].item_0x00 != 0) {
            draw_itemicon_item_id(item, box->cells_0x24[i].item_0x00, &base);
        }
    }
    if (box->picked_0x0C != 0 && found != 0) {
        set_blendmode(4, 1, 1);
        spr_data_copy(&mark, get_lsp_data(0x1305, NULL));
        mark.pos.x += picked.x;
        mark.pos.y += picked.y;
        spr_anim_draw(&mark, box->frame_0x0A, &base);
        set_blendmode(4, 5, 1);
        spr_data_copy(&mark, get_lsp_data(0x1306, NULL));
        mark.pos.x += picked.x;
        mark.pos.y += picked.y;
        spr_anim_draw(&mark, box->frame_0x0A, &base);
    }
}

/* 0x80303580 (0x78): draws the whale menu's item box page: its background, the hold cells, the count and the
 * option list. */
void lb_kujira_box_draw(void) {
    _mh_ivec2_ pos;

    get_lsp_data(0x2469, &pos);
    draw_sprite_ary(lb_kujira_box_bg_ids, &pos);
    lb_box_cells_draw(&eft052_item_box);
    lb_item_box_count_draw(&eft052_item_box, 0x2468, 0);
    eft052_box_list_draw(0x2497, lb_screen11_choice_ids, lb_screen11_choice_rows, 0x2467, 0);
}

/* 0x803035F8 (0x38): returns the `_tr_flag` record of goods `id`, or the table's first record when none matches. */
LbTrFlag* lb_tr_flag_get(u16 id) {
    LbTrFlag* rec = lb_tr_flag_data;

    while (rec->id_0x00 != 0xFFFF) {
        if (rec->id_0x00 == id) {
            return rec;
        }
        rec++;
    }
    return lb_tr_flag_data;
}

/* 0x80303630 (0x1C8): draws the once-only goods the trade handed over - their window, name and description. */
void lb_barter_got_draw(LbBarterWork* work) {
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    LbTrFlag* goods;

    get_lsp_data(0x2430, &base);
    draw_sprite_ary(lb_barter_got_bg_ids, &base);
    get_lsp_data(0x245B, &base);
    draw_sprite_ary(lb_barter_got_title_ids, &base);
    draw_font_idx(0x245C, (s8*)LbStr(0, 0x199), 0, &base);
    goods = lb_tr_flag_get(work->got_0x13C[work->got_index_0x007]);
    get_lsp_data(0x2466, &pos);
    pos.x += base.x;
    pos.y += base.y;
    draw_sprite_ary(lb_barter_got_icon_ids, &pos);
    draw_tex_icon_idx(0x2456, 0x22, -1, &pos);
    draw_font_idx(0x2455, goods->name_0x04, 1, &pos);
    get_lsp_data(0x244A, &base);
    draw_sprite_ary(lb_barter_got_text_ids, &base);
    draw_font_idx(0x244C, (s8*)LbStr(0, 0x19A), 1, &base);
    draw_font_idx(0x244D, (s8*)LbStr(0, 0x19B), 0, &base);
    draw_font_idx(0x244E, goods->text_0x08, 0, &base);
    draw_font_idx(0x244F, (s8*)LbStr(0, 0x19C), 0, &base);
    get_lsp_data(0x2453, &pos);
    pos.x += base.x;
    pos.y += base.y;
    lb_button_prompt_draw(work->blink_0x070, &pos);
}

/* 0x803037F8 (0x2A0): draws the trade by step: the tabs and both carts, the offer list with its arrow, gauge and
 * info window, the two dialogs, the hold box and the hand-over. */
void lb_barter_screen_draw(LbBarterWork* work) {
    switch (work->step_0x001) {
    case 0:
        lb_barter_tabs_draw(work, 1);
        lb_barter_cart_frame_draw(work, 0);
        lb_barter_cart_frame_draw(work, 1);
        lb_barter_cart_draw(work, 0, 0);
        lb_barter_cart_draw(work, 1, 0);
        lb_barter_ready_draw(work);
        break;
    case 1:
        lb_barter_list_draw(work, 1);
        if (work->tab_0x030 == 0) {
            lb_barter_get_arrow_draw();
            lb_barter_cart_frame_draw(work, 0);
            lb_barter_get_gauge_draw(work);
            lb_barter_cart_draw(work, 0, 0);
            lb_barter_ready_draw(work);
        } else {
            lb_barter_pay_arrow_draw(work);
            lb_barter_cart_frame_draw(work, 1);
            lb_barter_pay_gauge_draw(work);
            lb_barter_cart_draw(work, 1, 0);
            lb_barter_ready_draw(work);
        }
        lb_barter_item_info_draw(work);
        break;
    case 2:
        lb_barter_list_draw(work, 0);
        if (work->tab_0x030 == 0) {
            lb_barter_get_arrow_draw();
            lb_barter_cart_frame_draw(work, 0);
            lb_barter_get_gauge_draw(work);
            lb_barter_cart_draw(work, 0, 1);
            lb_barter_ready_draw(work);
        } else {
            lb_barter_pay_arrow_draw(work);
            lb_barter_cart_frame_draw(work, 1);
            lb_barter_pay_gauge_draw(work);
            lb_barter_cart_draw(work, 1, 1);
            lb_barter_ready_draw(work);
        }
        lb_barter_item_info_draw(work);
        break;
    case 3:
        lb_barter_tabs_draw(work, 0);
        lb_barter_cart_frame_draw(work, 0);
        lb_barter_cart_frame_draw(work, 1);
        lb_barter_cart_draw(work, 0, 0);
        lb_barter_cart_draw(work, 1, 0);
        lb_barter_ready_draw(work);
        lb_panel_msg_draw(0x1877, 0x14A);
        lb_panel_yes_no_draw(0x1879, work->dialog_0x050);
        break;
    case 5:
        lb_barter_tabs_draw(work, 0);
        lb_barter_cart_frame_draw(work, 0);
        lb_barter_cart_frame_draw(work, 1);
        lb_barter_cart_draw(work, 0, 0);
        lb_barter_cart_draw(work, 1, 0);
        lb_barter_ready_draw(work);
        lb_panel_msg_draw(0x1877, 0x14B);
        lb_panel_yes_no_draw(0x1879, work->dialog_0x050);
        break;
    case 4:
        menu_money_draw(0x14FC);
        lb_kujira_box_draw();
        break;
    case 6:
        lb_barter_got_draw(work);
        break;
    }
}

/* 0x80303A98 (0x274): draws one trade-kind row of the whale menu: its name, its on/off state and, under the cursor,
 * the kind's cart preview and the button guide. */
void lb_kujira_kind_row_draw(LbBarterWork* work, u8 bits, u8 kind, bool sel, bool enabled, _mh_ivec2_* base) {
    _SPR_DATA_ spr;
    _mh_ivec2_ cart;
    _mh_ivec2_ pos;
    _mh_ivec2_ guide;
    u32 flags;

    spr_data_copy(&spr, get_lsp_data(0x233F, NULL));
    get_lsp_data(lb_kujira_kind_row_pos_ids[kind], &pos);
    pos.x += base->x;
    pos.y += base->y;
    draw_sprite_anim_ary(lb_kujira_kind_row_ids, kind, &pos);
    spr.color = GetMenuFontColor(1, sel, enabled, 0);
    flags = sel != 0 ? 5 : 1;
    if (bits & (1 << kind)) {
        draw_font(spr, (s8*)LbStr(0, 0x193), flags, &pos);
    } else {
        draw_font(spr, (s8*)LbStr(0, 0x194), flags, &pos);
    }
    if (sel != 0) {
        if (enabled != 0) {
            put_menu_cursor(lb_kujira_kind_cursor_ids, 0, &pos);
            sprite_frame_apply(&spr, 1, 0x2344, NULL);
            draw_sprite_blink_idx(0x2344, 0x2345, spr.color, work->moved_0x044, &pos);
        }
        get_lsp_data(0x23B6, &cart);
        draw_sprite_anim_ary(lb_barter_cart_frame_ids, 2, &cart);
        get_lsp_data(0x23CB, &pos);
        pos.x += cart.x;
        pos.y += cart.y;
        draw_sprite_ary(lb_kujira_kind_name_ids, &pos);
        draw_font_idx(0x238D, (s8*)LbStr(0, kind + 0x190), 0, &pos);
        draw_sprite_ary(lb_barter_cart_cover_ids, &cart);
        if (chk_pointer() == 0) {
            get_lsp_data(0x2392, &guide);
            guide.x += pos.x;
            guide.y += pos.y;
            if (get_option_cfg(7) == 0) {
                put_button_guide_c(6, &guide);
            } else {
                put_button_guide_c(0xF, &guide);
            }
        }
    }
}

/* 0x80303D0C (0x37C): draws the whale menu - the trade kinds, the gift item and the confirm prompt. */
void lb_kujira_menu_draw(LbBarterWork* work) {
    _mh_ivec2_ base;
    _mh_ivec2_ panel;
    _mh_ivec2_ pos;
    _SPR_DATA_ spr;
    u8 count;
    u8 i;
    bool sel;
    u32 flags;

    get_lsp_data(0x23D9, &base);
    draw_sprite_anim_ary(lb_barter_tab_frame_ids, 1, &base);
    get_lsp_data(0x23E5, &panel);
    panel.x += base.x;
    panel.y += base.y;
    draw_sprite_ary(lb_kujira_menu_panel_ids, &panel);
    draw_font_idx(0x23A3, (s8*)LbStr(0, 0x18D), 5, &panel);
    draw_font_idx(0x23A7, (s8*)LbStr(0, 0x18E), 0, &panel);
    draw_font_idx(0x23A8, (s8*)LbStr(0, 0x18F), 0, &panel);
    count = 0;
    for (i = 0; i < work->menu_count_0x036; i++) {
        draw_font_idx(i + 0x23A4, (s8*)LbStr(0, i + 0x18A), 0, &panel);
        if (work->type_0x000 == 4) {
            if (i == work->menu_0x009[work->cursor_0x032]) {
                sel = 1;
            } else {
                sel = 0;
            }
        } else {
            sel = 0;
        }
        lb_kujira_kind_row_draw(work, work->ship_bits_0x003, i, sel, 1, &panel);
        if (work->ship_bits_0x003 & (1 << i)) {
            count++;
        }
    }
    draw_sprite_anim_idx(lb_kujira_kind_count_ids[count], work->item_0x004 != 0, &panel);
    get_lsp_data(0x23B3, &pos);
    pos.x += panel.x;
    pos.y += panel.y;
    draw_sprite_anim_ary(lb_barter_tab_row_ids, 2, &pos);
    spr_data_copy(&spr, get_lsp_data(0x2335, NULL));
    spr.color = GetMenuFontColor(1, sel, 1, 0);
    if (work->type_0x000 == 4 && work->menu_0x009[work->cursor_0x032] == 3) {
        sel = 1;
    } else {
        sel = 0;
    }
    flags = sel == 1 ? 5 : 1;
    if (work->type_0x000 == 4 && sel == 1) {
        put_menu_cursor(lb_barter_tab_cursor_ids, 0, &pos);
    }
    draw_font(spr, (s8*)ItemName(work->item_0x004), flags, &pos);
    if (work->type_0x000 == 4) {
        s32 on = work->menu_0x009[work->cursor_0x032] == 4;
        u8 bits;
        u8 j;
        get_lsp_data(0x23B4, &pos);
        pos.x += panel.x;
        pos.y += panel.y;
        bits = 0;
        for (j = 0; j < work->menu_count_0x036; j++) {
            if (work->ship_bits_0x003 & (1 << j)) {
                bits++;
            }
        }
        if (bits != 0) {
            lb_cursor_prompt_draw(on, (s16*)&pos);
        } else {
            lb_cursor_prompt_draw(2, (s16*)&pos);
        }
    }
}

/* 0x80304088 (0x10C): draws the whale's stay - how many days the trader still stays in the village. */
void lb_kujira_days_draw(void) {
    s8 days = -lobby_world_block->work_0x4832.count_0x02;
    _mh_ivec2_ pos;
    char buf[0x80];

    get_lsp_data(0x22ED, &pos);
    draw_sprite_ary(lb_kujira_days_ids, &pos);
    draw_font_idx(0x22EE, (s8*)LbStr(0, 0x195), 1, &pos);
    if (days <= 1) {
        sprintf(buf, (char*)LbStr(0, 0x198));
    } else if (days & 1) {
        sprintf(buf, (char*)LbStr(0, 0x197), days / 2);
    } else {
        sprintf(buf, (char*)LbStr(0, 0x196), days / 2);
    }
    draw_font_idx(0x22EF, (s8*)buf, 1, &pos);
}

/* 0x80304194 (0x264): prints the help panel's message for the current screen and step. */
void lb_barter_help_draw(LbBarterWork* work) {
    u16 msg = 0xFFFF;
    u16 line = 0xFFFF;
    u32 flags;

    switch (work->type_0x000) {
    case 2:
    case 3:
        switch (work->step_0x001) {
        case 0:
            msg = work->tab_0x030 + 0x13E;
            break;
        case 2:
            if (work->tab_0x030 == 0) {
                msg = 0x147;
            } else {
                msg = 0x148;
            }
            break;
        case 1:
            if (work->shown_0x04C == work->cursor_0x032) {
                msg = 0x149;
            } else if (work->tab_0x030 == 0) {
                msg = 0x141;
                flags = lb_barter_entry_flags(work, work->pay_0x7D4, work->cursor_0x032);
                if (flags & 2) {
                    if (work->type_0x000 == 2) {
                        line = 0x142;
                    } else {
                        line = 0x14F;
                    }
                } else if (flags & 0x20) {
                    line = 0x143;
                }
            } else {
                msg = 0x144;
                flags = lb_barter_entry_flags(work, work->get_0x784, work->cursor_0x032);
                if (flags & 2) {
                    if (work->type_0x000 == 2) {
                        line = 0x145;
                    } else {
                        line = 0x14D;
                    }
                } else if (flags & 0x20) {
                    if (work->type_0x000 == 2) {
                        line = 0x146;
                    } else {
                        line = 0x14D;
                    }
                }
            }
            break;
        }
        break;
    case 4:
        switch (work->step_0x001) {
        case 0:
            switch (work->menu_0x009[work->cursor_0x032]) {
            case 0:
            case 1:
            case 2:
                msg = 0x150;
                if ((work->ship_bits_0x003 & 7) == 0) {
                    line = 0x155;
                }
                break;
            case 3:
                msg = 0x151;
                break;
            case 4:
                msg = 0x152;
                break;
            }
            break;
        case 4:
            msg = 0x158;
            break;
        case 2:
            msg = 0x153;
            lb_panel_yes_no_draw(0x1879, work->dialog_0x050);
            break;
        case 3:
            msg = 0x154;
            lb_panel_yes_no_draw(0x1879, work->dialog_0x050);
            break;
        }
        break;
    case 5:
        msg = 0x159;
        line = 0x15A;
        break;
    }
    if (msg != 0xFFFF) {
        lb_panel_msg_draw(0x1877, msg);
        if (line != 0xFFFF) {
            lb_panel_str_print(0x1877, line, 2);
        }
    }
}

/* 0x803043F8 (0x110): draws lobby screen 11 (the sub-transfer callback `lb_screen11_step` queues). */
void lb_screen11_draw(void) {
    LbBarterWork* work = lobby_w.menu_0x0AC;

    set_zmode(0, 0, 0);
    set_blendmode(4, 5, 1);
    lb_barter_bg_draw(work);
    switch (work->type_0x000) {
    case 0:
    case 1:
        lb_choice_draw((LbList*)&work->mode_0x010, 0x2321, lb_screen11_choice_ids, lb_screen11_choice_rows, 0x1877, 0);
        lb_kujira_days_draw();
        break;
    case 2:
    case 3:
        lb_barter_screen_draw(work);
        break;
    case 4:
        switch (work->step_0x001) {
        case 1:
            eft052_hold_draw(0);
            break;
        case 4:
            lb_barter_list_draw(work, 1);
            lb_kujira_item_info_draw(work);
            break;
        default:
            lb_kujira_menu_draw(work);
            break;
        }
        break;
    case 5:
        lb_kujira_menu_draw(work);
        break;
    }
    lb_barter_help_draw(work);
}

/* ------------------------------------------------------------------------------------------------
 * The enemy and area effect spawners
 * ------------------------------------------------------------------------------------------------ */

/* 0x80304508 (0x8): spawns enemy effect `id` at joint `joint` of `self` (`pos` an optional offset, `scale` its
 * size). */
void eft_em_spawn(struct _ENEMY_WORK* self, u32 id, u32 joint, nw4r::math::VEC3* pos, f32 scale) {
    eft_em_create(self, id, joint, pos, scale);
}

/* 0x80304510 (0x40): the same spawn with a parameter word (an angle or a colour) for the effect's step. */
void eft_em_spawn_param(struct _ENEMY_WORK* self, u32 id, u32 joint, nw4r::math::VEC3* pos, f32 scale, u32 param) {
    _EFT* rec = eft_em_create(self, id, joint, pos, scale);
    if (rec != NULL) {
        ((EftEmWork*)rec->work_0x38)->param_0x1C = param;
    }
}

/* 0x80304550 (0x2B4): pools an enemy effect record for `self`: picks the variant of `id` the enemy's state and the
 * map ask for, stores the joint, offset and scale and installs the step and release callbacks. */
_EFT* eft_em_create(struct _ENEMY_WORK* self, u8 id, u32 joint, nw4r::math::VEC3* pos, f32 scale) {
    _EFT* rec;
    EftEmWork* work;

    if (self->area_no != get_now_areano()) {
        return NULL;
    }
    rec = (_EFT*)eft_res_slot_get(0x20);
    if (rec == NULL) {
        return NULL;
    }
    switch (id) {
    case 12:
    case 14:
    case 18:
        if (self->field_0x228 & 6) {
            id += 1;
        }
        break;
    case 13:
        break;
    case 34:
    case 35:
        if (self->field_0x228 & 6) {
            id += 54;
        }
        break;
    case 61:
        if (get_camera_pos().y > 0.0f && (self->field_0x228 & 2)) {
            id = 62;
        }
        break;
    case 68:
    case 69:
        if (self->field_0x228 & 6) {
            id += 86;
        }
        break;
    case 85:
        if (self->field_0x228 & 6) {
            id = 156;
        }
        break;
    case 71:
        switch (get_now_mapno()) {
        case 4:
        case 15:
            rec->demo_flag_0x08 = 2;
            break;
        default:
            if (self->field_0x228 & 6) {
                rec->demo_flag_0x08 = 3;
            } else {
                rec->demo_flag_0x08 = 0;
            }
            break;
        }
        break;
    }
    work = (EftEmWork*)rec->work_0x38;
    work->count_0x00 = 1;
    work->scale_0x08 = scale;
    work->joint_0x0C = joint;
    if (pos != NULL) {
        copyVec3(&work->offset_0x10, pos);
    } else {
        setVector3(&work->offset_0x10, 0.0f, 0.0f, 0.0f);
    }
    rec->field_0x03 = 0x27;
    rec->type_0x02 = id;
    rec->source_0x30 = self;
    rec->area_0x44 = self->area_no;
    rec->rot_0x24.x = self->field_0x1BC;
    rec->rot_0x24.y = self->field_0x1C0;
    rec->rot_0x24.z = self->field_0x1C4;
    switch (id) {
    case 44:
        rec->timer_0x0C = 12;
        break;
    default:
        rec->timer_0x0C = 0;
        break;
    }
    eft_state_flags_set(rec, 0, 0);
    rec->release_0x40 = eft_em_release;
    rec->dispatch_0x34 = eft_em_dispatch;
    if (event_demo_ck() == 1) {
        rec->field_0x07 = 1;
    }
    return rec;
}

/* 0x80304804 (0x3C): releases the record's effect handles. */
void eft_em_release(_EFT* self) {
    EftEmWork* work = (EftEmWork*)self->work_0x38;
    push_eft_effect_heap_num(&work->effect_0x04, work->count_0x00);
    work->count_0x00 = 0;
}

/* 0x80304840 (0x3C): steps the record by state: create, move, end, kill. */
void eft_em_dispatch(_EFT* self) {
    switch (self->state_0x05) {
    case 0:
        eft_em_model_create(self);
        break;
    case 1:
        eft_em_move(self);
        break;
    case 2:
        eft_em_end(self);
        break;
    case 3:
        eft_em_kill(self);
        break;
    }
}

/* 0x8030487C (0x96C): creates the record's effect from the id tables, places it on its joint by the id's placement
 * rule, plays its start sound and runs the first move. */
void eft_em_model_create(_EFT* self) {
    nw4r::math::VEC3 ofs;
    nw4r::math::MTX34 mtx;
    EftEmWork* work;
    u16 group;
    struct _ENEMY_WORK* enemy;
    u16 res;
    s32 i;

    VEC3_ctor(&ofs);
    MTX34_ctor(&mtx);
    work = (EftEmWork*)self->work_0x38;
    enemy = (struct _ENEMY_WORK*)self->source_0x30;
    self->state_0x05++;
    res = eft_em_res_ids[self->type_0x02];
    group = eft_em_res_groups[self->type_0x02];
    switch (self->type_0x02) {
    case 2:
        if (enemy->team == 0xF) {
            res = 0x4DD;
            group = 0x26;
        }
        break;
    }
    for (i = 0; i < work->count_0x00; i++) {
        work->effect_0x04 = res_eft_create(res + self->demo_flag_0x08, group, 0);
        if (work->effect_0x04 == NULL) {
            eft_em_kill(self);
            return;
        }
    }
    if (em_work_die_ck(enemy) != 0) {
        self->flag_0x01 = 0;
        self->state_0x05 = 3;
        return;
    }
    switch (self->type_0x02) {
    case 0x09: case 0x0B: case 0x0C: case 0x0D: case 0x12: case 0x13: case 0x20: case 0x21: case 0x2B: case 0x37:
    case 0x3C: case 0x3D: case 0x3E: case 0x4D: case 0x4E: case 0x51: case 0x53: case 0x5B: case 0x60: case 0x68:
    case 0x70: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x7C: case 0x7D: case 0x80: case 0x81:
    case 0x82: case 0x83: case 0x84: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x9D: case 0x9E:
    case 0x9F: case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA6: case 0xAB: case 0xAE: case 0xB0: case 0xB1:
    case 0xBA: case 0xBB:
        work->scale_0x08 *= get_em_scale(enemy);
        break;
    case 0x0A:
        work->param_0x1C = 0x8000;
        work->scale_0x08 *= get_em_scale(enemy);
        break;
    case 0x0E: case 0x0F:
        work->param_0x1C = 0xC000;
        work->scale_0x08 *= get_em_scale(enemy);
        break;
    case 0x19: case 0x1A: case 0x24: case 0x39: case 0x3B: case 0x43: case 0x44: case 0x47: case 0x48: case 0x49:
    case 0x4A: case 0x4B: case 0x50: case 0x54: case 0x56: case 0x5C: case 0x5D: case 0x5E: case 0x65: case 0x66:
    case 0x67: case 0x77: case 0x79: case 0x7A: case 0x7F: case 0x85: case 0x86: case 0x87: case 0x92: case 0x93:
    case 0xA7: case 0xA8: case 0xAC: case 0xAF: case 0xB6: case 0xB7: case 0xB9: case 0xCB:
        work->scale_0x08 *= get_em_scale(enemy);
        get_joint_wpos_em(enemy, work->joint_0x0C, &self->pos_0x18);
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        copyVec3(&ofs, &work->offset_0x10);
        mulVecMat(&ofs, &mtx);
        addVec3To(&self->pos_0x18, &ofs);
        SetRootMtxTrans(work->effect_0x04, &self->pos_0x18);
        break;
    case 0x1D: case 0x29: case 0x2A: case 0x3A: case 0x52: case 0x61: case 0x6F: case 0x89: case 0x90: case 0x94:
    case 0x95: case 0x96: case 0x97: case 0x98: case 0x99: case 0xA9: case 0xAA: case 0xB5: case 0xC4: case 0xC5:
    case 0xC8: case 0xCC:
        get_joint_wmat_em(enemy, work->joint_0x0C, &mtx);
        copyVec3(&ofs, &work->offset_0x10);
        mulVecMat(&ofs, &mtx);
        mtx34_trans_add(&mtx, &ofs);
        mtx34_trans_get(&mtx, &self->pos_0x18);
        work->effect_0x04->SetRootMtx(mtx);
        break;
    case 0x1C: case 0x1F: case 0x23: case 0x25: case 0x26: case 0x27: case 0x28: case 0x36: case 0x42: case 0x45:
        get_joint_wpos_em(enemy, work->joint_0x0C, &self->pos_0x18);
        self->pos_0x18.y = 5.0f + enemy->field_0x20C;
        SetRootMtxTrans(work->effect_0x04, &self->pos_0x18);
        break;
    case 0x30: case 0xCF: case 0xD1:
        copyVec3(&self->pos_0x18, &enemy->pos);
        self->pos_0x18.y = enemy->field_0x210 - 5.0f;
        SetRootMtxTrans(work->effect_0x04, &self->pos_0x18);
        break;
    case 0x38: case 0x58: case 0x59: case 0x9A: case 0x9B:
        get_joint_wpos_em(enemy, work->joint_0x0C, &self->pos_0x18);
        self->pos_0x18.y = 5.0f + enemy->field_0x210;
        SetRootMtxTrans(work->effect_0x04, &self->pos_0x18);
        break;
    case 0x3F:
        work->scale_0x08 *= get_em_scale(enemy);
        break;
    case 0x40: case 0x41: case 0x5F: case 0x88: case 0xB2: case 0xC7:
        work->scale_0x08 *= get_em_scale(enemy);
        get_joint_wpos_em(enemy, work->joint_0x0C, &self->pos_0x18);
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        copyVec3(&ofs, &work->offset_0x10);
        mulVecMat(&ofs, &mtx);
        addVec3To(&self->pos_0x18, &ofs);
        mtx34_set_trans(&mtx, &self->pos_0x18);
        work->effect_0x04->SetRootMtx(mtx);
        break;
    case 0x4F: case 0x62: case 0x63: case 0x64: case 0x78:
        work->scale_0x08 *= get_em_scale(enemy);
        get_joint_wpos_em(enemy, work->joint_0x0C, &self->pos_0x18);
        if (enemy->field_0x23C != 0) {
            self->pos_0x18.y = enemy->ceiling_0x240 - 5.0f;
        }
        SetRootMtxTrans(work->effect_0x04, &self->pos_0x18);
        break;
    case 0x22: case 0x55: case 0x7B:
        work->scale_0x08 *= get_em_scale(enemy);
        get_joint_wpos_em(enemy, work->joint_0x0C, &self->pos_0x18);
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        copyVec3(&ofs, &work->offset_0x10);
        mulVecMat(&ofs, &mtx);
        addVec3To(&self->pos_0x18, &ofs);
        self->pos_0x18.y = 5.0f + enemy->field_0x20C;
        mtx34_set_trans(&mtx, &self->pos_0x18);
        work->effect_0x04->SetRootMtx(mtx);
        break;
    case 0x57:
        work->scale_0x08 *= get_em_scale(enemy);
        get_joint_wpos_em(enemy, work->joint_0x0C, &self->pos_0x18);
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        copyVec3(&ofs, &work->offset_0x10);
        mulVecMat(&ofs, &mtx);
        addVec3To(&self->pos_0x18, &ofs);
        self->pos_0x18.y = 5.0f + enemy->field_0x210;
        SetRootMtxTrans(work->effect_0x04, &self->pos_0x18);
        break;
    case 0x69: case 0x6A:
        work->scale_0x08 *= get_em_scale(enemy);
        get_joint_wpos_em(enemy, work->joint_0x0C, &self->pos_0x18);
        self->rot_0x24.y += 0x4000;
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        copyVec3(&ofs, &work->offset_0x10);
        mulVecMat(&ofs, &mtx);
        addVec3To(&self->pos_0x18, &ofs);
        mtx34_set_trans(&mtx, &self->pos_0x18);
        work->effect_0x04->SetRootMtx(mtx);
        break;
    case 0x6B: case 0x6C:
        work->scale_0x08 *= get_em_scale(enemy);
        get_joint_wpos_em(enemy, work->joint_0x0C, &self->pos_0x18);
        self->rot_0x24.y += 0xC000;
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        copyVec3(&ofs, &work->offset_0x10);
        mulVecMat(&ofs, &mtx);
        addVec3To(&self->pos_0x18, &ofs);
        mtx34_set_trans(&mtx, &self->pos_0x18);
        work->effect_0x04->SetRootMtx(mtx);
        break;
    case 0x1B: case 0x4C: case 0xB4: case 0xCA:
        work->scale_0x08 *= get_em_scale(enemy);
        get_joint_wpos_em(enemy, work->joint_0x0C, &self->pos_0x18);
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        copyVec3(&ofs, &work->offset_0x10);
        mulVecMat(&ofs, &mtx);
        addVec3To(&self->pos_0x18, &ofs);
        mtx34_set_trans(&mtx, &self->pos_0x18);
        rotLocalMatY(work->param_0x1C, &mtx);
        work->effect_0x04->SetRootMtx(mtx);
        break;
    case 0x91:
        get_joint_wmat_em(enemy, work->joint_0x0C, &mtx);
        copyVec3(&ofs, &work->offset_0x10);
        mulVecMat(&ofs, &mtx);
        mtx34_trans_add(&mtx, &ofs);
        mtx34_trans_get(&mtx, &self->pos_0x18);
        rotLocalMatX(work->param_0x1C, &mtx);
        work->effect_0x04->SetRootMtx(mtx);
        break;
    case 0xAD:
        work->scale_0x08 *= get_em_scale(enemy);
        get_joint_wpos_em(enemy, work->joint_0x0C, &self->pos_0x18);
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        copyVec3(&ofs, &work->offset_0x10);
        mulVecMat(&ofs, &mtx);
        addVec3To(&self->pos_0x18, &ofs);
        self->pos_0x18.y = enemy->field_0x20C;
        mtx34_set_trans(&mtx, &self->pos_0x18);
        rotLocalMatY(work->param_0x1C, &mtx);
        work->effect_0x04->SetRootMtx(mtx);
        break;
    case 0xBD: case 0xBE: case 0xBF: case 0xC0: case 0xC1: case 0xC9:
        get_joint_wmat_em(enemy, work->joint_0x0C, &mtx);
        copyVec3(&ofs, &work->offset_0x10);
        mulVecMat(&ofs, &mtx);
        mtx34_trans_add(&mtx, &ofs);
        mtx34_trans_get(&mtx, &self->pos_0x18);
        self->pos_0x18.y = 5.0f + enemy->field_0x210;
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        mtx34_set_trans(&mtx, &self->pos_0x18);
        work->effect_0x04->SetRootMtx(mtx);
        break;
    default:
        work->param_0x1C = 0;
        break;
    }
    if (self->area_0x44 == get_now_areano()) {
        switch (self->type_0x02) {
        case 0x14:
            get_joint_wpos_em(enemy, work->joint_0x0C, &ofs);
            se_req_pos_ps(enemy->se_0xB14, 0x19, 2, &ofs);
            break;
        case 0x30:
            se_req_pos_impact(1, &self->pos_0x18);
            break;
        case 0x3F:
            se_req_pos_ps(enemy->se_0xB14, 4, 2, &self->pos_0x18);
            break;
        case 0x6F:
            get_joint_wpos_em(enemy, work->joint_0x0C, &ofs);
            se_req_pos_ps(enemy->se_0xB14, 0x22, 2, &ofs);
            break;
        case 0x88:
        case 0x89:
            se_req_pos_ps(enemy->se_0xB14, 4, 2, &self->pos_0x18);
            break;
        case 0xAB:
            get_joint_wpos_em(enemy, work->joint_0x0C, &self->pos_0x18);
            cpSetRotMatrix(&self->rot_0x24, &mtx);
            copyVec3(&ofs, &work->offset_0x10);
            mulVecMat(&ofs, &mtx);
            addVec3To(&self->pos_0x18, &ofs);
            se_req_pos_ps(enemy->se_0xB14, 0xAA, 2, &self->pos_0x18);
            break;
        case 0xAF:
            se_req_pos_ps(enemy->se_0xB14, 0xA9, 2, &self->pos_0x18);
            break;
        case 0xBC:
            get_joint_wmat_em(enemy, work->joint_0x0C, &mtx);
            mtx34_trans_get(&mtx, &self->pos_0x18);
            copyVec3(&ofs, &work->offset_0x10);
            mulVecMat(&ofs, &mtx);
            addVec3To(&self->pos_0x18, &ofs);
            se_req_pos_ps(enemy->se_0xB14, 0xB4, 2, &self->pos_0x18);
            break;
        case 0xD1:
            se_req_pos_impact(0, &self->pos_0x18);
            break;
        }
    }
    self->flag_0x01 = 1;
    eft_em_move(self);
}

/* 0x803051E8 (0x728): moves the record's effect with its joint, scales it, runs it and tints it with the area's
 * colours; the record ends when the enemy dies, a demo ends, the effect stops or its timer runs out. */
void eft_em_move(_EFT* self) {
    u32 spawn = 1;
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 scale;
    nw4r::math::MTX34 mtx;
    EftEmWork* work;
    struct _ENEMY_WORK* enemy;
    u32 placed;
    _GXColor col;
    u8* keys;
    u32 c;

    VEC3_ctor(&ofs);
    VEC3_ctor(&scale);
    MTX34_ctor(&mtx);
    work = (EftEmWork*)self->work_0x38;
    enemy = (struct _ENEMY_WORK*)self->source_0x30;
    if (em_work_die_ck(enemy) != 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if (self->field_0x07 == 1 && event_demo_ck() == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    switch (self->type_0x02) {
    case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0E: case 0x0F: case 0x12: case 0x13: case 0x20:
    case 0x21: case 0x2B: case 0x37: case 0x3C: case 0x3D: case 0x3E: case 0x3F: case 0x4D: case 0x4E: case 0x51:
    case 0x53: case 0x5B: case 0x60: case 0x68: case 0x70: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76:
    case 0x7C: case 0x7D: case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x8A: case 0x8B: case 0x8C:
    case 0x8D: case 0x8E: case 0x9D: case 0x9E: case 0x9F: case 0xA6: case 0xAB: case 0xAE: case 0xB0: case 0xB1:
    case 0xBA: case 0xBB:
        get_joint_wpos_em(enemy, work->joint_0x0C, &self->pos_0x18);
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        mtx34_set_trans(&mtx, &self->pos_0x18);
        placed = 1;
        break;
    case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1F: case 0x22: case 0x23: case 0x24: case 0x25:
    case 0x26: case 0x27: case 0x28: case 0x29: case 0x2A: case 0x30: case 0x36: case 0x39: case 0x3B: case 0x40:
    case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x47: case 0x48: case 0x49: case 0x4A: case 0x4B:
    case 0x4C: case 0x4F: case 0x50: case 0x52: case 0x54: case 0x55: case 0x56: case 0x57: case 0x58: case 0x59:
    case 0x5C: case 0x5D: case 0x5E: case 0x5F: case 0x61: case 0x62: case 0x63: case 0x65: case 0x66: case 0x67:
    case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6F: case 0x77: case 0x78: case 0x79: case 0x7A: case 0x7B:
    case 0x7F: case 0x85: case 0x86: case 0x87: case 0x88: case 0x89: case 0x90: case 0x91: case 0x92: case 0x93:
    case 0x94: case 0x95: case 0x96: case 0x97: case 0x98: case 0x99: case 0x9A: case 0x9B: case 0xA7: case 0xA8:
    case 0xA9: case 0xAA: case 0xAC: case 0xAD: case 0xAF: case 0xB2: case 0xB4: case 0xB5: case 0xB6: case 0xB7:
    case 0xB9: case 0xBD: case 0xBE: case 0xBF: case 0xC0: case 0xC1: case 0xC4: case 0xC5: case 0xC7: case 0xC9:
    case 0xCA: case 0xCB: case 0xCC:
        placed = 0;
        break;
    case 0xD0:
        get_joint_wmat_em(enemy, work->joint_0x0C, &mtx);
        setVector3(&scale, 1.0f, 1.0f, 1.0f);
        setScaleMat34W(&mtx, &scale);
        placed = 1;
        break;
    default:
        get_joint_wmat_em(enemy, work->joint_0x0C, &mtx);
        placed = 1;
        break;
    }
    if (placed == 1) {
        copyVec3(&ofs, &work->offset_0x10);
        mulVecMat(&ofs, &mtx);
        mtx34_trans_add(&mtx, &ofs);
        switch (self->type_0x02) {
        case 14: case 18: case 55: case 60: case 61: case 91: case 104: case 116: case 117: case 129:
        case 131: case 138: case 139: case 142: case 157: case 158: case 159: case 160: case 161: case 162: case 163:
        case 184: case 206:
            mtx.m[1][3] = 5.0f + enemy->field_0x20C;
            break;
        case 15: case 19: case 62: case 66: case 118:
            mtx.m[1][3] = 5.0f + enemy->field_0x210;
            break;
        case 112: case 114: case 126:
            mtx.m[1][3] = enemy->field_0x20C - work->offset_0x10.y;
            break;
        }
        mtx34_trans_get(&mtx, &self->pos_0x18);
        rotLocalMatY(work->param_0x1C, &mtx);
        work->effect_0x04->SetRootMtx(mtx);
    }
    change_paramscale_eff(work->effect_0x04, work->scale_0x08);
    switch (self->type_0x02) {
    case 0x2C:
        if (self->timer_0x0C-- <= 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        effect_move(work->effect_0x04);
        break;
    default:
        if (effect_move(work->effect_0x04) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        break;
    }
    switch (self->type_0x02) {
    case 0x15: case 0x16: case 0x17: case 0x2D: case 0x2E: case 0x2F:
        self->timer_0x0C++;
        switch (self->type_0x02) {
        case 21:
        case 45:
            keys = eft_em_key_rgb_a;
            break;
        case 22:
        case 46:
            keys = eft_em_key_rgb_b;
            break;
        case 23:
        case 47:
            keys = eft_em_key_rgb_c;
            break;
        }
        eftGetKeyRGB(keys, self->timer_0x0C, &col.r, &col.g, &col.b);
        col.a = 0xFF;
        change_color_eff(work->effect_0x04, &self->pos_0x18, col);
        break;
    case 0x0C: case 0x0E: case 0x12: case 0x19: case 0x1A: case 0x22: case 0x23: case 0x44: case 0x45: case 0x55:
    case 0x5A: case 0x5B: case 0x68: case 0x69: case 0x6B: case 0x70: case 0x71: case 0x72: case 0x73: case 0x74:
    case 0x75: case 0x7C: case 0x7D: case 0x7E: case 0x82: case 0x83: case 0x8A: case 0x8B: case 0x8C: case 0x8D:
    case 0x8E: case 0x8F: case 0x90: case 0x9D: case 0x9E: case 0x9F: case 0xA0: case 0xA1: case 0xA2: case 0xA3:
    case 0xA4: case 0xAD: case 0xB5: case 0xB8: case 0xCE:
        c = get_stg_eft_col(self->area_0x44, 0);
        col.r = (c & 0xFF000000) >> 24;
        col.g = (c & 0xFF0000) >> 16;
        col.b = (c & 0xFF00) >> 8;
        col.a = 0xFF;
        change_color_eff(work->effect_0x04, &self->pos_0x18, col);
        break;
    case 0x03: case 0x10: case 0x11: case 0x1C: case 0x1D: case 0x1E: case 0x1F: case 0x20: case 0x21: case 0x27:
    case 0x28: case 0x2A: case 0x2B: case 0x2C: case 0x30: case 0x31: case 0x37: case 0x3A: case 0x3B: case 0x3C:
    case 0x3D: case 0x3F: case 0x40: case 0x41: case 0x42: case 0x43: case 0x46: case 0x4D: case 0x4E: case 0x5C:
    case 0x5D: case 0x5E: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: case 0x98: case 0x99:
    case 0xD1:
        {
            if (get_camera_pos().y > 0.0f) {
                spawn = 0;
            } else {
                c = get_stg_eft_col(self->area_0x44, 1);
                col.r = (c & 0xFF000000) >> 24;
                col.g = (c & 0xFF0000) >> 16;
                col.b = (c & 0xFF00) >> 8;
                col.a = 0xFF;
                change_color_eff(work->effect_0x04, &self->pos_0x18, col);
            }
        }
        break;
    case 0x32: case 0x38: case 0x3E:
        {
            if (get_camera_pos().y < 0.0f) {
                spawn = 0;
            } else {
                c = get_stg_eft_col(self->area_0x44, 1);
                col.r = (c & 0xFF000000) >> 24;
                col.g = (c & 0xFF0000) >> 16;
                col.b = (c & 0xFF00) >> 8;
                col.a = 0xFF;
                change_color_eff(work->effect_0x04, &self->pos_0x18, col);
            }
        }
        break;
    case 0x0A: case 0x0B: case 0x0D: case 0x0F: case 0x13: case 0x18: case 0x24: case 0x33: case 0x34: case 0x50:
    case 0x56: case 0x57: case 0x58: case 0x59: case 0x6A: case 0x6C: case 0x76: case 0x9A: case 0x9B: case 0x9C:
    case 0xB2: case 0xB3: case 0xB4: case 0xBD: case 0xBE: case 0xBF: case 0xC0: case 0xC1: case 0xC2: case 0xC3:
    case 0xC6: case 0xC8: case 0xC9: case 0xCC: case 0xCD: case 0xCF:
        c = get_stg_eft_col(self->area_0x44, 1);
        col.r = (c & 0xFF000000) >> 24;
        col.g = (c & 0xFF0000) >> 16;
        col.b = (c & 0xFF00) >> 8;
        col.a = 0xFF;
        change_color_eff(work->effect_0x04, &self->pos_0x18, col);
        break;
    case 0x25: case 0x26: case 0x36: case 0x48: case 0x7B:
        if (em_parts_damage_level_get(enemy, 0) >= 2) {
            col.r = 0x7F;
            col.g = 0x7F;
            col.b = 0x7F;
            col.a = 0x7F;
            change_color_eff(work->effect_0x04, &self->pos_0x18, col);
        }
        break;
    case 0x47:
        switch (self->demo_flag_0x08) {
        case 0:
        case 1:
        case 2:
            c = get_stg_eft_col(self->area_0x44, 0);
            break;
        case 3:
            c = get_stg_eft_col(self->area_0x44, 1);
            break;
        }
        col.r = c >> 24;
        col.g = (c >> 16) & 0xFF;
        col.b = (c >> 8) & 0xFF;
        col.a = c;
        change_color_eff(work->effect_0x04, &self->pos_0x18, col);
        break;
    case 0x79: case 0x7A:
        if (em_parts_damage_level_get(enemy, 2) >= 2) {
            col.r = 0x7F;
            col.g = 0x7F;
            col.b = 0x7F;
            col.a = 0x7F;
            change_color_eff(work->effect_0x04, &self->pos_0x18, col);
        }
        break;
    }
    if (spawn == 1) {
        eft_res_models_spawn(self, (void**)&work->effect_0x04, 1, work->count_0x00, NULL);
    }
}

/* 0x80305910 (0x10): the end step - advances to the kill step. */
void eft_em_end(_EFT* self) {
    self->state_0x05++;
}

/* 0x80305920 (0x4): the kill step - returns the record to the pool. */
void eft_em_kill(_EFT* self) {
    eft_res_slot_release(self);
}

/* 0x80305924 (0xC): spawns the debris effect `id` at joint `joint` of `self`, tinted `color`, for 10 frames. */
void eft_em_spawn_tinted(struct _ENEMY_WORK* self, u32 id, u32 joint, nw4r::math::VEC3* pos, f32 scale, u32 color) {
    eft_em_tint_create(self, id, joint, pos, scale, color, 10);
}

/* 0x80305930 (0x8): the same spawn with the caller's frame count. */
void eft_em_spawn_tinted_ex(struct _ENEMY_WORK* self, u8 id, u32 joint, nw4r::math::VEC3* pos, f32 scale, u32 color,
                            u32 life) {
    eft_em_tint_create(self, id, joint, pos, scale, color, life);
}

/* 0x80305938 (0x21C): pools a debris record for `self`: one model per piece, each sized from the id's table with a
 * random step, then the joint, offset, scale and colour. */
_EFT* eft_em_tint_create(struct _ENEMY_WORK* self, u8 id, u32 joint, nw4r::math::VEC3* pos, f32 scale, u32 color,
                         s32 life) {
    _EFT* rec;
    EftTintWork* work;
    s32 i;

    if (self->area_no != get_now_areano()) {
        return NULL;
    }
    rec = (_EFT*)eft_res_slot_get(0xE8);
    if (rec == NULL) {
        return NULL;
    }
    work = (EftTintWork*)rec->work_0x38;
    work->effect_count_0xC4 = 1;
    work->count_0x00 = eft_tint_piece_counts[id];
    rec->type_0x02 = id;
    rec->release_0x40 = eft_em_tint_release;
    rec->dispatch_0x34 = eft_em_tint_dispatch;
    for (i = 0; i < work->count_0x00; i++) {
        work->pieces_0x04[i].model_0x04 = (MHchar*)eft_res_model_get();
        if (work->pieces_0x04[i].model_0x04 == NULL) {
            eft_res_slot_release(rec);
            return NULL;
        }
        work->pieces_0x04[i].size_0x14 = scale * (eft_tint_sizes[id] + 0.1f * (f32)(ran_suu(0) & 3));
        work->pieces_0x04[i].alpha_0x01 = 0xFF;
    }
    rec->field_0x03 = 0x28;
    rec->source_0x30 = self;
    rec->timer_0x0C = 40;
    rec->rot_0x24.x = 0;
    rec->rot_0x24.y = 0;
    rec->area_0x44 = self->area_no;
    rec->field_0x10 = life;
    copyVec3(&work->offset_0xD8, pos);
    work->joint_0xCC = joint;
    work->scale_0xD4 = scale * get_em_scale(self);
    work->color_0xE4.r = (color & 0xFF000000) >> 24;
    work->color_0xE4.g = (color & 0xFF0000) >> 16;
    work->color_0xE4.b = (color & 0xFF00) >> 8;
    work->color_0xE4.a = color;
    eft_state_flags_set(rec, 0, 0);
    return rec;
}

/* 0x80305B54 (0x78): releases the debris record's piece models and its effect. */
void eft_em_tint_release(_EFT* self) {
    s32 i;
    EftTintWork* work = (EftTintWork*)self->work_0x38;

    for (i = 0; i < work->count_0x00; i++) {
        eft_res_models_release(&work->pieces_0x04[i].model_0x04, 1);
    }
    push_eft_effect_heap_num(&work->effect_0xC8, work->effect_count_0xC4);
    work->count_0x00 = 0;
}

/* 0x80305BCC (0x3C): steps the debris record by state: create, move, end, kill. */
void eft_em_tint_dispatch(_EFT* self) {
    switch (self->state_0x05) {
    case 0:
        eft_em_tint_model_create(self);
        break;
    case 1:
        eft_em_tint_move(self);
        break;
    case 2:
        eft_em_tint_end(self);
        break;
    case 3:
        eft_em_tint_kill(self);
        break;
    }
}

/* 0x80305C08 (0x674): places the debris on its joint, creates its effect (the splash variant under water) and its
 * piece models, launches each piece in a random direction and picks the pieces' visible parts. */
void eft_em_tint_model_create(_EFT* self) {
    nw4r::math::VEC3 vel;
    nw4r::math::VEC3 ofs;
    nw4r::math::MTX34 mtx;
    EftTintWork* work = (EftTintWork*)self->work_0x38;
    struct _ENEMY_WORK* enemy = (struct _ENEMY_WORK*)self->source_0x30;
    s32 i;
    u8 kind;
    u8 pick;
    s32 end;
    u8 type;
    s32 part;

    VEC3_ctor(&vel);
    VEC3_ctor(&ofs);
    MTX34_ctor(&mtx);
    self->state_0x05++;
    copyVec3(&ofs, &work->offset_0xD8);
    get_joint_wmat_em(enemy, work->joint_0xCC, &mtx);
    mtx34_trans_get(&mtx, &self->pos_0x18);
    mulVecMat(&ofs, &mtx);
    addVec3To(&self->pos_0x18, &ofs);
    type = self->type_0x02;
    if (type - 1U > 1) {
        if (type == 0) {
            if (enemy->field_0x1E2 != 2 || !(self->pos_0x18.y < enemy->field_0x210)) {
                work->effect_0xC8 =
                    res_eft_create(eft_tint_res_ids[type], eft_tint_res_groups[type], 0);
                if (work->effect_0xC8 == NULL) {
                    eft_em_tint_kill(self);
                    return;
                }
                kind = 0;
            } else {
                work->effect_0xC8 = res_eft_create(0x1F5, 0x2B, 0);
                if (work->effect_0xC8 == NULL) {
                    eft_em_tint_kill(self);
                    return;
                }
                kind = 1;
            }
        }
    } else {
        work->effect_0xC8 = res_eft_create(eft_tint_res_ids[type], eft_tint_res_groups[type], 0);
        if (work->effect_0xC8 == NULL) {
            eft_em_tint_kill(self);
            return;
        }
        kind = 0;
    }
    change_paramscale_eff(work->effect_0xC8, work->scale_0xD4);
    SetRootMtxTrans(work->effect_0xC8, &self->pos_0x18);
    for (i = 0; i < work->count_0x00; i++) {
        if (res_eft_model_create(work->pieces_0x04[i].model_0x04, eft_tint_model_ids[self->type_0x02], 0x18) == NULL) {
            eft_em_tint_kill(self);
            return;
        }
        work->pieces_0x04[i].model_0x04->ready = 1;
        copyVec3((nw4r::math::VEC3*)&work->pieces_0x04[i].model_0x04->pos_0x04, &self->pos_0x18);
        work->pieces_0x04[i].model_0x04->scale_0x1C.x = work->pieces_0x04[i].size_0x14;
        work->pieces_0x04[i].model_0x04->scale_0x1C.y = work->pieces_0x04[i].size_0x14;
        work->pieces_0x04[i].model_0x04->scale_0x1C.z = work->pieces_0x04[i].size_0x14;
        work->pieces_0x04[i].model_0x04->field_0x28 = ran_suu(0);
        work->pieces_0x04[i].model_0x04->field_0x2C = ran_suu(0);
        work->pieces_0x04[i].model_0x04->field_0x30 = ran_suu(0);
        if (i < eft_tint_group_a_counts[self->type_0x02]) {
            setVector3(&vel, 0.0f, 0.0f, eft_tint_speed_a[kind][self->type_0x02]);
            work->fall_0xD0 = eft_tint_fall_a[kind][self->type_0x02];
        } else {
            setVector3(&vel, 0.0f, 0.0f, eft_tint_speed_b[kind][self->type_0x02]);
            work->fall_0xD0 = eft_tint_fall_b[kind][self->type_0x02];
        }
        rotVecX(&vel, 0xC000 - (u16)(0.5f + 65536.0f * (f32)(u32)(ran_suu(0) % 60) / 360.0f));
        rotVecY(&vel, ran_suu(0));
        copyVec3((nw4r::math::VEC3*)&work->pieces_0x04[i].vel_0x08, &vel);
    }
    switch (self->type_0x02) {
    case 0:
        for (i = 0; i < work->count_0x00; i++) {
            if (ran_suu(0) & 1) {
                for (part = eft_tint_part_first[self->type_0x02]; part < (end = eft_tint_part_mid[self->type_0x02]);
                     part++) {
                    work->pieces_0x04[i].model_0x04->setVisibility(part, 1);
                }
                for (; end < eft_tint_part_end[self->type_0x02]; end++) {
                    work->pieces_0x04[i].model_0x04->setVisibility(end, 0);
                }
            } else {
                for (part = eft_tint_part_first[self->type_0x02]; part < (end = eft_tint_part_mid[self->type_0x02]);
                     part++) {
                    work->pieces_0x04[i].model_0x04->setVisibility(part, 0);
                }
                for (; end < eft_tint_part_end[self->type_0x02]; end++) {
                    work->pieces_0x04[i].model_0x04->setVisibility(end, 1);
                }
            }
        }
        break;
    case 1:
        for (i = 0; i < 2; i++) {
            pick = (ran_suu(0) & 1) + 1;
            for (part = eft_tint_part_first[self->type_0x02]; part < eft_tint_part_end[self->type_0x02]; part++) {
                if (part == pick) {
                    work->pieces_0x04[i].model_0x04->setVisibility(part, 1);
                } else {
                    work->pieces_0x04[i].model_0x04->setVisibility(part, 0);
                }
            }
        }
        for (i = 2; i < work->count_0x00; i++) {
            pick = (ran_suu(0) & 1) + 3;
            for (part = eft_tint_part_first[self->type_0x02]; part < eft_tint_part_end[self->type_0x02]; part++) {
                if (part == pick) {
                    work->pieces_0x04[i].model_0x04->setVisibility(part, 1);
                } else {
                    work->pieces_0x04[i].model_0x04->setVisibility(part, 0);
                }
            }
        }
        if (self->area_0x44 == get_now_areano()) {
            se_req_pos_ps(enemy->se_0xB14, 0x25, 2, &self->pos_0x18);
        }
        break;
    }
    self->demo_flag_0x08 = kind;
    self->flag_0x01 = 1;
    eft_em_tint_move(self);
}

/* 0x8030627C (0x294): after the record's delay, moves each piece (drag, fall, spin), fades it out over the last 10
 * frames, and runs and tints the effect; the record ends when its timer runs out. */
void eft_em_tint_move(_EFT* self) {
    _GXColor col;
    s32 i;
    u16* mask;
    EftTintWork* work = (EftTintWork*)self->work_0x38;

    if (--self->field_0x10 < 0) {
        if (--self->timer_0x0C < 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        col.r = work->color_0xE4.r;
        col.g = work->color_0xE4.g;
        col.b = work->color_0xE4.b;
        mask = eft_tint_spin_masks[self->demo_flag_0x08];
        for (i = 0; i < work->count_0x00; i++) {
            if (i < eft_tint_group_a_counts[self->type_0x02]) {
                vec3_scale_in_place((VEC3*)&work->pieces_0x04[i].vel_0x08, eft_tint_drag_a[self->demo_flag_0x08][self->type_0x02]);
            } else {
                vec3_scale_in_place((VEC3*)&work->pieces_0x04[i].vel_0x08, eft_tint_drag_b[self->demo_flag_0x08][self->type_0x02]);
            }
            work->pieces_0x04[i].vel_0x08.y -= work->fall_0xD0;
            addVec3To(&work->pieces_0x04[i].model_0x04->pos_0x04, &work->pieces_0x04[i].vel_0x08);
            work->pieces_0x04[i].model_0x04->field_0x28 += mask[self->type_0x02] & ran_suu(0);
            work->pieces_0x04[i].model_0x04->field_0x2C += mask[self->type_0x02] & ran_suu(0);
            work->pieces_0x04[i].model_0x04->field_0x30 += mask[self->type_0x02] & ran_suu(0);
            switch (work->pieces_0x04[i].fade_0x00) {
            case 0:
                if (self->timer_0x0C < 10) {
                    work->pieces_0x04[i].fade_0x00 = 1;
                }
                break;
            case 1:
                if (self->timer_0x0C > 0) {
                    work->pieces_0x04[i].alpha_0x01 -= 25;
                } else {
                    work->pieces_0x04[i].alpha_0x01 = 0;
                }
                break;
            }
            col.a = work->pieces_0x04[i].alpha_0x01;
            work->pieces_0x04[i].model_0x04->setTevKColor(0, (_GXTevKColorID)3, &col);
            work->pieces_0x04[i].model_0x04->move(0);
            eft_res_models_spawn(self, (void**)&work->pieces_0x04[i].model_0x04, 2, 1, NULL);
        }
        effect_move(work->effect_0xC8);
        change_color_eff(work->effect_0xC8, &self->pos_0x18, work->color_0xE4);
        eft_res_models_spawn(self, (void**)&work->effect_0xC8, 1, 1, NULL);
    }
}

/* 0x80306510 (0x10): the debris end step - advances to the kill step. */
void eft_em_tint_end(_EFT* self) {
    self->state_0x05++;
}

/* 0x80306520 (0x4): the debris kill step - returns the record to the pool. */
void eft_em_tint_kill(_EFT* self) {
    eft_res_slot_release(self);
}

/* 0x80306524 - builds the effect-41 spawner: pools a 0x20-byte effect record, installs the
 * `eft041_dispatch` dispatcher / `eft041_release` releaser, fills its one model slot and arms it. */
void lb_area_fx_set(void) {
    u8 area = get_now_areano();
    u8* res;
    s32 i;
    _EFT* rec;
    Eft8030Work* w;

    res = stage_area_word_get(area);
    if (res == NULL) {
        return;
    }
    rec = (_EFT*)eft_res_slot_get(0x20);
    if (rec == NULL) {
        return;
    }
    rec->dispatch_0x34 = eft041_dispatch;
    rec->release_0x40 = eft041_release;
    w = (Eft8030Work*)rec->work_0x38;
    w->source_0x01C = res;
    w->count = 1;
    for (i = 0; i < w->count; i++) {
        w->slots_0x004[i].effect = (nw4r::ef::Effect*)eft_res_model_get();
        if (w->slots_0x004[i].effect == NULL) {
            eft_res_slot_release(rec);
            return;
        }
    }
    memset(w->models_0x00C, 0, 0x10);
    rec->field_0x03 = 0x29;
    eft_state_flags_set(rec, 8, 0);
    rec->area_0x44 = area;
}

/* 0x80306614 - releases the effect-41 spawner's models and pooled handles. */
void eft041_release(_EFT* self) {
    s32 i;
    s32 j;
    Eft8030Work* w = (Eft8030Work*)self->work_0x38;

    for (i = 0; i < w->count; i++) {
        for (j = 3; j >= 0; j--) {
            if (w->models_0x00C[i].parts_0x000[j] != NULL) {
                push_g3d_wk((_g3d_work*)w->models_0x00C[i].parts_0x000[j]);
            }
        }
        if (w->slots_0x004[i].effect != NULL) {
            eft_res_models_release(&w->slots_0x004[i], 1);
        }
    }
}

/* 0x803066B4 (0x3C): steps the area effect by state: create, move, end, kill. */
void eft041_dispatch(_EFT* self) {
    switch (self->state_0x05) {
    case 0:
        eft041_model_create(self);
        break;
    case 1:
        eft041_move(self);
        break;
    case 2:
        eft041_end(self);
        break;
    case 3:
        eft041_kill(self);
        break;
    }
}

} /* extern "C" */
