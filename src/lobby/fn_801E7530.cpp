/* lobby/fn_801E7530.cpp - the lobby menu-layer group.
 * RANGE. .text 0x801E7530-0x801EC9E0 (77 functions); .data 0x805B7CB0-0x805B85D8, .sdata 0x80791B70-0x80791C58, extab,
 *   extabindex.
 * NAMES. The map and the dump give only placeholders for the range, so the file keeps the map's stem.  Module `lobby`:
 *   the callees are the lobby UI API (`lobby_w`, `LbStr`, `lb_param_w`, `get_lsp_data`, `GetMenuFontColor`,
 *   `set_blendmode`, `draw_font_idx`, `draw_sprite_ary`).  Four families: the delivery counter (`lb_deli_*`), the free
 *   hunt's day schedule (`lb_hunt_day_*`, `lb_daily_menu_roll`), the farm (lobby screen 5, `lb_farm_*`: plots in the
 *   user data at +0x3F38, seeds, fertilizers, harvest) and the item box screen (lobby screen 6, `lb_box_*`).
 *   GUESS (from each body, its tables and its callers): `lb_world_submode_get` (the farm plot count),
 *   `lb_deli_screen_draw`, `lb_hunt_day_roll`, `lb_hunt_day_advance`, `lb_rand_pick_take`, every `lb_farm_*` function
 *   and table, `lb_box_cam_input_step`, `lb_box_model_turn`, `lb_box_model_turn_step`.
 * FLAGS. `#pragma peephole off` file-wide (retail keeps `clrlwi`/`rlwinm` + `cmpwi` pairs and narrows u8/u16 parameters at
 *   entry), back on for `lb_talk_screen_open` (it measures better with the pass); `#pragma opt_unroll_loops off` around
 *   `lb_daily_menu_roll` (retail keeps its 17-row clear as a `mtctr` loop).
 * RESIDUALS. Every row is written.
 *  - `lb_daily_menu_roll`: retail clears the point rows through three zero copies and indexes them `n * 3 * 2`; ours
 *    stores one zero and multiplies by 6;
 *  - `lb_rand_pick_take`: retail's eight-fold unroll checks `i < 14` where ours checks the 32-bit bound (12 B longer);
 *  - `fn_801E9AA8`: retail unrolls the three plots keeping `li`/`mulli` index products; ours strength-reduces (28 B);
 *  - `fn_801E7530`: retail tests the 0..2 range unsigned and cases 3..5 signed; ours tests all unsigned;
 *  - `fn_801E843C`, `fn_801E80FC`, `fn_801E8D5C`, `lb_farm_seed_list_draw`, `lb_farm_harvest`, `lb_farm_day_pass`,
 *    `lb_farm_plot_info_draw`, `lb_farm_plot_tabs_draw`, `lb_deli_screen_draw`, `fn_801E89B4`: register colouring and
 *    induction-variable order;
 *  - `lb_farm_seed_ck`: ours folds `seed >= 8 && seed < 12` into one range compare;
 *  - `fn_801E936C`: retail returns through `ble` + `clrlwi`, ours through `blelr`.
 *   flipcheck: `.data`/`.sdata` claimed, not emitted (the tables are declared, not defined).
 */
#include "types.h"

#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

#include "id_value.h"
#include "ef/fn_800CDB2C.h"        /* `ran_suu`, `game_ready_ck` (owner: `ef/fn_800CDB2C.cpp`, rule 2) */
#include "lobby/lb_cmd_pressed_ck.h" /* the command-mask testers (owner: `lobby/lb_npc.cpp`, rule 2) */
#include "lobby/lb_list_init.h"     /* the list/panel helpers (owner: `lobby/fn_80212810.cpp`, rule 2) */
#include "lobby/lb_screen05_open.h"   /* this unit's own leaf header (its consumer is `lobby/fn_80219260.cpp`) */
#include "unsplit/lobby.h"
#include "hud/cockpit.h"
#include "lobby/lb_npc.h"
#include "menu/menu_item.h"   /* `put_menu_cursor` (owner: `menu/menu_item.cpp`, rule 2) */

/* `sprintf` is a libc intrinsic; `-nosyspath` means it has to be declared here (the owner unit has no
 * publishable header). */
extern "C" {
int sprintf(char* dst, const char* fmt, ...);
}

/* Foreign functions whose owners' headers do not declare them, as plain prototypes (rule 2 debt). */
extern "C" {
void item_pair_copy(void* dst, const void* src);
s32 userdata_item_count_total(u16 id, void* out);
s32 item_take(s16 a, void* b, s32 c, s32 d, s32 e);
void* get_FqResult_work(void);
s32 userdata_progress_flag_ck(s32 a);
s32 userdata_zenny_add(s32 a);
void uv_pair_copy(_mh_ivec2_* dst, const _mh_ivec2_* src);
u16 rand_lcg_step(u16 id);
s32 sysSE_stop(s32 id);
}

/* This unit's own functions that earlier bodies call; C linkage, like their definitions. */
extern "C" {
void fn_801E7530(LbMenuWork* self, s16 list_mode);
void fn_801E78B4(LbMenuWork* self);
s32 fn_801E790C(LbMenuWork* self);
void fn_801E80FC(LbMenuWork* self);
void fn_801E89B4(LbMenuWork* self);
void fn_801E8C44(void);
void fn_801E88C4(LbStrBlock* dst, const LbStrBlock* src);
void fn_801E8CDC(u16 id, s8* str);
void fn_801E9AA8(u8 flag);
void lb_farm_box_draw(void);
void fn_801EB744(u16 id, _mh_ivec2_* off);
void lb_npc_choice_draw(LbList* list);
void fn_801E843C(LbMenuWork* self);
void fn_801E8348(LbMenuWork* self);
void fn_801E8CA0(void);
void fn_801E8D5C(LbMenuWork* self);
void fn_801E88E8(u8 mode);
u16 fn_801E936C(u16 a, u16 b);
s32 fn_801E9600(u8* list, u8 count, u8 id);
void fn_801E9FC8(void);
void fn_801EAC30(LbParam* dst, const LbParam* src);
u8 lb_rand_pick_take(u8* pool);
s8 lb_farm_seed_ck(const struct LbFarmPlot* plot, u8 seed);
s8 lb_farm_plot_days_get(const struct LbFarmPlot* plot);
void lb_farm_screen_draw(void);
void lb_deli_screen_draw(void);
void lb_box_model_turn_step(struct LbBoxWork* self);
void lb_farm_plot_tabs_draw(struct LbFarmWork* self);
}

/* Foreign C++-linkage callees (their map names are manglings); the front-end reproduces those names from
 * these declarations, and rule 9 forbids spelling the manglings at the call site. */
s32 ck_WideMode(void);
void font_set_size(s16 w, s16 h);
void set_blendmode(u8 a, u8 b, u8 c);
void set_zmode(bool a, u8 b, bool c);
void subTransSet(u32 a, s32 b, u32* c);
void sysSE_req(s32 id);

/* Retail keeps the unfused `clrlwi`/`rlwinm` + `cmpwi` pairs (playbook 39). */
#pragma peephole off

/* ------------------------------------------------------------------------------------------------ */

/* One row of the free-hunt monster table (`lbl_805B7A88`, .data 0x805B7A88): the monster's kind and the unlock level
 * it needs (0xFFFF: never drawn).  The table ends with kind 0xFF.  size: 0x4 */
typedef struct LbHuntMonsterRow {
    /* +0x0 */ u8 kind_0x0;
    /* +0x1 */ u8 pad_0x1;
    /* +0x2 */ u16 level_0x2;
} LbHuntMonsterRow;

/* Today's free-hunt day in the user data (`LbUserDataView::hunt_today_0x4068`): the two monsters, the area variant
 * and the flag, packed.  size: 0x6 */
typedef struct LbHuntToday {
    /* +0x0 */ u8 monster_0x0[2];
    /* +0x2 */ u16 variant_0x2;
    /* +0x4 */ u16 flag_0x4;
} LbHuntToday;

/* One coming day of the free hunt's schedule in the user data (tomorrow and the day after): the two monsters drawn
 * for it, the area variant (0..7) and the 80 % flag `lb_hunt_day_roll` draws.  size: 0x8 */
typedef struct LbHuntDay {
    /* +0x0 */ u8 monster_0x0[2];
    /* +0x2 */ u8 pad_0x2[2];
    /* +0x4 */ u16 variant_0x4;
    /* +0x6 */ u16 flag_0x6;
} LbHuntDay;

/* One farm plot of the user data (`LbUserDataView::farm_plots_0x3F38`).  size: 0xC */
typedef struct LbFarmPlot {
    /* +0x0 */ u8 state_0x0;          /* 0 empty, 1 ready to harvest, 2 growing */
    /* +0x1 */ u8 grade_0x1;          /* the soil grade the day roll draws (`weight_table_index`); 3 adds a level */
    /* +0x2 */ u8 level_0x2;          /* the plot's level, against a seed's required level */
    /* +0x3 */ u8 seed_0x3;           /* the planted seed (`lb_farm_seed_rows`'s row), 0xFF for none */
    /* +0x4 */ s8 days_0x4;           /* days left until the harvest */
    /* +0x5 */ u8 bonus_0x5;          /* a seed drawn from the ones the plot can take */
    /* +0x6 */ u16 fertilizer_0x6;    /* the fertilizer item used */
    /* +0x8 */ u16 rng_0x8;           /* the harvest's random seed (`rand_lcg_step`) */
    /* +0xA */ u8 grade_planted_0xA;  /* `grade_0x1` when the seed was planted */
    /* +0xB */ u8 bonus_planted_0xB;  /* `bonus_0x5` when the seed was planted */
} LbFarmPlot;

/* The user-data block `lobby_world_block` points at, as this unit reads it: the farm and the free-hunt schedule.
 * size: 0x4A2A (the named part of the 0x6000-byte block) */
typedef struct LbUserDataView {
    /* +0x0000 */ u8 pad_0x0000[0x3E02];
    /* +0x3E02 */ u8 farm_plot_count_0x3E02;    /* the farm plots open (1..3) */
    /* +0x3E03 */ u8 pad_0x3E03[0x101];
    /* +0x3F04 */ s32 zenny_0x3F04;              /* the money a seed's price is tested against */
    /* +0x3F08 */ u8 pad_0x3F08[0x30];
    /* +0x3F38 */ LbFarmPlot farm_plots_0x3F38[3];
    /* +0x3F5C */ u8 pad_0x3F5C[0x10C];
    /* +0x4068 */ LbHuntToday hunt_today_0x4068;
    /* +0x406E */ u8 pad_0x406E[0x7E2];
    /* +0x4850 */ LbHuntDay hunt_days_0x4850[2];          /* tomorrow and the day after */
    /* +0x4860 */ u8 item_box_0x4860[0x78];       /* the item box `item_take` stores into */
    /* +0x48D8 */ u16 hunted_0x48D8[0x80];        /* the hunt counts per monster, capped at 99 */
    /* +0x49D8 */ u16 captured_0x49D8[0x29];      /* the capture counts per monster, capped at 99 */
} LbUserDataView;

#define LB_USER_DATA ((LbUserDataView*)lobby_world_block)

/* The free-hunt result as this unit takes it (`get_FqResult_work`): the hunt and capture counts and the reward box.
 * size: 0x170 (the named part of the 0x27C-byte work) */
typedef struct LbFqResultView {
    /* +0x000 */ u16 hunted_0x000[0x29];
    /* +0x052 */ u16 captured_0x052[0x29];
    /* +0x0A4 */ u8 pad_0x0A4[0x54];
    /* +0x0F8 */ IdValue rewards_0x0F8[30];
} LbFqResultView;

/* The model the box screen turns (its `+0x10` work pointer): the yaw the pad's stick bits step.  size: 0xAC (the named
 * part) */
typedef struct LbTalkModel {
    /* +0x00 */ u8 pad_0x00[0xA8];
    /* +0xA8 */ s32 yaw_0xA8;
} LbTalkModel;

/* The farm screen's work (lobby screen 5, `lobby_w.menu_0xAC`; `lb_screen05_open` clears its 0x2000 bytes).
 * size: 0x2000 (the named part ends at 0xD6) */
typedef struct LbFarmWork {
    /* +0x00 */ u8 state_0x00;            /* the screen step (0 opening .. 5 closing) */
    /* +0x01 */ u8 step_0x01;             /* the plot menu's step */
    /* +0x02 */ u8 seed_menu_0x02;        /* 1 while the action cursor stands on the seed row */
    /* +0x03 */ u8 changed_0x03;          /* set once a seed or fertilizer was picked */
    /* +0x04 */ s16 action_0x04;          /* the plot menu's cursor: 0 seed, 1 fertilizer, 2 plant */
    /* +0x06 */ u16 page_moved_0x06;      /* the seed page step's moved mask */
    /* +0x08 */ s16 plot_0x08;            /* the plot picked */
    /* +0x0A */ s16 plot_count_0x0A;      /* the plots open */
    /* +0x0C */ s16 seed_cursor_0x0C;
    /* +0x0E */ s16 seed_count_0x0E;      /* the seeds of the page the plot can take */
    /* +0x10 */ s16 seed_page_0x10;       /* the seed tier shown */
    /* +0x12 */ s16 seed_pages_0x12;
    /* +0x14 */ u16 blink_0x14;           /* the confirm window's blink counter (0..19) */
    /* +0x16 */ u16 plot_look_0x16;       /* the plot frame's look: 0 empty, 1 in use, 2 just planted */
    /* +0x18 */ s16 fertilizer_count_0x18;
    /* +0x1A */ u8 pad_0x1A[2];
    /* +0x1C */ s32 yes_no_0x1C;          /* the yes/no answer `toggle_word_step`/`lb_yes_no_step` move */
    /* +0x20 */ LbList choice_0x20;       /* the opening choice list */
    /* +0x40 */ struct _PLW* player_0x40;  /* the player and NPC `lb_screen05_open` is handed */
    /* +0x44 */ struct _LB_NPC* npc_0x44;
    /* +0x48 */ LbFarmPlot edit_0x48;     /* the plot being set up */
    /* +0x54 */ IdValue harvest_0x54[30]; /* the harvest, ten rows per plot */
    /* +0xCC */ u16 fertilizers_0xCC[5];  /* the fertilizers the box holds */
} LbFarmWork;

