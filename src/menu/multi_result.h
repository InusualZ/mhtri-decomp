/*
 * menu/multi_result.h - `menu/multi_result.cpp`'s records and outbound declarations.  `_multi_result_work` keeps the
 *   original struct name the map's `...__FP18_multi_result_work` spells (`18` is its length; the screen keeps four of
 *   these 0x18-byte records at +0x33DC), and its `work` pointer leads back to the screen object whose box grids sit at
 *   +0x3070/+0x3130 (`MultiResultBoxGrids`; `menu/menu_result.h`'s `QResultScreen` names them `grid_entries`/
 *   `list_entries`).
 */
#ifndef MHTRI_MENU_MULTI_RESULT_H
#define MHTRI_MENU_MULTI_RESULT_H

#include "types.h"
#include "menu/menu_result.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8011D448.h"
#include "mh3_pad/task.h"
#include "lobby/lb_list_init.h"   /* LbList: the screens' list records */     /* TaskSlot: the game-mode task the flow tail drives */

/* One 4-byte slot of a box grid: the item id and how many of it are left.  Both fields are read by
 * `multi_box_rem_exist_ck` (`lhz` + `lha`) and written by `fn_8039D490` (both halves zeroed).
 * size: 0x4 */
typedef struct MultiResultBoxItem {
    /* +0x00 */ u16 item_id;
    /* +0x02 */ s16 count;
} MultiResultBoxItem; /* size: 0x4 */

/* The two 16-slot box grids the screen keeps at +0x3070 ("my box") and +0x3130 ("the others"), as
 * this unit's functions index them.  size: 0x3170 (a *view*: the screen object continues) */
typedef struct MultiResultBoxGrids {
    /* +0x0000 */ u8 pad_0x0000[0x3070];
    /* +0x3070 */ MultiResultBoxItem my_box[0x10];
    /* +0x30B0 */ u8 pad_0x30B0[0x3130 - 0x30B0];
    /* +0x3130 */ MultiResultBoxItem other_box[0x10];
} MultiResultBoxGrids; /* size: 0x3170 (approximate - a view of the screen object's two grids) */

/* One player's box record.  The screen keeps an array of them at +0x33DC of its work buffer and
 * iterates `player_count` of them (stride 0x18, proven by the +0x18 pointer bump in the loop that
 * calls `multi_box_records_step`).  Every field named here is one this unit's functions touch.
 * size: 0x18 */
typedef struct _multi_result_work {
    /* +0x00 */ u8 state;             /* 0 = choosing, 1 = confirmed, 0xFF = done */
    /* +0x01 */ u8 player_no;         /* indexes `Psw` (stride 0x350) and `get_vsUser_work` */
    /* +0x02 */ u8 field_0x02;        /* the box the cursor was on before the step */
    /* +0x03 */ u8 field_0x03;        /* the box count the cursor steps through */
    /* +0x04 */ u8 cursor_x;          /* grid cursor, column */
    /* +0x05 */ u8 cursor_y;          /* grid cursor, row */
    /* +0x06 */ u8 box_w;             /* the grid's width  (cursor index = x + y * box_w) */
    /* +0x07 */ u8 box_h;             /* the grid's height */
    /* +0x08 */ u8 field_0x08;        /* a counter that saturates at 2 */
    /* +0x09 */ u8 saved_cursor_x;    /* the cursor latched when the box was confirmed */
    /* +0x0A */ u8 saved_cursor_y;
    /* +0x0B */ u8 pad_0x0B;
    /* +0x0C */ s16 field_0x0C;        /* a timer that wraps at 12 */
    /* +0x0E */ s16 field_0x0E;        /* a timer that wraps at 40 */
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ MultiResultBoxGrids* work; /* the screen object that owns this record */
} _multi_result_work; /* size: 0x18 */

/* The screen work buffer's record array, as this unit reaches it (past the owner's `QResultScreen`
 * view, whose size is an approximation).  size: 0x33DC + 4 * 0x18 */
typedef struct MultiResultRecordArray {
    /* +0x0000 */ u8 pad_0x0000[0x33DC];
    /* +0x33DC */ _multi_result_work records[4];
} MultiResultRecordArray; /* size: 0x343C (a view of the screen object's record array) */

