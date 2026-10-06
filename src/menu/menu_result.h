/*
 * menu/menu_result.h - `menu/menu_result.cpp`'s records and outbound declarations.  `QResultScreen` is the 0x350C-byte
 *   result-screen work the quest move work's `result_buffer_0x150` points at (`q_result_work_clear` zeroes exactly
 *   that much); `QResultHuntRec` is one row of its hunted-monster size table.  `q_result_msg_adrs` is the unit's own
 *   `.bss`; every callee comes from its owner's header.
 */
#ifndef MHTRI_MENU_MENU_RESULT_H
#define MHTRI_MENU_MENU_RESULT_H

#include "types.h"
#include "id_value.h"                  /* IdValue: the (item id, count) pairs of the result's item grids */
#include "quest/quest_result_work.h"   /* Q_ResultWork: the one view of the 0x438-byte quest result record (rule 1) */
#include "Pl/plw.h"                    /* _EQUIP: the 12-byte equipment record */

struct Q_MoveWork;
struct _EFT;

/* One row of the screen's hunted-monster size table: which of the two size records the hunt beat, the monster's
 * index and the two sizes it measured.  The rows are filled packed, `hunt_rec_count` of them.  size: 0x10 */
typedef struct QResultHuntRec {
    /* +0x00 */ u8 new_flags;          /* bit 0: a new smallest size, bit 1: a new largest size */
    /* +0x01 */ u8 monster;            /* the monster index the sizes belong to */
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ u16 size_min;          /* the hunt's smallest size of the monster */
    /* +0x06 */ u16 size_max;          /* the hunt's largest size of the monster */
    /* +0x08 */ u8 pad_0x08[0x8];
} QResultHuntRec; /* size: 0x10 */

/* The quest-result screen work: the step machine's state, the reward rows the phases add up, the hunted-monster
 * size rows, the item grids the item phases move between and the screen's cursors.  `menu/multi_result.cpp` reads
 * the same block through its box views (`MultiResultBoxGrids`, `MultiResultRecordArray`).  size: 0x350C */
