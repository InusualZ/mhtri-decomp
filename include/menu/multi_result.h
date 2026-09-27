/*
 * `menu/multi_result.cpp`'s records and outbound declarations.
 *
 * The unit is the multiplayer-result screen's box cursor band plus the enemy action/substate
 * dispatchers that share its address range (see the unit header for the seam evidence).  Its two
 * defining symbols are the box helpers on `_multi_result_work`, the 0x18-byte record the screen
 * keeps four of at +0x33DC of its work buffer - the mangled map name `...__FP18_multi_result_work`
 * is the original struct's own spelling (`18` is the name's length, and the record's stride in
 * `multi_box_records_step` is 0x18), so the type keeps that name: a rename would ask the front-end
 * for a different symbol.
 *
 * The record's `work` pointer points back at the screen object that owns the record, whose box grids
 * sit at +0x3070 / +0x3130 (`MultiResultBoxGrids` is this unit's view of that part; the owner's
 * `QResultScreen` in `menu/menu_result.h` names the same two runs as `grid_entries`/`list_entries`).
 *
 * The owners of the symbols this unit only *references* are: `menu/menu_result.cpp`
 * (`q_result_phase_is_2`, `q_result_phase_is_3`), `enemy/fn_8011D448.cpp`
 * (`em_parts_damage_level_get`) - both included, never re-declared (rule 2).
 */
#ifndef MHTRI_MENU_MULTI_RESULT_H
#define MHTRI_MENU_MULTI_RESULT_H

#include "types.h"
#include "menu/menu_result.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8011D448.h"

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
/* The game-mode/quest flow object the tail of the range drives: its +0x08/+0x09 state bytes are
 * written by `fn_803A13B4`, `fn_803A168C`, `fn_803A1B04` and `fn_803A2248`, and `fn_803A13B4` reads
 * +0x09 back.  Only +0x09 is reachable from this unit's own body.  size: 0xA (approximate) */
typedef struct GameModeTask {
    /* +0x00 */ u8 pad_0x00[0x9];
    /* +0x09 */ u8 sub_state;
} GameModeTask; /* size: 0xA (approximate - only +0x09 is reachable from the range) */

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

/* The handlers the dispatchers above tail-call; their bodies are the next pass's work, so only the
 * dispatchers themselves are defined here (the map rows are renamed in the same change). */
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

/* `multi_box_phase_step` asks this one whether the screen is ready for the phase it names; the body
 * is the next pass's work (it needs `get_vsUser_work` declared by its owner, `fn_8004CAD8.cpp`). */
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

/* Sets the flow object's sub-state to 1 (called from `fn_803A13B4`'s state machine). */
void game_mode_sub_state_set1(GameModeTask* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MULTI_RESULT_H */