/* This unit's own entry points (named for what they do; every name is a GUESS recorded in the unit
 * header - the range has no `__FILE__` string and the runtime dump's only in-range names are the
 * two `multi_box_*` manglings plus junk duplicates).  C linkage, like the neighbour
 * `menu/menu_result.cpp`: the map rows these replace are plain names, so the front-end must not
 * mangle them (the two `multi_box_*` whose map names are real manglings stay C++). */

/* One 0xA-byte row of the player act-name table `pl_act_name_row_get` hands out: the frame the act's hand-over
 * starts on and three parameters (the first is also the act's length from there).  size: 0xA */
typedef struct LbActRow {
    /* +0x00 */ u8 pad_0x00[0x2];
    /* +0x02 */ s16 start_0x02;
    /* +0x04 */ s16 param_0x04;
    /* +0x06 */ s16 param_0x06;
    /* +0x08 */ s16 param_0x08;
} LbActRow;

/* Screen 0x15, the Poogie menu, in `lobby_w`'s 0x2000-byte menu block: its state, the top menu and the costume
 * list, the petting act's row and frame, and the player and Poogie NPC it runs between.  size: 0x60 (a view of the
 * 0x2000-byte block) */
typedef struct LbPigMenuWork {
    /* +0x00 */ u8 state_0x00;
    /* +0x01 */ u8 sub_0x01;
    /* +0x02 */ u8 petted_0x02;        /* 1 once the petting act ran to its end */
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ s16 frame_0x04;        /* the petting act's frame */
    /* +0x06 */ s16 wait_0x06;
    /* +0x08 */ s32 confirm_0x08;      /* the yes/no toggle */
    /* +0x0C */ LbList menu_0x0C;      /* the top menu (pet / costume) */
    /* +0x2C */ LbList dress_0x2C;     /* the costume list */
    /* +0x4C */ u8 dress_ids_0x4C[8];  /* the costume of each list row */
    /* +0x54 */ LbActRow* pet_row_0x54;
    /* +0x58 */ struct _PLW* player_0x58;
    /* +0x5C */ struct _LB_NPC* npc_0x5C;
} LbPigMenuWork;

/* Screen 0x16, the name menu of a kitchen Felyne or the Poogie, in the same block: its state, the cursor, the name
 * count, the page, the rows of the page, the current name and the name table.  size: 0x2C (a view of the
 * 0x2000-byte block) */
typedef struct LbNameMenuWork {
    /* +0x00 */ u8 state_0x00;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ s16 cursor_0x02;
    /* +0x04 */ s16 total_0x04;
    /* +0x06 */ s16 page_0x06;
    /* +0x08 */ s16 pages_0x08;
    /* +0x0A */ s16 rows_0x0A;
    /* +0x0C */ u16 moved_0x0C;        /* the page step's "moved" flag `menu_cursor_step_fixed_tail` writes */
    /* +0x0E */ s16 current_0x0E;      /* the name the NPC has */
    /* +0x10 */ s16 row_ids_0x10[6];   /* the name of each row of the page */
    /* +0x1C */ s32 confirm_0x1C;      /* the yes/no toggle */
    /* +0x20 */ s32* names_0x20;       /* the NPC kind's name table (`lb_cat_name` / `lb_pig_name`) */
    /* +0x24 */ struct _PLW* player_0x24;
    /* +0x28 */ struct _LB_NPC* npc_0x28;
} LbNameMenuWork;

/* The two box helpers whose map names are the range's own manglings stay C++ (the front-end has to
 * reproduce `...__FP18_multi_result_work`), so they are declared before the C block. */
MultiResultBoxItem* multi_box_cursor_item_get(_multi_result_work* box);
u32 multi_box_rem_exist_ck(_multi_result_work* box);

