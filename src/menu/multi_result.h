/*
 * menu/multi_result.h - `menu/multi_result.cpp`'s records and outbound declarations: the lobby/game-mode flow, the
 *   Poogie and name menus, the note pane and the NPC trade/gift rolls.  The box band's records live in
 *   `menu/menu_result.h` with its owner, the enemy 029 handlers in `enemy/em029_prog.h`.
 */
#ifndef MHTRI_MENU_MULTI_RESULT_H
#define MHTRI_MENU_MULTI_RESULT_H

#include "types.h"
#include "menu/menu_result.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8011D448.h"
#include "mh3_pad/task.h"
#include "lobby/lb_list_init.h"   /* LbList: the screens' list records */     /* TaskSlot: the game-mode task the flow tail drives */

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

#ifdef __cplusplus
extern "C" {
#endif

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