/* One item-get window of the block `lb_item_get_data` points at (`eft052_item_get_open` opens one): the plot count
 * the farm's harvest shows.  size: 0x20 */
typedef struct LbItemGetWindow {
    /* +0x00 */ u8 pad_0x00[2];
    /* +0x02 */ s16 count_0x02;
    /* +0x04 */ u8 pad_0x04[0x1C];
} LbItemGetWindow;

/* The block `lb_item_get_data` points at, as this unit uses it: the delivery's window and the farm's.  size: 0x60
 * (the named part) */
typedef struct LbItemGetView {
    /* +0x00 */ u8 pad_0x00[0x20];
    /* +0x20 */ LbItemGetWindow deli_0x20;
    /* +0x40 */ LbItemGetWindow farm_0x40;
} LbItemGetView;

/* The sprite record `get_lsp_data` returns and `draw_sprite`/`draw_font` take (`_SPR_DATA_`), as this unit fills it:
 * its position and its colour word.  size: 0x24 */
typedef struct LbSprData {
    /* +0x00 */ _mh_ivec2_ pos_0x00;
    /* +0x04 */ u8 pad_0x04[0x18];
    /* +0x1C */ u32 color_0x1C;
    /* +0x20 */ u8 pad_0x20[0x4];
} LbSprData;

/* The item box `eft052_item_box` (.bss 0x806BF310, `ef/eft052.cpp`'s) as the farm draws it: its step, the cursor
 * and mark animations, the marked and cursor cells and the item rows.  size: 0x58 */
typedef struct LbItemBoxView {
    /* +0x00 */ u8 state_0x00;
    /* +0x01 */ u8 pad_0x01[7];
    /* +0x08 */ u16 anim_0x08;
    /* +0x0A */ u16 mark_anim_0x0A;
    /* +0x0C */ u16 marked_0x0C;
    /* +0x0E */ u16 mark_0x0E;
    /* +0x10 */ u16 cursor_0x10;
    /* +0x12 */ u8 pad_0x12[0x12];
    /* +0x24 */ IdValue* items_0x24;
    /* +0x28 */ u8 pad_0x28[0x30];
} LbItemBoxView;

/* The box screen's work (lobby screen 6, `lobby_w.menu_0xAC`; `lb_talk_screen_open` clears its 0x2000 bytes).
 * size: 0x2000 (the named part ends at 0x28) */
typedef struct LbBoxWork {
    /* +0x00 */ u8 state_0x00;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u8 kind_0x02;             /* the kind `lb_talk_screen_open` is handed: 1 the small box */
    /* +0x03 */ u8 manual_cam_0x03;       /* 1 while the camera is turned by hand */
    /* +0x04 */ u8 help_0x04;             /* toggled by the 0x40 command */
    /* +0x05 */ u8 pad_0x05;
    /* +0x06 */ s16 cursor_0x06;
    /* +0x08 */ s16 page_0x08;
    /* +0x0A */ u8 pad_0x0A[2];
    /* +0x0C */ s16 pages_0x0C;           /* 3 for the small box, 7 otherwise */
    /* +0x0E */ s16 row_0x0E;
    /* +0x10 */ LbTalkModel* model_0x10;
    /* +0x14 */ u8 pad_0x14[0x13];
    /* +0x27 */ u8 stick_ready_0x27;      /* 1 once the stick was let go after the screen opened */
} LbBoxWork;

/* ------------------------------------------------------------------------------------------------ */

/* The two 6-byte selection-table walks below share this: a row's flag gates it (`lb_unlock_cond_ck`). */

/* Every map name in this range is a plain `fn_XXXXXXXX`, so the whole batch of definitions takes C
 * linkage (the map does not mangle them). */
extern "C" {
void fn_801E7530(LbMenuWork* self, s16 list_mode)
{
    const u16* flag_row;
    const u16* row;
    s16 count;

    flag_row = lb_trade_flag_rows;
    row = lb_trade_pair_rows;
    self->list_mode_0x0C = list_mode;
    self->count_0x10 = 0;
    count = 0;
    while (flag_row[0] != 0) {
        if (lb_unlock_cond_ck(flag_row[2]) == 0) {
            break;
        }
        count++;
        flag_row += 3;
    }
    if (lb_unlock_cond_ck(0xA) != 0) {
        if (count > 6) {
            self->stage_0x0E = 6;
        } else {
            self->stage_0x0E = 5;
        }
    } else {
        if (count > 6) {
            self->stage_0x0E = 5;
        } else {
            self->stage_0x0E = 4;
        }
    }
    memset(self->entries_0xD0, 0, 0x30);
    switch ((u32)self->list_mode_0x0C) {
    case 0:
    case 1:
    case 2: {
        const LbTableRow* price = &((const LbTableRow*)lb_trade_price_rows)[self->list_mode_0x0C * 6];

        while (price->id_0x00 != 0) {
            self->count_0x10++;
            if (self->count_0x10 >= 6) {
                break;
            }
            price++;
        }
        lb_goods_price_group_get(self, self->list_mode_0x0C);
        break;
    }
    case 3:
        row = lb_trade_flag_rows;
        count = 0;
        while (row[0] != 0) {
            if (lb_unlock_cond_ck(row[2]) == 0) {
                break;
            }
            self->entries_0xD0[self->count_0x10].kind_0x00 = (u8)row[0];
            self->entries_0xD0[self->count_0x10].id_0x02 = row[1];
            self->entries_0xD0[self->count_0x10].icon_kind_0x01 = count;
            self->entries_0xD0[self->count_0x10].value_0x04 = lbex_zaco_str[self->count_0x10];
            self->count_0x10++;
            if (self->count_0x10 >= 6) {
                break;
            }
            row += 3;
        }
        break;
    case 4:
        if (self->stage_0x0E != 3 || (u32)lb_unlock_cond_ck(0xA) != 1) {
            row = lb_trade_flag_rows;
            row += 18;
            count = 0;
            while (row[0] != 0) {
                if (lb_unlock_cond_ck(row[2]) == 0) {
                    break;
                }
                self->entries_0xD0[self->count_0x10].kind_0x00 = (u8)row[0];
                self->entries_0xD0[self->count_0x10].id_0x02 = row[1];
                self->entries_0xD0[self->count_0x10].icon_kind_0x01 = count;
                self->entries_0xD0[self->count_0x10].value_0x04 = lbex_zaco_str[self->count_0x10 + 6];
                self->count_0x10++;
                if (self->count_0x10 >= 6) {
                    break;
                }
                row += 3;
            }
            break;
        }
        /* fall through */
    case 5:
        count = 1;
        while (row[0] != 0) {
            if (lb_unlock_cond_ck(row[3]) == 0) {
                break;
            }
            self->entries_0xD0[self->count_0x10].kind_0x00 = (u8)row[0];
            self->entries_0xD0[self->count_0x10].id_0x02 = row[1];
            self->entries_0xD0[self->count_0x10].icon_kind_0x01 = count;
            self->entries_0xD0[self->count_0x10].value_0x04 = lbex_main_str[self->count_0x10];
            self->count_0x10++;
            if (self->count_0x10 >= 6) {
                break;
            }
            row += 4;
        }
        break;
    }
    if (self->count_0x10 <= self->selected_0x08) {
        self->selected_0x08 = self->count_0x10 - 1;
    }
}

void fn_801E78B4(LbMenuWork* self)
{
    if (lb_unlock_cond_ck(0xA) != 0) {
        self->stage_0x0E = 3;
    } else {
        self->stage_0x0E = 2;
    }
    fn_801E7530(self, 0);
}

s32 fn_801E790C(LbMenuWork* self)
{
    s16 value;
    s32 result = 0;

    if (lb_cmd_pressed_ck(0x20) != 0) {
        result = 2;
        sysSE_req(1);
    } else if (lb_cmd_repeat_ck(0xC) != 0) {
        /* The string block's first halfword receives the moved mask; the cast emits nothing. */
        value = menu_cursor_step_fixed_tail(self->list_mode_0x0C, self->stage_0x0E, lb_cmd_repeat_get(), 4, 8, (u16*)&self->list_0x24.cursor_0x00);
        self->list_mode_0x0C = value;
        fn_801E7530(self, value);
    } else if (lb_cmd_repeat_ck(3) != 0) {
        self->selected_0x08 = menu_cursor_step(self->selected_0x08, self->count_0x10, lb_cmd_repeat_get(), 1, 2);
    }
    return result;
}

void lb_deli_screen_step(void)
{
    LbMenuWork* self;
    u16 flags;

    self = lobby_w.menu_0xAC;
    if (self->open_0x05 != 0) {
        if (++self->timer_0x06 > 5U) {
            self->timer_0x06 = 5U;
        }
    } else if (self->timer_0x06 != 0) {
        self->timer_0x06--;
    }
    self->item_id_0x12 = 0;
    switch (self->state_0x00) {
    case 0:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1U) {
            self->state_0x00 = 1U;
            lb_goods_sale_list_build(self);
            flags = 0;
            if (lb_unlock_cond_ck(5U) == 0 && fn_802089F4(self->data_0x1C) == 0xFFFF) {
                flags |= 4;
            }
            if (lb_unlock_cond_ck(6U) == 0) {
                flags |= 1;
            }
            if (lb_deli_list_build(self->data_0x1C, self->data_0x20) > 0 && lb_unlock_cond_last_ck(4) == 0) {
                lb_choice_init(&self->list_0x24, (const LbListTmpl*)(lbl_805B7CB0 + 20), flags, 0x10);
            } else {
                lb_choice_init(&self->list_0x24, (const LbListTmpl*)lbl_805B7CB0, flags, 0);
            }
            self->list_0x24.se_ok_0x14 = -1;
            self->list_0x24.flags_0x18 = 3;
            camera_talk_lock_set(1);
            sysSE_stop(0x29);
        }
        break;
    case 1:
        ainpc_page_hold_set();
        switch (lb_choice_step(&self->list_0x24)) {
        case 1:
            self->mode_0x01 = 0;
            self->unused_0x02[1] = 0;
            self->list_mode_0x0C = 0;
            switch (self->list_0x24.cursor_0x00) {
            case 0:
                self->state_0x00 = 2U;
                self->list_mode_0x0C = 0;
                self->stage_0x0E = 3;
                self->open_0x05 = 0;
                self->timer_0x06 = 0;
                sysSE_req(0x1B);
                break;
            case 1:
                self->state_0x00 = 3U;
                lb_goods_sale_list_build(self);
                sysSE_req(0x1B);
                break;
            case 2:
                self->state_0x00 = 4U;
                {
                    LbSeParam param;

                    param.value_0x00 = 0x63;
                    param.mode_0x04 = 0xB;
                    param.id_0x06 = -1;
                    param.max_0x0A = 0xC;
                    param.limit_0x08 = 0xF;
                    param.title_0x0E = 0x13E6;
                    param.time_0x10 = 0;
                    param.cur_0x0C = lb_goods_price_group_get(self, 0);
                    param.data_0x14 = &self->flags_0x50[0x10];
                    param.flag_0x18 = 1;
                    eft052_hold_entry_set((s32*)&param, 0);
                }
                sysSE_req(5);
                break;
            case 3:
                self->state_0x00 = 7U;
                fn_801E78B4(self);
                sysSE_req(5);
                break;
            case 4:
                self->state_0x00 = 6U;
                lb_deli_list_build(self->data_0x1C, self->data_0x20);
                sysSE_req(0x1B);
                break;
            }
            break;
        case 2:
            self->state_0x00 = 8U;
            lb_talk_page_open(1);
            sysSE_req(1);
            camera_talk_lock_set(0);
            break;
        }
        break;
    case 7:
        ainpc_page_hold_set();
        if ((u32)(fn_801E790C(self) - 1) <= 1U) {
            self->state_0x00 = 1U;
        }
        break;
    case 6:
        ainpc_page_hold_set();
        switch (lb_deli_menu_step()) {
        case 1:
            return;
        case 2:
            self->state_0x00 = 1U;
            break;
        }
        break;
    case 2:
        ainpc_page_hold_set();
        if (lb_cmd_pressed_ck(0x20) != 0) {
            self->state_0x00 = 1U;
            sysSE_req(1);
        } else if (lb_cmd_pressed_ck(0x80) != 0) {
            self->open_0x05 = self->open_0x05 ^ 1;
            if (self->open_0x05 != 0) {
                sysSE_req(0x1B);
            } else {
                sysSE_req(1);
            }
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->list_mode_0x0C = menu_cursor_step_open_last(self->list_mode_0x0C, self->stage_0x0E, lb_cmd_repeat_get(), 4, 8, 6);
        }
        break;
    case 3:
        ainpc_page_hold_set();
        switch (lb_goods_sale_step(self)) {
        case 1:
            self->state_0x00 = 5U;
            eft052_item_get_open(lobby_world_block + 0x4860, lb_item_get_data + 0x20);
            break;
        case 2:
            self->state_0x00 = 1U;
            break;
        }
        break;
    case 4:
        ainpc_page_hold_set();
        if ((u32)(lb_goods_hold_step(self) - 1) <= 1U) {
            self->state_0x00 = 1U;
        }
        break;
    case 5:
        ainpc_page_hold_set();
        if (eft052_item_get_step() == 1) {
            self->state_0x00 = 1U;
        }
        break;
    case 8:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1U) {
            lb_goods_sale_close();
        }
        break;
    }
    if (self->state_0x00 != 0 && self->state_0x00 != 8) {
        subTransSet((u32)lb_deli_screen_draw, 0, 0);
    }
}