#ifdef __cplusplus
extern "C" {
#endif

u16 multi_box_cursor_index(_multi_result_work* box);
u32 multi_box_phase_step(QResultScreen* self, u8 mode);
/* The box band's own frame step and phase latch (the `fn_8039D278`/`multi_box_phase_step` pair the
 * screen's dispatcher runs). */
void multi_box_phase_apply(QResultScreen* self);
/* Credits the player's box contents into their VS user block and empties the grid. */
void multi_box_grid_clear(_multi_result_work* box, struct _vs_user_data* user);

/* The enemy action/substate dispatchers in the same range: `em_action_dispatch` runs the handler
 * for `_ENEMY_WORK::action`, each `em_actionN_dispatch` the one for `state_sub`. */
void em_action_dispatch(struct _ENEMY_WORK* self);
void em_action0_dispatch(struct _ENEMY_WORK* self);
void em_action1_dispatch(struct _ENEMY_WORK* self);
void em_action2_dispatch(struct _ENEMY_WORK* self);
void em_action3_dispatch(struct _ENEMY_WORK* self);
void em_action4_dispatch(struct _ENEMY_WORK* self);
void em_action5_dispatch(struct _ENEMY_WORK* self);
void em_action6_dispatch(struct _ENEMY_WORK* self);
void em_action7_dispatch(struct _ENEMY_WORK* self);

/* The handlers the dispatchers above tail-call; their bodies are not written yet. */
void em_action0_step(struct _ENEMY_WORK* self);
void em_action1_sub0(struct _ENEMY_WORK* self);
void em_action1_sub1(struct _ENEMY_WORK* self);
void em_action1_sub2(struct _ENEMY_WORK* self, u8 mode);
void em_action1_sub3(struct _ENEMY_WORK* self);
void em_action1_sub4(struct _ENEMY_WORK* self);
void em_action1_sub5(struct _ENEMY_WORK* self);
void em_action1_sub6(struct _ENEMY_WORK* self);
void em_action1_sub7(struct _ENEMY_WORK* self);
void em_action1_sub8(struct _ENEMY_WORK* self);
void em_action2_sub0(struct _ENEMY_WORK* self, f32 value);
void em_action2_sub1(struct _ENEMY_WORK* self, u8 mode);
void em_action2_sub2(struct _ENEMY_WORK* self, u8 mode);
void em_action2_sub3(struct _ENEMY_WORK* self);
void em_action2_sub4(struct _ENEMY_WORK* self, u8 mode);
void em_action2_sub5(struct _ENEMY_WORK* self, f32 value);
void em_action3_sub0(struct _ENEMY_WORK* self, u8 mode);
void em_action3_sub1(struct _ENEMY_WORK* self);
void em_action3_sub2(struct _ENEMY_WORK* self);
void em_action6_sub0(struct _ENEMY_WORK* self);
void em_action6_sub1(struct _ENEMY_WORK* self);
void em_action7_step(struct _ENEMY_WORK* self);

/* `multi_box_phase_step` asks this one whether the screen is ready for the phase it names. */
u32 multi_box_phase_ck(QResultScreen* self, u8 mode);

/* Whether the area already holds an active team-19 enemy, the action band's hand-over test. */
u32 em_area_team_ck(u8 area);

/* Two stub rows inside the range: an empty one and a `return 0` one.  The runtime dump's names for
 * them (`DBClose`, `gdev_cc_shutdown`) are its *junk* mappings - both spellings appear at 295 and 133
 * addresses respectively, i.e. the dump's import duplicated them - so they are named for their
 * bodies and the band they sit in instead (rule 7, evidence class 4). */
void em_action_nop(void);
s32 em_action_ret0(void);

/* `fn_803A11D4`'s own helper view: 1 when part 0 is undamaged. */
u32 em_parts_damage0_ck(struct _ENEMY_WORK* self, u8 part);

/* The game-mode flow the tail of the range drives on the task's +0x08..+0x0B state bytes: the task body, its
 * set-up and its three lobby sub-states, and the area change (GUESS names). */
void game_mode_sub_state_set1(TaskSlot* self);
void lobby_flow_init(void);
void game_mode_flow_task(TaskSlot* task);
void lobby_flow_boot_step(TaskSlot* task);
void lobby_flow_main_step(TaskSlot* task);
void lobby_area_change_net_req(void);
void lobby_area_change_req(void);
void lobby_area_change_step(TaskSlot* task);
void lobby_flow_leave_step(TaskSlot* task);
/* The Poogie menu (screen 0x15) and the name menu (screen 0x16) (GUESS names). */
void lobby_pig_dress_view_set(u8 dress);
void pig_dress_npc_set(u8 dress);
void pig_menu_open(struct _PLW* player, struct _LB_NPC* npc);
void pig_menu_close(void);
void pig_menu_step(void);
void pig_dress_list_build(LbPigMenuWork* w);
void pig_dress_list_draw(LbPigMenuWork* w, bool active);
void pig_menu_draw(void);
void name_menu_open(struct _PLW* player, struct _LB_NPC* npc);
void name_menu_close(void);
void name_menu_page_build(LbNameMenuWork* w, s32 page);
void name_menu_step(void);
void name_menu_list_draw(LbNameMenuWork* w, bool active);
void name_menu_draw(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MULTI_RESULT_H */