typedef struct QResultScreen {
    /* +0x0000 */ u8 state;                    /* 0 while the screen sets itself up, 1 while it runs */
    /* +0x0001 */ u8 sub_state;                /* the running screen's step: enter, run, fade, close */
    /* +0x0002 */ u8 select_mode;              /* the item phases' cursor mode (command row, grid, list, ...) */
    /* +0x0003 */ u8 phase_step;               /* cleared on every phase entry */
    /* +0x0004 */ u8 pad_0x0004;
    /* +0x0005 */ u8 anim_step;                /* per-frame step counter */
    /* +0x0006 */ u8 player_no;                /* the local player (`my_player_no`) */
    /* +0x0007 */ u8 field_0x0007;             /* the player count the rewards are split by */
    /* +0x0008 */ s8 faint_count;              /* the local player's faints; each costs 20 points */
    /* +0x0009 */ s8 reward_shown[3];          /* the main quest and the two sub-quests are on the reward page */
    /* +0x000C */ s8 reward_cleared[3];        /* the main quest and the two sub-quests were cleared */
    /* +0x000F */ u8 pad_0x000F;
    /* +0x0010 */ s32 zenny_rows[4];           /* the fee refund, then the main and the two sub-quest rewards */
    /* +0x0020 */ s32 point_rows[3];           /* the main and the two sub-quest hunter points */
    /* +0x002C */ s32 hunt_count[2];           /* how many of each of the two hunted monster tiers */
    /* +0x0034 */ s32 hunt_zenny[2];           /* the reward per monster of each tier */
    /* +0x003C */ s32 hunt_points[2];          /* the points per monster of each tier */
    /* +0x0044 */ u8 hunt_monster[2];          /* the monster of each tier */
    /* +0x0046 */ u8 pad_0x0046[0x2];
    /* +0x0048 */ s32 zenny_total;             /* the quest reward the result record carries */
    /* +0x004C */ s32 points_total;            /* the hunter points the result record carries */
    /* +0x0050 */ s32 hunt_zenny_total;        /* the per-monster rewards, summed */
    /* +0x0054 */ s32 hunt_points_total;       /* the per-monster points, summed */
    /* +0x0058 */ s32 item_value_total;        /* the value of the items the result keeps */
    /* +0x005C */ u8 skill_points;             /* the player has the skill (category 0x1B) that raises the points */
    /* +0x005D */ u8 skill_zenny;              /* the player has the skill (category 0x29) that raises the reward */
    /* +0x005E */ u8 reward_cut;               /* the rewards are cut to three quarters */
    /* +0x005F */ u8 pad_0x005F;
    /* +0x0060 */ QResultHuntRec hunt_recs[10];  /* the hunted-monster size rows */
    /* +0x0100 */ s32 hunt_rec_count;          /* how many of `hunt_recs` are filled */
    /* +0x0104 */ u8 pad_0x0104[0x2FE4 - 0x104];
    /* +0x2FE4 */ u32 unlock_bits[2];          /* the unlock flags the quest raised and the save has not seen */
    /* +0x2FEC */ u16 unlock_events;           /* the event flags the result raised */
    /* +0x2FEE */ u8 pad_0x2FEE[0x2];
    /* +0x2FF0 */ s32 phase;                   /* the state machine's phase (0..8) */
    /* +0x2FF4 */ s32 phase_timer;             /* frames left in the phase */
    /* +0x2FF8 */ u8 phase_frame;              /* cleared on every phase entry */
    /* +0x2FF9 */ u8 pad_0x2FF9;
    /* +0x2FFA */ IdValue kept_items[8];       /* the items the result keeps, packed */
    /* +0x301A */ u8 pad_0x301A[0x2];
    /* +0x301C */ s32 kept_item_count;         /* how many of `kept_items` are filled */
    /* +0x3020 */ s16 unlock_wait;             /* frames before the next unlock notice may be confirmed */
    /* +0x3022 */ u8 cursor;                   /* the page or command row the cursor is on */
    /* +0x3023 */ u8 cursor_count;             /* how many rows `cursor` steps through */
    /* +0x3024 */ u16 page_moved;              /* the page-step flags the arrows light up with */
    /* +0x3026 */ u8 open_frame;               /* the panels' opening frame, counted up to 7 */
    /* +0x3027 */ u8 init_flag;                /* set once, when the band latches */
    /* +0x3028 */ u8 grid_cursor_col;          /* the item grid: column, 8 per row */
    /* +0x3029 */ u8 grid_cursor_row;          /* the item grid: row */
    /* +0x302A */ u8 moved_col;                /* the grid cell the last moved item came from */
    /* +0x302B */ u8 moved_row;
    /* +0x302C */ u8 grid_rows;                /* the item grid's row count */
    /* +0x302D */ u8 list_cursor_row;          /* the box list: scaled by eight in the flat index */
    /* +0x302E */ u8 list_rows;                /* the box list's row count */
    /* +0x302F */ u8 list_cursor_col;          /* the box list: added unscaled */
    /* +0x3030 */ u8 sell_mode;                /* the grid's command: 0 keeps the items, 1 sells them */
    /* +0x3031 */ u8 equip_ready;              /* the equipment grid has been filled */
    /* +0x3032 */ u16 equip_wait;              /* frames the equipment grid's opening has run */
    /* +0x3034 */ u16 cursor_blink;            /* the cursor's blink frame, wraps at 40 */
    /* +0x3036 */ u16 moved_anim;              /* the moved item's flash frame, 0xFFFF when idle */
    /* +0x3038 */ u16 equip_sel_bits[3];       /* the equipment cells marked for selling, one bit each */
    /* +0x303E */ u8 pad_0x303E[0x2];
    /* +0x3040 */ s32 sell_confirm;            /* the sell dialog's yes/no word */
    /* +0x3044 */ IdValue moved_item;          /* the item the last move took */
    /* +0x3048 */ IdValue* box_main;           /* the item box's first 0x18 slots */
    /* +0x304C */ IdValue* box_extra;          /* the gunner box's 8 slots, or null */
    /* +0x3050 */ s32 sell_total;              /* what the sell dialog pays */
    /* +0x3054 */ u8 pad_0x3054[0x4];
    /* +0x3058 */ s32 rank_state;              /* the hunter rank's next-rank state */
    /* +0x305C */ u16 rank_before;             /* the hunter rank before the quest */
    /* +0x305E */ u16 rank_anim;               /* the rank-up animation frame, counted up to 40 */
    /* +0x3060 */ u8 ready_flag;               /* the result carries items to take (`q_result_phase_ck`) */
    /* +0x3061 */ u8 hunt_anim[10];            /* each size row's flash frame */
    /* +0x306B */ u8 pad_0x306B;
    /* +0x306C */ s16 swap_timer;              /* armed to 90 (`0x5A`) frames */
    /* +0x306E */ u8 swap_counter;             /* how many unlock notices the screen shows */
    /* +0x306F */ u8 unlock_text;              /* the unlock notice on show (0xFF: none) */
    /* +0x3070 */ IdValue grid_items[0x30];    /* the items the quest delivered */
    /* +0x3130 */ IdValue grid_kept[0x30];     /* the items the result keeps */
    /* +0x31F0 */ _EQUIP equip_cells[0x28];    /* the equipment the quest delivered */
    /* +0x33D0 */ u8 pad_0x33D0[0x4];
    /* +0x33D4 */ s32 box_cursor;              /* the box band's cursor, -1 when off */
    /* +0x33D8 */ s32 box_frame;
    /* +0x33DC */ u8 box_records[0x350C - 0x33DC];  /* the box band's player records (`MultiResultRecordArray`) */
} QResultScreen; /* size: 0x350C */