void fn_801E7EA8(u16 id, const _mh_ivec2_* pos)
{
    LbItemData* data;
    u16 a;
    u16 b;
    u8 buf[8];

    data = (LbItemData*)lb_goods_pair_row_get(id);
    draw_sprite_ary(&lbl_805B7D18[0], pos);
    draw_font_idx(0x1DD9U, (s8*)str_tbl_33_get((u8)id), 4, pos);
    a = lb_goods_count_get(id, 0);
    b = lb_goods_count_get(id, 1);
    sprintf((char*)buf, (char*)lbl_80791B88, a);
    draw_font_idx(0x1DDAU, (s8*)buf, 2, pos);
    sprintf((char*)buf, (char*)lbl_80791B88, b);
    draw_font_idx(0x1DDBU, (s8*)buf, 2, pos);
    sprintf((char*)buf, (char*)lbl_80791B88, data->rate_0x02 * a + data->rate_0x04 * b);
    draw_font_idx(0x1DDCU, (s8*)buf, 2, pos);
    draw_font_idx(0x1DDDU, (s8*)LbStr(1, 0x2CU), 0, pos);
    draw_monstericon_idx(0x1DCA, (u8)id, pos);
}

void fn_801E7FEC(u16 id, const _mh_ivec2_* pos)
{
    LbItemData* data;
    u16 a;
    u8 buf[8];

    data = (LbItemData*)lb_goods_flag_row_get(id);
    draw_sprite_ary(&lbl_805B7D38[0], pos);
    draw_font_idx(0x1DEFU, (s8*)str_tbl_33_get((u8)id), 4, pos);
    a = lb_goods_count_get(id, 0);
    sprintf((char*)buf, (char*)lbl_80791B88, a);
    draw_font_idx(0x1DF0U, (s8*)buf, 2, pos);
    sprintf((char*)buf, (char*)lbl_80791B88, data->rate_0x02 * a);
    draw_font_idx(0x1DF1U, (s8*)buf, 2, pos);
    draw_font_idx(0x1DF2U, (s8*)LbStr(1, 0x2CU), 0, pos);
    draw_monstericon_idx(0x1DE6, (u8)id, pos);
}

void fn_801E80FC(LbMenuWork* self)
{
    _mh_ivec2_ anchor;
    _mh_ivec2_ pos;
    char text[16];
    s16 bound;
    s32 step;
    s32 mode;
    s16 i;
    const u16* idp;
    const u16* idp2;

    get_lsp_data(0x1DB1U, &anchor);
    draw_sprite_ary((const u16*)lbl_805B7CFC, &anchor);
    draw_font_idx(0x1DB4U, (s8*)LbStr(0, 0x132U), 0, &anchor);
    bound = self->slots_0x44[self->list_mode_0x0C];
    if (bound < self->bound_0x4C) {
        step = 3;
        mode = 0;
        draw_sprite_ary((const u16*)lbl_80791B70, &anchor);
        draw_font_idx(0x1DB7U, (s8*)LbStr(0, 0x130U), 4, &anchor);
    } else {
        step = 5;
        mode = 1;
        draw_sprite_ary((const u16*)lbl_80791B78, &anchor);
        draw_font_idx(0x1DB8U, (s8*)LbStr(0, 0x131U), 4, &anchor);
    }
    sprintf(text, (char*)lbl_80791B88, self->value_0x100);
    draw_font_idx(0x1DB9U, (s8*)text, 2, &anchor);
    draw_font_idx(0x1DBAU, (s8*)LbStr(1, 0x2CU), 0, &anchor);
    for (i = 0, idp = lbl_80791B80, idp2 = lbl_805B7D0C; i < step; i++) {
        if (mode == 0) {
            get_lsp_data(*idp, &pos);
            pos.x += anchor.x;
            pos.y += anchor.y;
            draw_sprite_ary((const u16*)lbl_805B7D4C, &pos);
            if (self->flags_0x50[bound + i] != 0) {
                fn_801E7EA8(self->flags_0x50[bound + i], &pos);
            }
        } else {
            get_lsp_data(*idp2, &pos);
            pos.x += anchor.x;
            pos.y += anchor.y;
            draw_sprite_ary((const u16*)lbl_805B7D5C, &pos);
            if (self->flags_0x50[bound + i] != 0) {
                fn_801E7FEC(self->flags_0x50[bound + i], &pos);
            }
        }
        idp++;
        idp2++;
    }
    lb_page_arrow_draw(0x1D93, self->list_mode_0x0C, self->stage_0x0E, self->item_id_0x12,
                (u32)((self->mode_0x01 - 1) << __cntlzw((self->mode_0x01 - 1) ^ 1)) >> 0x1FU);
}
void fn_801E8348(LbMenuWork* self)
{
    _mh_ivec2_ pos;
    u16 a = 0xFFFF;
    u16 b = 0xFFFF;
    u8 mode;

    get_lsp_data(0x1D99U, &pos);
    draw_sprite_ary((const u16*)lbl_805B7D70, &pos);
    fn_801E80FC(self);
    mode = self->mode_0x01;
    switch ((s32)mode) {
    case 0:
        a = 7;
        if ((s32)(self->bound_0x4C + self->bound_0x4E) == 0) {
            b = 8;
        }
        break;
    case 1:
        a = 9;
        lb_panel_yes_no_draw(0x1879, self->data_0x1C);
        break;
    case 2:
        a = 0xA;
        break;
    }
    if (a != 0xFFFFU) {
        lb_panel_msg_draw(0x1877, (s16)a);
        if (b != 0xFFFFU) {
            lb_panel_str_print(0x1877, (s16)b, 2);
        }
    }
}

void fn_801E843C(LbMenuWork* self)
{
    LbSprData spr;
    LbHuntDay day;
    _mh_ivec2_ anchor;
    _mh_ivec2_ pos;
    LbUserDataView* user;
    const LbHuntMonsterRow* rows;
    const u8* monster;
    const u16* idp;
    s32 type;
    s32 t;
    u8 variant;
    u8 kind;
    s32 i;
    u16 shown;
    u8 m;

    user = LB_USER_DATA;
    if (chk_pointer() == 0) {
        get_lsp_data(0x1D8BU, &anchor);
        draw_sprite_ary((const u16*)lbl_80791B90, &anchor);
        draw_sprite_idx(-(get_option_cfg(7) == 0) + 0x1D8D, &anchor);
        draw_sprite_idx(-(self->open_0x05 == 0) + 0x1D8F, &anchor);
    }
    get_lsp_data(0x1D3EU, &anchor);
    draw_sprite_anim_ary((const u16*)lbl_805B7DA0, (u16)self->timer_0x06, &anchor);
    get_lsp_data(0x1D60U, &anchor);
    draw_sprite_ary((const u16*)lbl_805B7DC4, &anchor);
    draw_font_idx(0x1D7DU, (s8*)LbStr(0, 0x11BU), 0, &anchor);
    draw_font_idx(0x1D7EU, (s8*)LbStr(0, 0x11CU), 0, &anchor);
    if (self->list_mode_0x0C - 1U > 1U) {
        type = (getItemListSelection() - 1 == 0) + 0x1D6F;
        memcpy(day.monster_0x0, user->hunt_today_0x4068.monster_0x0, 2);
        variant = user->hunt_today_0x4068.variant_0x2;
        kind = user->hunt_today_0x4068.flag_0x4;
    } else {
        type = 0x1D71;
        fn_801E88C4((LbStrBlock*)&day, (const LbStrBlock*)&LB_USER_DATA->hunt_days_0x4850[self->list_mode_0x0C - 1]);
        variant = day.variant_0x4;
        kind = day.flag_0x6;
    }
    draw_sprite_idx(type, &anchor);
    draw_font_idx(0x1D83U, (s8*)LbStr(0, 0x118U), (s16)((s16)self->list_mode_0x0C == 0 ? 5 : 1), &anchor);
    draw_font_idx(0x1D84U, (s8*)LbStr(0, 0x119U), (s16)((s16)self->list_mode_0x0C == 1 ? 5 : 1), &anchor);
    draw_font_idx(0x1D85U, (s8*)LbStr(0, 0x11AU), (s16)((s16)self->list_mode_0x0C == 2 ? 5 : 1), &anchor);
    draw_font_idx(0x1D7FU, (s8*)LbStr(0, kind + 0x128), 0, &anchor);
    t = variant * 3;
    draw_font_idx(0x1D82U, (s8*)LbStr(0, lbl_805B7E18[t] + 0x11D), 0, &anchor);
    draw_font_idx(0x1D81U, (s8*)LbStr(0, lbl_805B7E18[t + 1] + 0x11F), 0, &anchor);
    draw_font_idx(0x1D80U, (s8*)LbStr(0, lbl_805B7E18[t + 2] + 0x122), 0, &anchor);
    shown = 0;
    monster = day.monster_0x0;
    idp = lbl_80791BA8;
    rows = (const LbHuntMonsterRow*)lbl_805B7A88;
    for (i = 0; i < 2; i++) {
        m = *monster;
        if (rows[m].kind_0x0 != 0) {
            get_lsp_data(*idp, &pos);
            pos.x += anchor.x;
            pos.y += anchor.y;
            draw_sprite_ary((const u16*)lbl_805B7E0C, &pos);
            draw_monstericon_idx(0x1D59, rows[m].kind_0x0, &pos);
            draw_font_idx(0x1D5FU, (s8*)str_tbl_33_get(rows[m].kind_0x0), 0, &pos);
            shown++;
        }
        monster++;
        idp++;
    }
    if (shown == 0) {
        spr_data_copy((s16*)&spr, get_lsp_data(0x1D7DU, NULL));
        spr.pos_0x00.y += 0x32;
        draw_font((const _SPR_DATA_&)spr, (s8*)LbStr(0, 0x13CU), 0, &anchor);
    }
    idp = lbl_80791B98;
    for (i = 0; i < 3; i++) {
        get_lsp_data(*idp, &pos);
        pos.x += anchor.x;
        pos.y += anchor.y;
        draw_sprite_ary((const u16*)lbl_80791BA0, &pos);
        idp++;
    }
    get_lsp_data(lbl_80791B98[self->list_mode_0x0C], &pos);
    pos.x += anchor.x;
    pos.y += anchor.y;
    put_menu_cursor((u16*)lbl_805B7E00, 0, &pos);
}

void fn_801E88C4(LbStrBlock* dst, const LbStrBlock* src)
{
    dst->code_0x00 = src->code_0x00;
    dst->code_0x02 = src->code_0x02;
    dst->code_0x04 = src->code_0x04;
    dst->code_0x06 = src->code_0x06;
}

void fn_801E88E8(u8 mode)
{
    _mh_ivec2_ pos;

    if (ck_WideMode() != 0) {
        get_lsp_data(0x1D27U, &pos);
        draw_sprite_ary((const u16*)lbl_805B7E5C, &pos);
    } else {
        get_lsp_data(0x1D22U, &pos);
        draw_sprite_ary((const u16*)lbl_805B7E50, &pos);
    }
    if (mode == 0) {
        lb_points_draw(0x1D11);
    } else {
        menu_money_draw(0x14FC);
    }
    get_lsp_data(0x1D12U, &pos);
    draw_sprite_ary((const u16*)lbl_805B7E30, &pos);
}

void lb_npc_choice_draw(LbList* list)
{
    lb_choice_draw(list, 0x1D2D, lbl_805B7E68, lbl_805B7E80, 0x1877, 0);
}

void fn_801E89B4(LbMenuWork* self)
{
    _mh_ivec2_ pos;
    _mh_ivec2_ p0;
    _mh_ivec2_ p1;
    _mh_ivec2_ p2;
    _mh_ivec2_ m;
    _mh_ivec2_ a;
    s32 i;
    s32 found;
    s32 off;

    get_lsp_data(0x1E16U, &pos);
    draw_sprite_ary((const u16*)lb_ship_box_frames, &pos);
    found = 0;
    spr_data_copy((s16*)&p0, (s16*)get_lsp_data(0x1307U, NULL));
    uv_pair_copy(&a, &p0);
    off = 0;
    for (i = 0; i < 0x1E; i++) {
        lb_ship_cell_pos_get(i, (s16*)&m);
        spr_data_copy((s16*)&p2, (s16*)get_lsp_data(0x12F9U, NULL));
        uv_pair_copy(&p2, &m);
        p0.x = m.x + a.x;
        p0.y = m.y + a.y;
        draw_sprite((const _SPR_DATA_&)p2, &pos);
        if (self->stage_0x0E == i) {
            found = 1;
            uv_pair_copy(&p1, &m);
        }
        if ((u8)(self->state_0x00 + 0xFF) <= 1U && chk_pointer() == 0 && self->count_0x10 == i) {
            spr_data_copy((s16*)&p2, (s16*)get_lsp_data(0x1304U, NULL));
            p2.x += m.x;
            p2.y += m.y;
            set_blendmode(4, 1, 1);
            spr_anim_draw((s16*)&p2, self->selected_0x08, (s16*)&pos);
            set_blendmode(4, 5, 1);
        }
        if (*(s32*)(self->data_0x1C + off) != 0) {
            draw_itemicon_item_id((const _SPR_DATA_&)p0, self->item_id_0x12, &pos);
        }
        off += 4;
    }
    if (self->list_mode_0x0C != 0 && found != 0) {
        set_blendmode(4, 1, 1);
        spr_data_copy((s16*)&p2, (s16*)get_lsp_data(0x1305U, NULL));
        p2.x += p1.x;
        p2.y += p1.y;
        spr_anim_draw((s16*)&p2, (u16)self->stage_0x0E, (s16*)&pos);
        set_blendmode(4, 5, 1);
        spr_data_copy((s16*)&p2, (s16*)get_lsp_data(0x1306U, NULL));
        p2.x += p1.x;
        p2.y += p1.y;
        spr_anim_draw((s16*)&p2, (u16)self->stage_0x0E, (s16*)&pos);
    }
}

void fn_801E8C44(void)
{
    _mh_ivec2_ pos;

    get_lsp_data(0x1DFBU, &pos);
    draw_sprite_ary((const u16*)lbl_805B7E8C, &pos);
    fn_801E89B4((LbMenuWork*)eft052_item_box);
    lb_item_box_count_draw(eft052_item_box, 0x1E17, 0U);
}

void fn_801E8CA0(void)
{
    fn_801E8C44();
    eft052_box_list_draw(0x1E26, lbl_805B7EC8, lbl_80791BAC, 0x1DFA, 0);
}

void fn_801E8CDC(u16 id, s8* str)
{
    _mh_ivec2_ pos;

    get_lsp_data(0x1E36U, &pos);
    draw_sprite_ary((const u16*)lbl_805B7F00, &pos);
    draw_font_idx(0x1E37U, (s8*)str_tbl_33_get((u8)id), 1, &pos);
    draw_font_idx(0x1E38U, str, 0, &pos);
}

void fn_801E8D5C(LbMenuWork* self)
{
    LbSprData spr;
    char text[0x20];
    _mh_ivec2_ anchor;
    _mh_ivec2_ pos;
    u16 key[2];
    const LbTableRow* row;
    const u16* idp;
    s32 i;
    s32 color;
    u16 id;
    bool selected;

    get_lsp_data(0x1E4BU, &anchor);
    draw_sprite_ary((const u16*)lbl_805B7EE0, &anchor);
    draw_sprite_ary((const u16*)lbl_80791BB8, &anchor);
    lb_page_arrow_draw(0x1E6D, self->list_mode_0x0C, self->stage_0x0E, self->item_id_0x12, 1);
    if ((u32)self->list_mode_0x0C <= 2U) {
        row = &((const LbTableRow*)lb_trade_price_rows)[self->list_mode_0x0C * 6];
        draw_font_idx(0x1E59U, (s8*)LbStr(0, 0x139U), 1, &anchor);
        for (i = 0, idp = lbl_805B7EF4; i < self->count_0x10; i++) {
            id = row->id_0x00;
            if (self->selected_0x08 == i) {
                key[0] = id;
                key[1] = 0;
                menu_hold_row_draw_by_lsp(0x1E35, key);
                selected = 1;
            } else {
                selected = 0;
            }
            get_lsp_data(*idp, &pos);
            pos.x += anchor.x;
            pos.y += anchor.y;
            lb_item_cell_draw_layout1(id, row->value_0x02, (s16*)&pos, 1, selected, 1, 0x10);
            idp++;
            row++;
        }
        return;
    }
    if (self->entries_0xD0[0].icon_kind_0x01 != 0) {
        draw_font_idx(0x1E59U, (s8*)LbStr(0, 0x13BU), 1, &anchor);
    } else {
        draw_font_idx(0x1E59U, (s8*)LbStr(0, 0x13AU), 1, &anchor);
    }
    for (i = 0, idp = lbl_805B7EF4; i < self->count_0x10; i++) {
        if (self->selected_0x08 == i) {
            fn_801E8CDC(self->entries_0xD0[i].kind_0x00, (s8*)self->entries_0xD0[i].value_0x04);
            selected = 1;
        } else {
            selected = 0;
        }
        get_lsp_data(*idp, &pos);
        pos.x += anchor.x;
        pos.y += anchor.y;
        color = GetMenuFontColor(1, selected, 1, 0);
        lb_list_row_draw((s8*)str_tbl_33_get(self->entries_0xD0[i].kind_0x00), selected, &pos, color, 0xC);
        sprintf(text, (char*)lbl_80791BC0, self->entries_0xD0[i].id_0x02, LbStr(1, 0x2CU));
        spr_data_copy((s16*)&spr, get_lsp_data(0x1E62U, NULL));
        spr.color_0x1C = color;
        draw_font((const _SPR_DATA_&)spr, (s8*)text, 2, &pos);
        idp++;
    }
}
u16 fn_801E936C(u16 a, u16 b)
{
    u32 v = a + b;

    if (v > 0x63U) {
        v = 0x63;
    }
    return v;
}

s32 fn_801E9600(u8* list, u8 count, u8 id)
{
    u8 i;

    for (i = 0; i < count; i++) {
        if (id == list[i]) {
            return 1;
        }
    }
    return 0;
}

void fn_801EC804(s16* value)
{
    if (++*value >= 0x28) {
        *value = 0;
    }
}

s32 lb_screen_work_busy_ck(void)
{
    return lobby_w.menu_0xAC->active_0x04 != 0;
}

void fn_801EC7AC(void)
{
    if (game_ready_ck() == 1U) {
        lobby_w.param_0x12D = 1;
    }
}

void fn_801EC828(u8* self)
{
    self[3] = 1;
    camera_lobby_manual_set(1);
    sysSE_req(1);
}

void fn_801EC194(void)
{
    eft052_hold_draw(0);
}

void fn_801E9FC8(void)
{
    fn_801E9AA8(1U);
    lb_panel_close();
}

void fn_801EB524(void)
{
    lb_farm_box_draw();
    eft052_box_list_draw(0x1718, lbl_805B83F4, lbl_80791BE8, 0x16D6, 0);
}

void fn_801EAC30(LbParam* dst, const LbParam* src)
{
    dst->kind_0x00 = src->kind_0x00;
    dst->type_0x01 = src->type_0x01;
    dst->sub_0x02 = src->sub_0x02;
    dst->flag_0x03 = src->flag_0x03;
    dst->level_0x04 = src->level_0x04;
    dst->count_0x05 = src->count_0x05;
    dst->id_0x06 = src->id_0x06;
    dst->value_0x08 = src->value_0x08;
    dst->rate_0x0A = src->rate_0x0A;
    dst->mode_0x0B = src->mode_0x0B;
}

void fn_801EB0B8(void)
{
    _mh_ivec2_ pos;

    if (ck_WideMode() != 0) {
        get_lsp_data(0x16BEU, &pos);
        draw_sprite_ary((const u16*)lbl_805B8354, &pos);
    } else {
        get_lsp_data(0x16B9U, &pos);
        draw_sprite_ary((const u16*)lbl_805B8348, &pos);
    }
    get_lsp_data(0x16A8U, &pos);
    draw_sprite_ary((const u16*)lbl_805B8360, &pos);
}

void fn_801EB6F4(void)
{
    _mh_ivec2_ pos;

    get_lsp_data(0x171BU, &pos);
    draw_sprite_ary((const u16*)lbl_805B84B0, &pos);
    fn_801EB744(0x1749U, &pos);
    fn_801EB744(0x174AU, &pos);
}

void fn_801EB744(u16 id, _mh_ivec2_* off)
{
    _mh_ivec2_ pos;

    get_lsp_data(id, &pos);
    pos.x += off->x;
    pos.y += off->y;
    draw_sprite_ary((const u16*)lbl_80791BF0, &pos);
}

void fn_801EB138(s32 idx, _mh_ivec2_* out)
{
    _mh_ivec2_ pos;

    get_lsp_data(lbl_805B8384[idx % 10], &pos);
    out->x = pos.x + 4;
    out->y = pos.y + 3;
}

void fn_801E9318(LbPartRec* dst, const LbPartRec* src)
{
    dst->kind_0x00 = src->kind_0x00;
    dst->flag_0x01 = src->flag_0x01;
    dst->sub_0x02 = src->sub_0x02;
    dst->mode_0x03 = src->mode_0x03;
    dst->level_0x04 = src->level_0x04;
    dst->count_0x05 = src->count_0x05;
    dst->id_0x06 = src->id_0x06;
    dst->value_0x08 = src->value_0x08;
    dst->rate_0x0A = src->rate_0x0A;
    dst->data_0x0C = src->data_0x0C;
}

void fn_801E92B0(void)
{
    LbPartRec* dst;
    const LbPartRec* src;
    s32 i;

    dst = (LbPartRec*)(lobby_world_block + 0x3F98);
    for (i = 0, src = (const LbPartRec*)lbl_805B7F28; i < 0xD; i++) {
        fn_801E9318(dst, src);
        src++;
        dst++;
    }
}

void fn_801E9AA8(u8 arg0)
{
    LbUserDataView* user = LB_USER_DATA;
    s32 i;

    for (i = 0; i < 3; i++) {
        if (user->farm_plots_0x3F38[i].state_0x0 != 2) {
            lobby_w.slots_0x07D[i] = 0;
        } else if (lobby_w.slots_0x07D[i] == 0 && arg0 != 0) {
            lobby_w.slots_0x07D[i] = 1;
        }
    }
}

/* 0x801E9010 (0x1B4): draws the delivery screen for its step. */
void lb_deli_screen_draw(void)
{
    LbMenuWork* self;
    u16 msg;
    s16 sel;
    u16 row;
    s32 count;

    self = lobby_w.menu_0xAC;
    set_zmode(0, 0, 0);
    set_blendmode(4, 5, 1);
    switch (self->state_0x00) {
    case 0:
        return;
    case 5:
        fn_801E88E8(1);
        break;
    default:
        fn_801E88E8(0);
        break;
    }
    switch (self->state_0x00) {
    case 1:
        lb_npc_choice_draw(&self->list_0x24);
        sel = self->list_0x24.cursor_0x00;
        msg = sel;
        if ((self->list_0x24.off_mask_0x02 & (1 << sel)) != 0 && sel == 0) {
            msg = 5;
        }
        lb_panel_msg_draw(0x1877, msg);
        break;
    case 2:
        fn_801E843C(self);
        lb_panel_msg_draw(0x1877, 6);
        break;
    case 3:
        fn_801E8348(self);
        break;
    case 4:
        switch (self->mode_0x01) {
        case 0:
            eft052_hold_draw(0);
            break;
        case 1:
            eft052_hold_draw(1);
            eft052_hold_row_get(&row, &count);
            lb_help_num_put(0x1877, 0xD, item_price_get(row) * count);
            lb_panel_yes_no_draw(0x1879, self->yes_no_0x18);
            break;
        }
        break;
    case 5:
        fn_801E8CA0();
        break;
    case 6:
        lb_deli_menu_draw();
        break;
    case 7:
        fn_801E8D5C(self);
        lb_panel_msg_draw(0x1877, 0xE);
        break;
    }
}

/* 0x801E91C4 (0xEC): adds the free-hunt result's hunt and capture counts to the user data and stores its rewards
 * in the item box, clearing the result. */
void lb_fq_result_items_take(void)
{
    LbFqResultView* result;
    IdValue* reward;
    s32 i;

    result = (LbFqResultView*)get_FqResult_work();
    for (i = 0; i < 0x29; i++) {
        LB_USER_DATA->hunted_0x48D8[i] = fn_801E936C(LB_USER_DATA->hunted_0x48D8[i], result->hunted_0x000[i]);
        LB_USER_DATA->captured_0x49D8[i] = fn_801E936C(LB_USER_DATA->captured_0x49D8[i], result->captured_0x052[i]);
        result->hunted_0x000[i] = 0;
        result->captured_0x052[i] = 0;
    }
    reward = result->rewards_0x0F8;
    for (i = 0; i < 30; i++) {
        if (reward->id != 0) {
            item_take(reward->value, LB_USER_DATA->item_box_0x4860, 30, 0, 99);
            reward->id = 0;
            reward->value = 0;
        }
        reward++;
    }
}