#ifdef __cplusplus
extern "C" {
#endif

/* The `.bss` message table at 0x806C5528 (the dump's own name, 12 pointers):
 * `q_result_msg_adrs[message id][language]` is the pointer array the screen indexes by string id. */
extern char*** q_result_msg_adrs[12];

/* This unit's own entry points (named for what they do; see the unit header - every name is a
 * GUESS recorded there). */
void q_result_effect_follow_npc(struct _EFT* self);
void q_result_anim_counter_inc(struct _EFT* self);
void q_result_release_effect(struct _EFT* self);
char** q_result_msg_table(u8 id);
char* q_result_msg_entry(u8 id, u8 index);
char** q_result_msg_table_alt(u8 id);
char* q_result_msg_string(u8 id);
char* q_result_msg_string_alt(u8 id);
BOOL q_result_work_clear(struct Q_MoveWork* move);
u32 q_result_file_ready(QResultScreen* self);
u32 q_result_sub_screen_ready(QResultScreen* self);
IdValue* q_result_grid_entry(QResultScreen* self, u8 which);
IdValue* q_result_list_entry(QResultScreen* self);
void q_result_row_init(QResultScreen* self, const IdValue* item, u8 col, u8 row);
BOOL q_result_ready_ck(QResultScreen* self);
u8 q_result_swap_counter_get(QResultScreen* self);
u32 q_result_phase_is_2(QResultScreen* self);
u32 q_result_phase_is_3(QResultScreen* self);
void q_result_init_flag_set(QResultScreen* self);
void q_result_swap_start(QResultScreen* self);
u16 q_result_grid_cursor_index(QResultScreen* self);
u16 q_result_list_cursor_index(QResultScreen* self);
void q_result_draw_icon_value(QResultScreen* self);
void q_result_font_print_a(s16 row, u8 frame_kind);
void q_result_font_print_b(s16 row, s32 value, u8 frame_kind);
void q_result_font_print_row(s16 row, u8 with_value, s32 value, u8 frame_kind);
void q_result_page_set(QResultScreen* self, u8 mode);
void q_result_page_step_a(QResultScreen* self);
void q_result_page_step_b(QResultScreen* self);
void q_result_noop(QResultScreen* self);
BOOL q_result_phase_ck(QResultScreen* self, u8 mode);
/* 0x8039D110 / 0x803980F0 - the screen's phase entry and its per-frame phase advance, the two
 * helpers `menu/multi_result.cpp` drives its box band through. */
void q_result_phase_enter(QResultScreen* self, s32 phase);
void q_result_phase_apply(QResultScreen* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_RESULT_H */