/* 0x801E938C (0x274): draws one day of the free-hunt schedule: its monsters, its area variant and its flag. */
void lb_hunt_day_roll(u8* monsters, u16* variant, u16* flag)
{
    u8 pool[14];
    const LbHuntMonsterRow* row;
    u16 top;
    u16 v;
    u8 count;
    u8 n;
    u8 idx;
    u8 i;

    row = (const LbHuntMonsterRow*)lbl_805B7A88;
    memset(monsters, 0, 2);
    memset(pool, 0, 14);
    v = (u16)ran_suu(1) % 4;
    if (lb_unlock_cond_ck(9) == 1 && (u16)ran_suu(1) % 100 > 65) {
        v += 4;
    }
    *variant = v;
    *flag = (u16)ran_suu(1) % 100 < 80;
    if (lb_unlock_cond_ck(10) == 0) {
        count = 0;
    } else {
        count = (ran_suu(1) & 1) + 1;
    }
    top = lb_unlock_cond_top_get(0);
    n = 0;
    idx = 0;
    while (row->kind_0x0 != 0xFF) {
        if (lb_unlock_cond_ck(row->level_0x2) != 0 && row->level_0x2 != 0xFFFF) {
            pool[n] = idx;
            n++;
        }
        idx++;
        row++;
    }
    for (i = 0; i < count; i++) {
        monsters[i] = lb_rand_pick_take(pool);
        if ((u16)ran_suu(1) % 100 < 80) {
            u8 first;
            u8 j;

            n = 0;
            j = 0;
            first = monsters[i];
            memset(pool, 0, 14);
            row = (const LbHuntMonsterRow*)lbl_805B7A88;
            while (row->kind_0x0 != 0xFF) {
                if (row->level_0x2 <= top && row->level_0x2 != 0xFFFF && first != j) {
                    pool[n] = j;
                    n++;
                }
                j++;
                row++;
                if (first + j >= 15) {
                    break;
                }
            }
        }
    }
}

#pragma opt_unroll_loops off
/* 0x801E9640 (0x248): rebuilds the free hunt's point rows from the monster tables and draws its four-monster
 * rotation, leaving out the monsters of `today`. */
void lb_daily_menu_roll(u8* today)
{
    u8 picks[4];
    u8 pool[14];
    const LbHuntMonsterRow* monster;
    const u16* row;
    EmSetPointRow* point;
    s32 i;
    u8 n;
    u8 count;
    u8 top;
    u8 idx;
    u8* pick;

    monster = (const LbHuntMonsterRow*)lbl_805B7A88;
    for (i = 0; i < 17; i++) {
        lb_param_w.points_0x32[i].kind_0x0 = 0;
        lb_param_w.points_0x32[i].kill_points_0x2 = 0;
        lb_param_w.points_0x32[i].capture_points_0x4 = 0;
    }
    row = lb_trade_flag_rows;
    n = 0;
    while (row[0] != 0) {
        point = &lb_param_w.points_0x32[n];
        point->kind_0x0 = row[0];
        point->kill_points_0x2 = row[1];
        point->capture_points_0x4 = 0;
        n++;
        row += 3;
    }
    row = lb_trade_pair_rows;
    while (row[0] != 0) {
        point = &lb_param_w.points_0x32[n];
        point->kind_0x0 = row[0];
        point->kill_points_0x2 = row[1];
        point->capture_points_0x4 = row[2];
        n++;
        row += 4;
    }
    memset(picks, 0, 4);
    memset(pool, 0, 14);
    if (lb_unlock_cond_ck(10) == 0) {
        count = 0;
    } else {
        count = (u16)ran_suu(1) % 5;
    }
    top = lb_unlock_cond_top_get(0);
    n = 0;
    idx = 0;
    while (monster->kind_0x0 != 0xFF) {
        if (monster->level_0x2 != 0xFFFF && monster->level_0x2 <= top && fn_801E9600(today, 2, idx) == 0) {
            pool[n] = idx;
            n++;
        }
        idx++;
        monster++;
    }
    pick = picks;
    for (i = 0; i < count; i++) {
        *pick = lb_rand_pick_take(pool);
        pick++;
    }
    lb_param_w.rotation_0x1E[0][0] = picks[0];
    lb_param_w.rotation_0x1E[0][1] = 2;
    lb_param_w.rotation_0x1E[1][0] = picks[1];
    lb_param_w.rotation_0x1E[1][1] = 2;
    lb_param_w.rotation_0x1E[2][0] = picks[2];
    lb_param_w.rotation_0x1E[2][1] = 2;
    lb_param_w.rotation_0x1E[3][0] = picks[3];
    lb_param_w.rotation_0x1E[3][1] = 2;
}
#pragma opt_unroll_loops reset

/* 0x801E9888 (0x84): moves the free-hunt schedule on by one day and draws the new last day. */
void lb_hunt_day_advance(void)
{
    LbUserDataView* user;

    user = LB_USER_DATA;
    memcpy(user->hunt_today_0x4068.monster_0x0, LB_USER_DATA->hunt_days_0x4850[0].monster_0x0, 2);
    user->hunt_today_0x4068.variant_0x2 = LB_USER_DATA->hunt_days_0x4850[0].variant_0x4;
    user->hunt_today_0x4068.flag_0x4 = LB_USER_DATA->hunt_days_0x4850[0].flag_0x6;
    memcpy(&LB_USER_DATA->hunt_days_0x4850[0], &LB_USER_DATA->hunt_days_0x4850[1], 8);
    lb_hunt_day_roll(LB_USER_DATA->hunt_days_0x4850[1].monster_0x0, &LB_USER_DATA->hunt_days_0x4850[1].variant_0x4,
                     &LB_USER_DATA->hunt_days_0x4850[1].flag_0x6);
    lb_daily_menu_roll(LB_USER_DATA->hunt_today_0x4068.monster_0x0);
}

/* 0x801E990C (0x184): takes a random entry out of the zero-terminated 14-byte list `pool` and returns it (0 for an
 * empty list). */
u8 lb_rand_pick_take(u8* pool)
{
    u8* p;
    s32 n;
    s32 i;
    u8 value;

    n = 0;
    for (p = pool; *p != 0; p++) {
        n++;
    }
    if (n != 0) {
        u8 k = (u16)ran_suu(1) % n;

        value = pool[k];
        for (i = k; i < 13; i++) {
            pool[i] = pool[i + 1];
        }
    } else {
        value = 0;
    }
    return value;
}

/* 0x801E9A90 (0x18): sets plot `plot`'s mark in the lobby work. */
void lb_farm_plot_mark_set(u8 plot, u8 mark)
{
    lobby_w.slots_0x07D[plot] = mark;
}

/* 0x801E9B68 (0xC): returns the number of farm plots open. */
u8 lb_world_submode_get(void)
{
    return LB_USER_DATA->farm_plot_count_0x3E02;
}

/* 0x801E9B74 (0xAC): whether farm plot `slot` is open: the farm is unlocked, the plot is not growing and its own
 * progress flag is set. */
u32 lb_part_slot_open_ck(u8 slot)
{
    if (userdata_progress_flag_ck(0x1A) == 0) {
        return 0;
    }
    if (LB_USER_DATA->farm_plots_0x3F38[slot].state_0x0 == 2) {
        return 0;
    }
    switch (slot) {
    case 1:
        if (userdata_progress_flag_ck(0x21) == 0) {
            return 0;
        }
        break;
    case 2:
        if (userdata_progress_flag_ck(0x26) == 0) {
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x801E9C20 (0x38): whether farm plot `id` is empty with soil grade 7. */
u32 userdata_quest_slot_open_ck(u8 id)
{
    u32 open = 1;
    LbFarmPlot* plot = &LB_USER_DATA->farm_plots_0x3F38[id];

    if (plot->grade_0x1 != 7 || plot->state_0x0 != 0) {
        open = 0;
    }
    return open;
}

/* 0x801E9C58 (0x1A4): passes one day on the farm: growing plots count down and ripen, and every plot draws a new
 * soil grade and bonus seed. */
void lb_farm_day_pass(void)
{
    u8 pool[8];
    const LbFarmGradeRow* grade;
    LbUserDataView* user;
    LbFarmPlot* plot;
    s32 i;
    s16 k;
    s32 j;
    u8 count;
    u8 n;

    grade = lb_farm_grade_rows;
    user = LB_USER_DATA;
    count = lb_world_submode_get();
    plot = user->farm_plots_0x3F38;
    for (i = 0; i < count; i++) {
        if (plot->days_0x4 != 0) {
            plot->days_0x4--;
            if (plot->days_0x4 == 0) {
                plot->state_0x0 = 1;
                plot->days_0x4 = 0;
            }
        }
        plot++;
    }
    plot = user->farm_plots_0x3F38;
    if (getItemListSelection() == 1) {
        if (count == 3) {
            grade++;
        }
        for (k = 0; k < count; k++) {
            plot->grade_0x1 = weight_table_index(grade->weights_0x4, grade->count_0x0, (u16)ran_suu(1) % 100);
            n = 0;
            for (j = 0; j < 8; j++) {
                pool[j] = 0;
                if (lb_farm_seed_ck(plot, j) != -1) {
                    pool[n] = j;
                    n++;
                }
            }
            if (n != 0) {
                plot->bonus_0x5 = pool[(u16)ran_suu(1) % n];
            } else {
                plot->bonus_0x5 = 0;
            }
            plot++;
        }
    }
}

/* 0x801E9DFC (0x28): returns the tier of seed `id`: 0 for the first four, 1 for the next four, else 2. */
u8 lb_goods_tier_get(u8 id)
{
    if (id < 4) {
        return 0;
    }
    if (id < 8) {
        return 1;
    }
    return 2;
}

/* 0x801E9E24 (0x2C): returns the price of seed `seed` (0 for none). */
u16 lb_farm_seed_price_get(u8 seed)
{
    if (seed != 0xFF) {
        return lb_farm_seed_rows[seed].price_0x2;
    }
    return 0;
}

/* 0x801E9E50 (0xE8): whether `plot` can take seed `seed`: -1 when it cannot, 2 when the money does not reach its
 * price, else 0. */
s8 lb_farm_seed_ck(const LbFarmPlot* plot, u8 seed)
{
    s32 result = 0;
    const LbFarmSeedRow* row = &lb_farm_seed_rows[seed];
    u8 level;

    if (LB_USER_DATA->zenny_0x3F04 < row->price_0x2) {
        result = 2;
    }
    level = plot->level_0x2;
    if (plot->grade_0x1 == 3) {
        level++;
    }
    if (level < row->level_0x0) {
        result = -1;
    } else if (seed >= 8 && seed < 12) {
        switch (row->level_0x0) {
        case 1:
            if (lb_unlock_cond_ck(0x10) == 0) {
                result = -1;
            }
            break;
        case 2:
            if (lb_unlock_cond_ck(0x17) == 0) {
                result = -1;
            }
            break;
        case 3:
            result = -1;
            break;
        }
    }
    return result;
}

/* 0x801E9F38 (0x90): opens the farm screen (lobby screen 5) with its plots. */
void lb_screen05_open(struct _PLW* player, struct _LB_NPC* npc)
{
    LbFarmWork* self;
    u8 count;

    self = (LbFarmWork*)lobby_w.menu_0xAC;
    count = lb_world_submode_get();
    memset(lobby_w.menu_0xAC, 0, 0x2000);
    self->player_0x40 = player;
    self->npc_0x44 = npc;
    self->plot_count_0x0A = count;
    lobby_w.state_0x000 = 5;
    lobby_w.active_0x008 = 1;
    if (self->plot_count_0x0A == 0) {
        self->plot_count_0x0A = 1;
    }
    lb_talk_page_open(0);
}

/* 0x801E9FF0 (0xC0): one input step of the plot picker: 2 on cancel, else 0. */
s32 lb_farm_plot_select_step(LbFarmWork* self)
{
    s32 result = 0;

    if (lb_cmd_pressed_ck(0x20) != 0) {
        result = 2;
        sysSE_req(1);
    } else if (lb_cmd_repeat_ck(0xC) != 0) {
        self->plot_0x08 = menu_cursor_step_open_last(self->plot_0x08, self->plot_count_0x0A, lb_cmd_repeat_get(), 4, 8, 6);
        if (LB_USER_DATA->farm_plots_0x3F38[self->plot_0x08].state_0x0 != 0) {
            self->plot_look_0x16 = 1;
        } else {
            self->plot_look_0x16 = 0;
        }
    }
    return result;
}

/* 0x801EA0B0 (0x128): returns how many harvest rows ripe plot `plot` gives (4, more for its planted grade's luck). */
s16 lb_farm_harvest_rolls_get(const LbFarmPlot* plot)
{
    s16 rolls = 4;

    switch (plot->grade_planted_0xA) {
    case 0:
        if (plot->seed_0x3 == plot->bonus_planted_0xB) {
            rolls = 5;
        }
        break;
    case 1:
        if (plot->seed_0x3 == plot->bonus_planted_0xB) {
            if ((ran_suu(1) & 1) != 0) {
                rolls = 6;
            } else {
                rolls = 3;
            }
        }
        break;
    case 2:
        if (plot->seed_0x3 == plot->bonus_planted_0xB && (u16)ran_suu(1) % 100 < 20) {
            rolls = 5;
        }
        break;
    case 5:
        if ((u16)ran_suu(1) % 100 < 20) {
            rolls = 5;
        }
        break;
    }
    return rolls;
}

/* 0x801EA1D8 (0xDC): rolls `rolls` harvest rows (at most `room`) from `table` into `out` with the generator `rng`. */
void lb_farm_harvest_roll(IdValue* out, const LbFarmHarvestRow* table, s16 rolls, s16 room, u16 rng)
{
    const LbFarmHarvestRow* row;
    u32 sum;
    s16 i;

    if (room > 0) {
        if (rolls > room) {
            rolls = room;
        }
        IdValue* p;

        for (i = 0, p = out; i < rolls; i++) {
            u32 roll;

            rng = rand_lcg_step(rng);
            roll = rng % 100;
            sum = 0;
            for (row = table; row->item_0x0 != 0; row++) {
                sum += row->rate_0x4;
                if (roll < sum) {
                    item_pair_copy(p, row);
                    break;
                }
            }
            p++;
        }
    }
}

/* 0x801EA2B4 (0x210): harvests every ripe plot into the work's harvest rows (a fertilizer adds a draw from a seed
 * of its tier) and opens the item-get window on them. */
void lb_farm_harvest(LbFarmWork* self)
{
    LbUserDataView* user;
    LbFarmPlot* plot;
    IdValue* out;
    const LbFarmHarvestRow* table;
    const LbFarmHarvestRow* extra;
    s32 i;
    s32 base;
    s32 k;
    s32 j;
    u32 found;
    u32 tier;
    s16 rolls;
    s16 mult;
    u16 rng;
    u8 count;

    user = LB_USER_DATA;
    count = lb_world_submode_get();
    memset(self->harvest_0x54, 0, 0x78);
    plot = user->farm_plots_0x3F38;
    out = self->harvest_0x54;
    base = 0;
    for (i = 0; i < self->plot_count_0x0A; i++) {
        if (plot->state_0x0 == 1) {
            table = lb_farm_harvest_tables[plot->seed_0x3];
            rolls = lb_farm_harvest_rolls_get(plot);
            if (rolls > 10) {
                rolls = 10;
            }
            lb_farm_harvest_roll(out, table, rolls, 10, plot->rng_0x8);
            found = 0;
            switch (plot->fertilizer_0x6) {
            case 0xEA:
                tier = 0;
                k = 0;
                found = 1;
                break;
            case 0xEB:
                tier = 1;
                k = 4;
                found = 1;
                break;
            case 0xEC:
                tier = 2;
                k = 8;
                found = 1;
                break;
            }
            if (found == 1) {
                s32 s = (u8)(plot->seed_0x3 % 4);

                mult = 1;
                if (tier == lb_goods_tier_get(plot->seed_0x3)) {
                    mult = 3;
                }
                extra = NULL;
                for (; s >= 0; s--) {
                    if (lb_farm_seed_ck(plot, k + s) != -1) {
                        extra = lb_farm_harvest_tables[k + s];
                        break;
                    }
                }
                if (extra != NULL) {
                    rng = plot->rng_0x8;
                    for (j = 0; j < 10; j++) {
                        rng = rand_lcg_step(rng);
                    }
                    lb_farm_harvest_roll(&self->harvest_0x54[base + rolls], extra, mult, 10 - rolls, rng);
                }
            }
        }
        plot++;
        out += 10;
        base += 10;
    }
    ((LbItemGetView*)lb_item_get_data)->farm_0x40.count_0x02 = count;
    eft052_item_get_open(self->harvest_0x54, &((LbItemGetView*)lb_item_get_data)->farm_0x40);
}

/* 0x801EA4C4 (0xA4): plants the plot being set up: pays the seed, uses the fertilizer and starts the plot growing. */
void lb_farm_plant(LbFarmWork* self)
{
    LbFarmPlot* plot;

    plot = &LB_USER_DATA->farm_plots_0x3F38[self->plot_0x08];
    userdata_zenny_add(-lb_farm_seed_price_get(self->edit_0x48.seed_0x3));
    eft052_page_count_add(self->edit_0x48.fertilizer_0x6, 1);
    memcpy(plot, &self->edit_0x48, 12);
    plot->state_0x0 = 2;
    plot->rng_0x8 = ran_suu(1);
    plot->days_0x4 = lb_farm_plot_days_get(plot);
    plot->grade_planted_0xA = plot->grade_0x1;
    plot->bonus_planted_0xB = plot->bonus_0x5;
}

/* 0x801EA568 (0x104): lists the fertilizers the box holds and opens the fertilizer picker on them. */
void lb_farm_fertilizer_menu_open(LbFarmWork* self)
{
    LbSeParam param;
    u8 count;
    const u16* item;
    s32 i;

    count = lb_world_submode_get();
    memset(self->fertilizers_0xCC, 0, count * 0x14);
    self->fertilizer_count_0x18 = 0;
    item = lb_farm_fertilizer_items;
    for (i = 0; i < 5; i++) {
        if (userdata_item_count_total(*item, lobby_world_block) != 0) {
            self->fertilizers_0xCC[self->fertilizer_count_0x18] = *item;
            self->fertilizer_count_0x18++;
        }
        item++;
    }
    param.value_0x00 = 0;
    param.mode_0x04 = 0x87;
    param.id_0x06 = 0x8E;
    param.max_0x0A = -1;
    param.limit_0x08 = 0x88;
    param.title_0x0E = 0x13E5;
    param.time_0x10 = self->edit_0x48.fertilizer_0x6;
    param.cur_0x0C = self->fertilizer_count_0x18;
    param.data_0x14 = self->fertilizers_0xCC;
    param.flag_0x18 = 0;
    eft052_hold_entry_set(&param, 0);
}

/* 0x801EA66C (0x94): counts the seeds of the shown tier the plot being set up can take and keeps the cursor on one. */
void lb_farm_seed_count_update(LbFarmWork* self)
{
    s32 i;

    self->seed_count_0x0E = 0;
    for (i = 0; i < 4; i++) {
        if (lb_farm_seed_ck(&self->edit_0x48, i + self->seed_page_0x10 * 4) == -1) {
            break;
        }
        self->seed_count_0x0E++;
    }
    if (self->seed_cursor_0x0C >= self->seed_count_0x0E) {
        self->seed_cursor_0x0C = self->seed_count_0x0E - 1;
    }
}

/* 0x801EA700 (0x530): one input step of the farm's plot menu (pick a plot, set its seed and fertilizer, plant):
 * 2 on cancel, else 0. */
s32 lb_farm_plot_menu_step(LbFarmWork* self)
{
    s32 result = 0;
    LbFarmPlot* plot = &LB_USER_DATA->farm_plots_0x3F38[self->plot_0x08];
    u16 row;
    u8 seed;

    switch (self->step_0x01) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            if (plot->state_0x0 == 0) {
                self->step_0x01 = 1;
                fn_801EAC30((LbParam*)&self->edit_0x48, (const LbParam*)&LB_USER_DATA->farm_plots_0x3F38[self->plot_0x08]);
                self->edit_0x48.seed_0x3 = 0xFF;
                self->action_0x04 = 0;
                self->seed_menu_0x02 = 1;
                self->changed_0x03 = 0;
                sysSE_req(5);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            result = 2;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->plot_0x08 = menu_cursor_step_open_last(self->plot_0x08, self->plot_count_0x0A, lb_cmd_repeat_get(), 4, 8, 6);
            if (LB_USER_DATA->farm_plots_0x3F38[self->plot_0x08].state_0x0 != 0) {
                self->plot_look_0x16 = 1;
            } else {
                self->plot_look_0x16 = 0;
            }
        }
        break;
    case 1:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            switch (self->action_0x04) {
            case 0:
                self->step_0x01 = 2;
                if (userdata_quest_slot_open_ck(self->plot_0x08) != 0) {
                    self->seed_pages_0x12 = 3;
                } else {
                    self->seed_pages_0x12 = 2;
                }
                self->seed_page_0x10 = 0;
                self->seed_cursor_0x0C = 0;
                lb_farm_seed_count_update(self);
                sysSE_req(5);
                break;
            case 1:
                lb_farm_fertilizer_menu_open(self);
                self->step_0x01 = 3;
                sysSE_req(5);
                break;
            case 2:
                self->step_0x01 = 4;
                self->yes_no_0x1C = 0;
                self->blink_0x14 = 0;
                sysSE_req(0);
                break;
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            if (self->changed_0x03 != 0) {
                self->step_0x01 = 5;
                self->yes_no_0x1C = 0;
                sysSE_req(1);
            } else {
                self->step_0x01 = 0;
                sysSE_req(1);
            }
        } else if (lb_cmd_repeat_ck(3) != 0) {
            s16 rows = (self->seed_menu_0x02 == 0) + 2;

            self->action_0x04 = menu_cursor_step(self->action_0x04, rows, lb_cmd_repeat_get(), 1, 2);
        }
        break;
    case 2:
        seed = self->seed_cursor_0x0C + self->seed_page_0x10 * 4;
        if (lb_cmd_pressed_ck(0x10) != 0) {
            if (lb_farm_seed_ck(&self->edit_0x48, self->seed_cursor_0x0C + self->seed_page_0x10 * 4) == 0) {
                self->step_0x01 = 1;
                self->changed_0x03 = 1;
                self->seed_menu_0x02 = 0;
                self->edit_0x48.seed_0x3 = seed;
                self->edit_0x48.days_0x4 = lb_farm_seed_rows[seed].days_0x1;
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            self->step_0x01 = 1;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            self->seed_cursor_0x0C = menu_cursor_step(self->seed_cursor_0x0C, self->seed_count_0x0E, lb_cmd_repeat_get(), 1, 2);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->seed_page_0x10 = menu_cursor_step_fixed_tail(self->seed_page_0x10, self->seed_pages_0x12, lb_cmd_repeat_get(), 4, 8,
                                                               &self->page_moved_0x06);
            lb_farm_seed_count_update(self);
        }
        break;
    case 3:
        switch (eft052_hold_step()) {
        case 1:
            eft052_hold_row_get(&row, NULL);
            if (self->edit_0x48.fertilizer_0x6 == row) {
                self->edit_0x48.fertilizer_0x6 = 0;
            } else {
                self->edit_0x48.fertilizer_0x6 = row;
            }
            self->step_0x01 = 1;
            self->changed_0x03 = 1;
            break;
        case 2:
            self->step_0x01 = 1;
            break;
        }
        break;
    case 4:
        self->blink_0x14++;
        if (self->blink_0x14 >= 20) {
            self->blink_0x14 = 0;
        }
        switch (toggle_word_step(&self->yes_no_0x1C, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            lb_farm_plant(self);
            self->step_0x01 = 0;
            self->plot_look_0x16 = 2;
            sysSE_stop(0x14);
            break;
        case 2:
            self->step_0x01 = 1;
            break;
        }
        break;
    case 5:
        switch (lb_yes_no_step(&self->yes_no_0x1C)) {
        case 1:
            self->step_0x01 = 0;
            break;
        case 2:
            self->step_0x01 = 1;
            break;
        }
        break;
    }
    return result;
}

/* 0x801EAC84 (0x2A0): one step of the farm screen: the opening choice, the plot menu, the plot picker and the
 * harvest, then the screen's draw is queued. */
void lb_item_get_screen_step(void)
{
    LbFarmWork* self;
    s32 i;

    self = (LbFarmWork*)lobby_w.menu_0xAC;
    if (self->plot_look_0x16 == 2) {
        self->plot_look_0x16 = 1;
    }
    self->page_moved_0x06 = 0;
    switch (self->state_0x00) {
    case 0:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1U) {
            self->state_0x00++;
            lb_list_init(&self->choice_0x20, 3, 0, 0x13E, 0x7C, 0, -1, 3, 0);
            camera_talk_lock_set(1);
            sysSE_stop(0x29);
        }
        break;
    case 1:
        ainpc_page_hold_set();
        switch (lb_choice_step(&self->choice_0x20)) {
        case 1:
            switch (self->choice_0x20.cursor_0x00) {
            case 1:
                self->state_0x00 = 4;
                sysSE_stop(0xC);
                lb_farm_harvest(self);
                break;
            case 0:
                self->state_0x00 = 2;
                sysSE_stop(0xE);
                break;
            case 2:
                self->state_0x00 = 3;
                sysSE_req(0x1B);
                break;
            }
            self->step_0x01 = 0;
            self->action_0x04 = 0;
            self->plot_0x08 = 0;
            if (LB_USER_DATA->farm_plots_0x3F38[self->plot_0x08].state_0x0 != 0) {
                self->plot_look_0x16 = 1;
            } else {
                self->plot_look_0x16 = 0;
            }
            break;
        case 2:
            self->state_0x00 = 5;
            lb_talk_page_open(1);
            camera_talk_lock_set(0);
            break;
        }
        break;
    case 2:
        ainpc_page_hold_set();
        if ((u32)(lb_farm_plot_menu_step(self) - 1) <= 1U) {
            self->state_0x00 = 1;
        }
        break;
    case 3:
        ainpc_page_hold_set();
        if ((u32)(lb_farm_plot_select_step(self) - 1) <= 1U) {
            self->state_0x00 = 1;
        }
        break;
    case 4:
        ainpc_page_hold_set();
        if ((u32)(eft052_item_get_step() - 1) <= 1U) {
            for (i = 0; i < self->plot_count_0x0A; i++) {
                if (LB_USER_DATA->farm_plots_0x3F38[i].state_0x0 == 1) {
                    LB_USER_DATA->farm_plots_0x3F38[i].state_0x0 = 0;
                    LB_USER_DATA->farm_plots_0x3F38[i].fertilizer_0x6 = 0;
                }
            }
            self->state_0x00 = 1;
        }
        break;
    case 5:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1U) {
            fn_801E9FC8();
        }
        break;
    }
    if (self->state_0x00 != 5) {
        subTransSet((u32)lb_farm_screen_draw, 0, 0);
    }
}

/* 0x801EAF24 (0x194): puts the farm screen's help line for its step. */
void lb_farm_panel_draw(LbFarmWork* self)
{
    u16 msg = 0xFFFF;
    u16 line = 0xFFFF;
    s32 row = 1;

    switch (self->state_0x00) {
    case 2:
        switch (self->step_0x01) {
        case 0:
            msg = 0x7F;
            switch (LB_USER_DATA->farm_plots_0x3F38[self->plot_0x08].state_0x0) {
            case 1:
                line = 0x80;
                break;
            case 2:
                line = 0x81;
                break;
            }
            break;
        case 1:
            msg = self->action_0x04 + 0x82;
            break;
        case 2:
            msg = 0x85;
            if (lb_farm_seed_ck(&self->edit_0x48, self->seed_cursor_0x0C + self->seed_page_0x10 * 4) != 0) {
                line = 0x86;
                row = 2;
            }
            break;
        case 4:
            lb_help_num_put(0x1877, 0x89, lb_farm_seed_price_get(self->edit_0x48.seed_0x3));
            lb_panel_yes_no_draw(0x1879, self->yes_no_0x1C);
            return;
        case 5:
            msg = 0x8A;
            lb_panel_yes_no_draw(0x1879, self->yes_no_0x1C);
            break;
        }
        break;
    case 3:
        msg = 0x8B;
        break;
    }
    if (msg != 0xFFFF) {
        lb_panel_msg_draw(0x1877, msg);
        if (line != 0xFFFF) {
            lb_panel_line_draw(0x1877, line, 2, row);
        }
    }
}

/* 0x801EB1B8 (0x2AC): draws page `page` of the farm's item box: its ten cells, their items, the cursor and the
 * marked cell. */
void lb_farm_box_page_draw(LbItemBoxView* box, u8 page)
{
    LbSprData cell_spr;
    LbSprData icon_spr;
    LbSprData cursor_spr;
    _mh_ivec2_ cell;
    _mh_ivec2_ base;
    _mh_ivec2_ mark;
    _mh_ivec2_ anchor;
    s32 marked;
    s32 first;
    s32 i;
    s32 slot;

    get_lsp_data(lb_farm_box_page_lsp[page], &anchor);
    draw_sprite_anim_ary(lb_farm_box_page_frame, page, &anchor);
    marked = 0;
    first = page * 10;
    spr_data_copy((s16*)&icon_spr, get_lsp_data(0x1307, NULL));
    uv_pair_copy(&base, &icon_spr.pos_0x00);
    for (i = 0; i < 10; i++) {
        fn_801EB138(i, &cell);
        spr_data_copy((s16*)&cell_spr, get_lsp_data(lb_farm_box_cell_lsp[page], NULL));
        uv_pair_copy(&cell_spr.pos_0x00, &cell);
        icon_spr.pos_0x00.x = cell.x + base.x;
        icon_spr.pos_0x00.y = cell.y + base.y;
        draw_sprite((const _SPR_DATA_&)cell_spr, &anchor);
        slot = first + i;
        if (box->mark_0x0E == slot) {
            marked = 1;
            uv_pair_copy(&mark, &cell);
        }
        if ((u8)(box->state_0x00 - 1) <= 1U && chk_pointer() == 0 && box->cursor_0x10 == slot) {
            spr_data_copy((s16*)&cursor_spr, get_lsp_data(0x1304, NULL));
            cursor_spr.pos_0x00.x += cell.x;
            cursor_spr.pos_0x00.y += cell.y;
            set_blendmode(4, 1, 1);
            spr_anim_draw((s16*)&cursor_spr, box->anim_0x08, (s16*)&anchor);
            set_blendmode(4, 5, 1);
        }
        if (box->items_0x24[first + i].id != 0) {
            draw_itemicon_item_id((const _SPR_DATA_&)icon_spr, box->items_0x24[first + i].id, &anchor);
        }
    }
    if (box->marked_0x0C != 0 && marked != 0) {
        set_blendmode(4, 1, 1);
        spr_data_copy((s16*)&cursor_spr, get_lsp_data(0x1305, NULL));
        cursor_spr.pos_0x00.x += mark.x;
        cursor_spr.pos_0x00.y += mark.y;
        spr_anim_draw((s16*)&cursor_spr, box->mark_anim_0x0A, (s16*)&anchor);
        set_blendmode(4, 5, 1);
        spr_data_copy((s16*)&cursor_spr, get_lsp_data(0x1306, NULL));
        cursor_spr.pos_0x00.x += mark.x;
        cursor_spr.pos_0x00.y += mark.y;
        spr_anim_draw((s16*)&cursor_spr, box->mark_anim_0x0A, (s16*)&anchor);
    }
}

/* 0x801EB464 (0xC0): draws the farm's item box, one page per plot open. */
void lb_farm_box_draw(void)
{
    _mh_ivec2_ anchor;
    LbItemBoxView* box;
    const u16* id;
    s32 i;
    u8 last;

    box = (LbItemBoxView*)eft052_item_box;
    last = lb_world_submode_get() - 1;
    get_lsp_data(0x16D7, &anchor);
    draw_sprite_anim_ary(lb_farm_box_frame, last, &anchor);
    id = lb_farm_box_page_tabs;
    for (i = 0; i <= last; i++) {
        lb_farm_box_page_draw(box, i);
        draw_sprite_anim_idx(*id, last, &anchor);
        id++;
    }
    lb_item_box_count_draw(box, 0x170A, last);
}

/* 0x801EB560 (0x194): draws the plot tabs, in the order the picked plot sets, with the picked one's cursor and page
 * arrows. */
void lb_farm_plot_tabs_draw(LbFarmWork* self)
{
    _mh_ivec2_ anchor;
    const LbFarmTabRow* tab;
    s32 i;
    u16 frame;
    u16 blink;
    u8 plot;

    for (i = 0; i < 3; i++) {
        plot = lb_farm_tab_order[self->plot_0x08 * 3 + i];
        if (plot < self->plot_count_0x0A) {
            if (plot != self->plot_0x08) {
                frame = 6;
                blink = 0;
            } else {
                frame = 0;
                blink = (u32)lobby_w.counter_0x030 % 40;
            }
            tab = &lb_farm_tab_rows[plot];
            get_lsp_data(tab->lsp_0x0, &anchor);
            draw_sprite_anim_ary(tab->frames_0x8, frame, &anchor);
            if (chk_pointer() == 0) {
                if (plot == self->plot_0x08) {
                    draw_sprite_anim_idx(tab->cursor_0x2, blink, &anchor);
                }
                if (self->plot_count_0x0A > 1 && plot == self->plot_0x08 && (self->step_0x01 == 0 || self->state_0x00 == 3)) {
                    draw_sprite_ary(tab->arrows_0xC, &anchor);
                    if (lb_cmd_repeat_ck(4) != 0) {
                        draw_sprite_anim_idx(tab->arrows_0xC[2], 1, &anchor);
                    } else if (lb_cmd_repeat_ck(8) != 0) {
                        draw_sprite_anim_idx(tab->arrows_0xC[3], 1, &anchor);
                    }
                }
            }
            draw_sprite_anim_idx(tab->frame_0x4, frame, &anchor);
        }
    }
}

/* 0x801EB7A4 (0x68): returns the days `plot` takes to grow: its seed's days, one less on grade 4 and less with a
 * speed fertilizer, at least one. */
s8 lb_farm_plot_days_get(const LbFarmPlot* plot)
{
    s8 days = 0;

    if (plot->days_0x4 > 0) {
        days = plot->days_0x4;
        if (plot->grade_0x1 == 4) {
            days = plot->days_0x4 - 1;
        }
        switch (plot->fertilizer_0x6) {
        case 0xFB:
            days -= 1;
            break;
        case 0xED:
            days -= 2;
            break;
        }
        if (days < 1) {
            days = 1;
        }
    }
    return days;
}

/* 0x801EB80C (0x4B4): draws the picked plot's info panel: its seed, tier, days, fertilizer, grade stars and state. */
void lb_farm_plot_info_draw(LbFarmWork* self, u8 active)
{
    _mh_ivec2_ star;
    _mh_ivec2_ anchor;
    LbSprData spr;
    char text[0x40];
    LbFarmPlot* plot;
    LbSprData* frame;
    u32 font;
    u32 i;
    u8 grade;
    u8 level;
    void* str;

    plot = &LB_USER_DATA->farm_plots_0x3F38[self->plot_0x08];
    if (self->state_0x00 == 3) {
        get_lsp_data(0x17D6, &anchor);
    } else {
        get_lsp_data(0x171B, &anchor);
        frame = (LbSprData*)get_lsp_data(self->plot_0x08 == 1 ? 0x17B0 : (self->plot_0x08 == 2 ? 0x17C0 : 0x17A1), NULL);
        spr_data_copy((s16*)&spr, get_lsp_data(0x172D, NULL));
        spr.color_0x1C = frame->color_0x1C;
        draw_sprite((const _SPR_DATA_&)spr, &anchor);
    }
    draw_sprite_ary(lb_farm_info_frame, &anchor);
    if (plot->state_0x0 == 1) {
        draw_sprite_idx(0x172E, &anchor);
    }
    fn_801EB744(0x1749, &anchor);
    fn_801EB744(0x174A, &anchor);
    if (self->state_0x00 == 3) {
        draw_sprite_idx(lb_farm_plot_title_ids_pick[self->plot_0x08], &anchor);
    } else {
        draw_sprite_idx(lb_farm_plot_title_ids[self->plot_0x08], &anchor);
    }
    spr_data_copy((s16*)&spr, get_lsp_data(0x1728, NULL));
    if (active != 0 && self->action_0x04 == 0) {
        spr.color_0x1C = GetMenuFontColor(1, 1, 1, 0);
        font = 5;
    } else {
        font = 1;
    }
    if (plot->seed_0x3 != 0xFF && plot->state_0x0 == 2) {
        str = LbStr(0, plot->seed_0x3 + 0x15C);
    } else {
        str = LbStr(1, 0);
    }
    draw_font((const _SPR_DATA_&)spr, (s8*)str, font, &anchor);
    if (plot->seed_0x3 != 0xFF && plot->state_0x0 == 2) {
        str = LbStr(0, lb_goods_tier_get(plot->seed_0x3) + 0x159);
    } else {
        str = LbStr(1, 0);
    }
    draw_font_idx(0x1729, (s8*)str, 1, &anchor);
    if (plot->seed_0x3 != 0xFF && plot->state_0x0 == 2) {
        str = LbStr(0, plot->days_0x4 + 0x148);
    } else {
        str = LbStr(1, 0);
    }
    draw_font_idx(0x172A, (s8*)str, 1, &anchor);
    spr_data_copy((s16*)&spr, get_lsp_data(0x172B, NULL));
    if (active != 0 && self->action_0x04 == 1) {
        spr.color_0x1C = GetMenuFontColor(1, 1, 1, 0);
        font = 5;
    } else {
        font = 1;
    }
    draw_font((const _SPR_DATA_&)spr, (s8*)ItemName(plot->fertilizer_0x6), font, &anchor);
    if (plot->state_0x0 == 2) {
        grade = plot->grade_planted_0xA;
    } else {
        grade = plot->grade_0x1;
    }
    level = plot->level_0x2;
    if (grade == 3) {
        level++;
    }
    for (i = 0; i < 3; i++) {
        get_lsp_data(lb_farm_grade_star_lsp[i], &star);
        if (grade == 3 && i == level) {
            spr_data_copy((s16*)&spr, get_lsp_data(0x1745, NULL));
        } else if (i <= level) {
            spr_data_copy((s16*)&spr, get_lsp_data(0x174C, NULL));
        } else {
            spr_data_copy((s16*)&spr, get_lsp_data(0x174D, NULL));
        }
        uv_pair_copy(&spr.pos_0x00, &star);
        draw_sprite((const _SPR_DATA_&)spr, &anchor);
    }
    if (level == 3) {
        draw_sprite_idx(0x1745, &anchor);
    }
    if (plot->state_0x0 == 2) {
        draw_font_idx(0x172C, (s8*)LbStr(0, 0x158), 0, &anchor);
    } else {
        if (plot->grade_0x1 < 3) {
            void* bonus = LbStr(0, plot->bonus_0x5 + 0x15C);

            sprintf(text, (char*)LbStr(0, plot->grade_0x1 + 0x150), bonus);
        } else {
            sprintf(text, (char*)LbStr(0, plot->grade_0x1 + 0x150));
        }
        draw_font_idx(0x172C, (s8*)text, 0, &anchor);
    }
    draw_sprite_anim_idx(0x172F, self->plot_look_0x16, &anchor);
}

/* 0x801EBCC0 (0xD4): draws plot `plot`'s frame for the plot picker, with the plant prompt on an empty plot. */
void lb_farm_plot_frame_draw(s16 plot)
{
    _mh_ivec2_ anchor;
    LbFarmPlot* p;

    p = &LB_USER_DATA->farm_plots_0x3F38[plot];
    get_lsp_data(0x1793, &anchor);
    draw_sprite_ary(lb_farm_window_frame, &anchor);
    draw_sprite_ary(lb_farm_plot_window_body, &anchor);
    draw_sprite_ary(lb_farm_plot_window_rows, &anchor);
    if (p->state_0x0 == 2) {
        draw_sprite_ary(lb_farm_plot_growing_frame, &anchor);
    } else {
        draw_sprite_ary(lb_farm_plot_empty_frame, &anchor);
        if (chk_pointer() == 0 && p->state_0x0 != 1) {
            draw_sprite_ary(lb_farm_plot_prompt_frame, &anchor);
            get_lsp_data(0x1719, &anchor);
            lb_button_prompt_draw(0, (s16*)&anchor);
        }
    }
}

/* 0x801EBD94 (0x37C): draws the plot set-up window: the action rows and cursor, the seed picked with its tier and
 * days, the fertilizer and the price. */
void lb_farm_edit_draw(LbFarmWork* self)
{
    LbSprData spr;
    char text[0x10];
    _mh_ivec2_ anchor;
    _mh_ivec2_ pos;
    LbFarmPlot* edit;
    u32 open;
    u32 font;
    u8 prompt;

    edit = &self->edit_0x48;
    get_lsp_data(0x1763, &anchor);
    get_lsp_data(0x1793, &pos);
    draw_sprite_ary(lb_farm_window_frame, &pos);
    draw_sprite_ary(lb_farm_edit_window_body, &pos);
    open = self->step_0x01 != 5;
    switch (self->action_0x04) {
    case 0:
        if (open == 1 && chk_pointer() == 0) {
            draw_sprite_ary(lb_farm_edit_seed_hilite, &anchor);
        }
        draw_sprite_ary(lb_farm_edit_rows, &anchor);
        if (open == 1) {
            get_lsp_data(0x175D, &pos);
            put_menu_cursor(lb_farm_edit_cursor, 0, &pos);
        }
        break;
    case 1:
        if (open == 1 && chk_pointer() == 0) {
            draw_sprite_ary(lb_farm_edit_fert_hilite, &anchor);
        }
        draw_sprite_ary(lb_farm_edit_rows, &anchor);
        if (open == 1) {
            get_lsp_data(0x1762, &pos);
            put_menu_cursor(lb_farm_edit_cursor, 0, &pos);
        }
        break;
    case 2:
        draw_sprite_ary(lb_farm_edit_rows, &anchor);
        break;
    }
    if (self->action_0x04 == 2 && self->step_0x01 == 1) {
        prompt = 1;
    } else {
        prompt = self->seed_menu_0x02 == 1 ? 2 : 0;
    }
    get_lsp_data(0x171A, &pos);
    lb_cursor_prompt_draw(prompt, (s16*)&pos);
    if (edit->seed_0x3 != 0xFF) {
        spr_data_copy((s16*)&spr, get_lsp_data(0x1766, NULL));
        if (self->action_0x04 == 0) {
            font = 5;
            spr.color_0x1C = GetMenuFontColor(1, 1, 1, 0);
        } else {
            font = 1;
        }
        draw_font((const _SPR_DATA_&)spr, (s8*)LbStr(0, edit->seed_0x3 + 0x15C), font, &anchor);
        draw_font_idx(0x1767, (s8*)LbStr(0, lb_goods_tier_get(edit->seed_0x3) + 0x159), font, &anchor);
        draw_font_idx(0x1768, (s8*)LbStr(0, lb_farm_plot_days_get(edit) + 0x148), font, &anchor);
    }
    spr_data_copy((s16*)&spr, get_lsp_data(0x1769, NULL));
    if (self->action_0x04 == 1) {
        font = 5;
        spr.color_0x1C = GetMenuFontColor(1, 1, 1, 0);
    } else {
        font = 1;
    }
    draw_font((const _SPR_DATA_&)spr, (s8*)ItemName(edit->fertilizer_0x6), font, &anchor);
    draw_font_idx(0x1765, (s8*)LbStr(0, 0x144), 0, &anchor);
    sprintf(text, lb_farm_price_fmt, lb_farm_seed_price_get(edit->seed_0x3));
    draw_font_idx(0x176A, (s8*)text, 2, &anchor);
    draw_font_idx(0x176B, (s8*)LbStr(1, 0x2C), 0, &anchor);
}

/* 0x801EC110 (0x84): draws the planting confirm window's blinking frame. */
void lb_farm_confirm_blink_draw(LbFarmWork* self)
{
    _mh_ivec2_ anchor;

    if (chk_pointer() != 1) {
        get_lsp_data(0x1763, &anchor);
        draw_sprite_ary(lb_farm_confirm_frame, &anchor);
        draw_sprite_anim_ary(lb_farm_confirm_blink_frame, self->blink_0x14 % 20, &anchor);
    }
}

/* 0x801EC19C (0x60): draws the tier icon of seed `seed`. */
void lb_farm_seed_tier_icon_draw(u8 seed)
{
    _mh_ivec2_ anchor;

    get_lsp_data(0x17D8, &anchor);
    draw_sprite_idx(0x17DC, &anchor);
    draw_sprite_idx(lb_farm_tier_icon_ids[lb_goods_tier_get(seed)], &anchor);
}

/* 0x801EC1FC (0x170): draws seed `seed`'s info window: its tier, name, first two harvest items, text, price and days. */
void lb_farm_seed_info_draw(u8 seed, u16 fertilizer)
{
    char text[0x10];
    _mh_ivec2_ anchor;
    const LbFarmHarvestRow* row;
    const u16* id;
    s32 i;
    void* unit;

    get_lsp_data(0x17FD, &anchor);
    draw_sprite_ary(lb_farm_seed_info_frame, &anchor);
    lb_farm_seed_tier_icon_draw(seed);
    draw_font_idx(0x180C, (s8*)LbStr(0, seed + 0x15C), 4, &anchor);
    row = lb_farm_harvest_tables[seed];
    id = lb_farm_seed_item_lsp;
    for (i = 0; i < 2; i++) {
        if (row->item_0x0 != 0) {
            draw_font_idx(*id, (s8*)ItemName(row->item_0x0), 1, &anchor);
            row++;
        }
        id++;
    }
    draw_font_idx(0x180F, (s8*)LbStr(0, seed + 0x169), 0, &anchor);
    unit = LbStr(1, 0x2C);
    sprintf(text, lb_farm_seed_price_fmt, lb_farm_seed_price_get(seed), unit);
    draw_font_idx(0x1810, (s8*)text, 2, &anchor);
    draw_font_idx(0x1811, (s8*)LbStr(0, lb_farm_seed_rows[seed].days_0x1 + 0x148), 2, &anchor);
}

/* 0x801EC36C (0x12C): draws the seed list of the shown tier, with the cursor row's info window. */
void lb_farm_seed_list_draw(LbFarmWork* self)
{
    _mh_ivec2_ anchor;
    _mh_ivec2_ pos;
    _mh_ivec2_* cell;
    const u16* id;
    s32 i;
    s32 sel;
    s8 ok;
    s32 enabled;
    u8 seed;

    get_lsp_data(0x17EB, &anchor);
    draw_sprite_ary(lb_farm_seed_list_frame, &anchor);
    lb_page_arrow_draw(0x17D7, self->seed_page_0x10, self->seed_pages_0x12, self->page_moved_0x06, 1);
    id = lb_farm_seed_cell_lsp;
    for (i = 0; i < 4; i++) {
        seed = i + self->seed_page_0x10 * 4;
        ok = lb_farm_seed_ck(&self->edit_0x48, seed);
        if (i == self->seed_cursor_0x0C) {
            sel = 1;
            lb_farm_seed_info_draw(seed, self->edit_0x48.fertilizer_0x6);
        } else {
            sel = 0;
        }
        enabled = ok == 0;
        if (ok != -1) {
            cell = (_mh_ivec2_*)get_lsp_data(*id, NULL);
            pos.x = anchor.x + cell->x;
            pos.y = anchor.y + cell->y;
            lb_item_cell_draw_layout10(seed, 0, (s16*)&pos, enabled, sel, 1, 0x84);
        }
        id++;
    }
}

/* 0x801EC498 (0xD8): draws the farm's plot menu for its step. */
void lb_farm_plot_menu_draw(LbFarmWork* self)
{
    switch (self->step_0x01) {
    case 0:
        lb_farm_plot_tabs_draw(self);
        fn_801EB6F4();
        lb_farm_plot_frame_draw(self->plot_0x08);
        lb_farm_plot_info_draw(self, 1);
        break;
    case 1:
    case 5:
        lb_farm_plot_tabs_draw(self);
        fn_801EB6F4();
        lb_farm_edit_draw(self);
        lb_farm_plot_info_draw(self, 1);
        break;
    case 2:
        lb_farm_seed_list_draw(self);
        break;
    case 3:
        fn_801EC194();
        break;
    case 4:
        lb_farm_plot_tabs_draw(self);
        fn_801EB6F4();
        lb_farm_confirm_blink_draw(self);
        lb_farm_edit_draw(self);
        lb_farm_plot_info_draw(self, 1);
        break;
    }
}

/* 0x801EC570 (0x110): draws the farm screen for its step. */
void lb_farm_screen_draw(void)
{
    LbFarmWork* self;

    self = (LbFarmWork*)lobby_w.menu_0xAC;
    set_zmode(0, 0, 0);
    set_blendmode(4, 5, 1);
    if (self->state_0x00 != 0 && self->state_0x00 != 5) {
        fn_801EB0B8();
        if (self->state_0x00 == 4) {
            menu_money_draw(0x14FC);
        } else {
            lb_points_draw(0x16C3);
        }
    }
    font_set_size(0x12, 0x12);
    switch (self->state_0x00) {
    case 1:
        lb_choice_draw(&self->choice_0x20, 0x16C5, (const u16*)lbl_805B83F4, (const u16*)lbl_80791BE8, 0x1877, 0);
        break;
    case 3:
        lb_farm_plot_tabs_draw(self);
        lb_farm_plot_info_draw(self, 0);
        break;
    case 4:
        fn_801EB524();
        break;
    case 2:
        lb_farm_plot_menu_draw(self);
        break;
    }
    lb_farm_panel_draw(self);
}

#pragma peephole on
/* 0x801EC680 (0xC8): opens the box screen (lobby screen 6) for `model`; `kind` 1 is the small box. */
void lb_talk_screen_open(LbTalkModel* model, u32 kind)
{
    LbBoxWork* self;

    self = (LbBoxWork*)lobby_w.menu_0xAC;
    memset(self, 0, 0x2000);
    self->model_0x10 = model;
    self->kind_0x02 = kind;
    self->cursor_0x06 = 0;
    self->pages_0x0C = 0;
    self->page_0x08 = 0;
    self->row_0x0E = 0;
    if (self->kind_0x02 == 1) {
        self->pages_0x0C = 3;
    } else {
        self->pages_0x0C = 7;
    }
    self->manual_cam_0x03 = 0;
    self->help_0x04 = 1;
    self->stick_ready_0x27 = 0;
    lobby_w.param_0x12D = 0;
    lobby_w.state_0x000 = 6;
    lobby_w.active_0x008 = 1;
    sysSE_req(7);
}
#pragma peephole off

/* 0x801EC748 (0x64): closes the box screen and sends the profile when it changed. */
void lb_box_menu_close(LbBoxWork* self)
{
    lobby_w.state_0x000 = 0;
    lobby_w.active_0x008 = 0;
    if (self->kind_0x02 == 0) {
        camera_lobby_goal_clear();
    }
    if (lobby_w.param_0x12D != 0) {
        lobby_w.param_0x12D = 0;
        ((s32 (*)(u8*))lobby_net_profile_part1_send)(lobby_world_block);
    }
}

/* 0x801EC85C (0x84): the box screen's camera input: the 0xA0 command hands the camera back, 0x40 toggles the help. */
void lb_box_cam_input_step(LbBoxWork* self, u8 locked)
{
    if (locked == 0 && lb_cmd_pressed_ck(0xA0) == 1) {
        self->manual_cam_0x03 = 0;
        camera_lobby_manual_set(0);
        sysSE_req(5);
    } else if (lb_cmd_pressed_ck(0x40) == 1) {
        self->help_0x04 ^= 1;
    }
    lb_box_model_turn_step(self);
}

/* 0x801EC8E0 (0x7C): turns `model` by the pad's left/right stick bits while the network is idle. */
void lb_box_model_turn(LbTalkModel* model)
{
    u16 press;

    if (lobby_net_busy_ck() != 1) {
        press = Psw[0].button_0x2C0.press_0x08;
        if ((press & 0xC00) != 0) {
            if ((press & 0x800) != 0) {
                model->yaw_0xA8 -= 0x400;
            }
            if ((press & 0x400) != 0) {
                model->yaw_0xA8 += 0x400;
            }
        }
    }
}

/* 0x801EC95C (0x84): turns the box screen's model once the stick was let go after the screen opened. */
void lb_box_model_turn_step(LbBoxWork* self)
{
    LbTalkModel* model = self->model_0x10;

    if (self->kind_0x02 == 0) {
        if ((Psw[0].button_0x2C0.press_0x08 & 0x3C00) != 0 && lobby_net_busy_ck() == 0) {
            if (self->stick_ready_0x27 == 1) {
                lb_box_model_turn(model);
            }
        } else {
            self->stick_ready_0x27 = 1;
        }
    }
}

} /* extern "C" */
