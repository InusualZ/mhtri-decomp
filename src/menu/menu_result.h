/*
 * menu/menu_result.h - `menu/menu_result.cpp`'s records and outbound declarations.  `QResultScreen` is the record the
 *   band's functions take first (a view: only the fields the unit touches are named).  `system_w`, `Psw` and the
 *   `sprintf`/`strcpy`/`strcat` family are declared here (their owners' headers do not declare them yet);
 *   `q_result_msg_adrs` is the unit's own `.bss`; every other callee comes from its owner's header.
 */
#ifndef MHTRI_MENU_MENU_RESULT_H
#define MHTRI_MENU_MENU_RESULT_H

#include "types.h"
#include "quest/quest_result_work.h"   /* Q_ResultWork: the one view of the 0x438-byte quest result record (rule 1) */

/* The 0x350C-byte work buffer `QResultScreen::work` points at; only its first byte is read here.
 * size: 0x350C */
typedef struct QResultScreenWork {
    /* +0x0000 */ u8 field_0x0000;     /* non-zero once the buffer is filled in */
    /* +0x0001 */ u8 pad_0x0001[0x350C - 0x0001];
} QResultScreenWork; /* size: 0x350C */

/* The quest-result screen record the band's functions take as their first argument.  The size is an
 * *approximation*: the last field this unit touches is `swap_counter` at +0x306E and no allocation
 * of the record is reachable from the band, so the size is rounded up to the next 16-byte boundary.
 * size: 0x3070 (approximate) */
typedef struct QResultScreen {
    /* +0x0000 */ u8 state;                    /* the screen's own phase */
    /* +0x0001 */ u8 sub_state;
    /* +0x0002 */ u8 field_0x0002;
    /* +0x0003 */ u8 pad_0x0003[0x005 - 0x003];
    /* +0x0005 */ u8 anim_step;                /* per-frame step counter (two bodies bump it) */
    /* +0x0006 */ u8 field_0x0006;
    /* +0x0007 */ u8 field_0x0007;
    /* +0x0008 */ u8 pad_0x0008[0x018 - 0x008];
    /* +0x0018 */ f32 pos_0x0018[3];           /* the screen's world position */
    /* +0x0024 */ u8 pad_0x0024[0x030 - 0x024];
    /* +0x0030 */ void* npc;                   /* the `_LB_NPC` the screen poses */
    /* +0x0034 */ u8 pad_0x0034[0x038 - 0x034];
    /* +0x0038 */ void* chara;                 /* the `MHchar`/`_LB_NPC` pair the pose drives */
    /* +0x003C */ u8 pad_0x003C[0x05E - 0x03C];
    /* +0x005E */ u8 field_0x005E;
    /* +0x005F */ u8 pad_0x005F[0x100 - 0x05F];
    /* +0x0100 */ s32 field_0x0100;
    /* +0x0104 */ u8 pad_0x0104[0x150 - 0x104];
    /* +0x0150 */ QResultScreenWork* work;     /* the 0x350C-byte screen work buffer */
    /* +0x0154 */ u8 pad_0x0154[0x22E2 - 0x154];
    /* +0x22E2 */ u8 field_0x22E2;
    /* +0x22E3 */ u8 pad_0x22E3[0x2FF0 - 0x22E3];
    /* +0x2FF0 */ s32 phase;                   /* the state machine's phase (0..8) */
    /* +0x2FF4 */ s32 phase_timer;             /* frames left in the phase */
    /* +0x2FF8 */ u8 pad_0x2FF8[0x3027 - 0x2FF8];
    /* +0x3027 */ u8 init_flag;                /* set once, when the band latches */
    /* +0x3028 */ u8 grid_cursor_col;          /* first list: column, 8 per row */
    /* +0x3029 */ u8 grid_cursor_row;          /* first list: row */
    /* +0x302A */ u8 field_0x302A;              /* row state the row initialiser copies in */
    /* +0x302B */ u8 field_0x302B;
    /* +0x302C */ u8 pad_0x302C[0x302D - 0x302C];
    /* +0x302D */ u8 list_cursor_row;          /* second list: scaled by eight in the flat index */
    /* +0x302E */ u8 pad_0x302E[0x302F - 0x302E];
    /* +0x302F */ u8 list_cursor_col;          /* second list: added unscaled */
    /* +0x3030 */ u8 pad_0x3030[0x3036 - 0x3030];
    /* +0x3036 */ u16 field_0x3036;
    /* +0x3038 */ u8 pad_0x3038[0x3044 - 0x3038];
    /* +0x3044 */ u16 field_0x3044;             /* the 6-byte pair the row initialiser fills */
    /* +0x3046 */ s16 field_0x3046;
    /* +0x3048 */ u32* table_a_0x3048;          /* row table, 0x18 entries */
    /* +0x304C */ u32* table_b_0x304C;          /* the table an index above 0x17 goes to */
    /* +0x3050 */ u8 pad_0x3050[0x3060 - 0x3050];
    /* +0x3060 */ u8 ready_flag;               /* the `q_result_phase_ck` gate */
    /* +0x3061 */ u8 pad_0x3061[0x306C - 0x3061];
    /* +0x306C */ u16 swap_timer;              /* armed to 90 (`0x5A`) frames */
    /* +0x306E */ u8 swap_counter;
    /* +0x306F */ u8 pad_0x306F[0x3070 - 0x306F];
    /* +0x3070 */ u32 grid_entries[0x30];      /* first grid, indexed by the grid cursor */
    /* +0x3130 */ u32 list_entries[4];         /* second table's head; only its base is read here */
} QResultScreen; /* size: 0x3140 (approximate - the record continues past the last field read) */

#ifdef __cplusplus
extern "C" {
#endif

/* The `.bss` message table at 0x806C5528 (the dump's own name, 12 pointers): 
 * `q_result_msg_adrs[message id][language]` is the pointer array the screen indexes by string id. */
extern char*** q_result_msg_adrs[12];

/* The game/system state block at 0x806585E0; only its +9 byte (the language index) is read here. */
extern u8 system_w[];

/* `get_qResult_work()` comes from its owner's header, `fn_8004CAD8.h`, which this unit includes. */

void fn_802DF6E4(u16 sprite_id);
void q_result_font_print_row(s16 row, s32 flag, s32 sub, u8 sprite_id);
void q_result_page_set(QResultScreen* self, s32 mode);

/* This unit's own entry points (named for what they do; see the unit header - every name is a
 * GUESS recorded there). */
void q_result_anim_counter_inc(QResultScreen* self);
void q_result_release_effect(QResultScreen* self);
char** q_result_msg_table(u8 id);
char* q_result_msg_entry(u8 id, u8 index);
char** q_result_msg_table_alt(u8 id);
char* q_result_msg_string(u8 id);
char* q_result_msg_string_alt(u8 id);
BOOL q_result_work_clear(QResultScreen* self);
u32 q_result_file_ready(QResultScreen* self);
BOOL q_result_sub_screen_ready(void);
u32 fn_803B4C64(void* ptr);
u32 fn_803B4CE8(void* ptr);
u32* q_result_grid_entry(QResultScreen* self, u8 which);
u32* q_result_list_entry(QResultScreen* self);
void q_result_row_init(QResultScreen* self, void* src, u8 a, u8 b);
BOOL q_result_ready_ck(QResultScreen* self);
u8 q_result_swap_counter_get(QResultScreen* self);
u32 q_result_phase_is_2(QResultScreen* self);
u32 q_result_phase_is_3(QResultScreen* self);
void q_result_init_flag_set(QResultScreen* self);
void q_result_swap_start(QResultScreen* self);
u16 q_result_grid_cursor_index(QResultScreen* self);
u16 q_result_list_cursor_index(QResultScreen* self);
void q_result_draw_icon_value(void);
void q_result_font_print_a(s16 row, u8 sprite_id);
void q_result_font_print_b(s16 row, s32 flag, u8 sprite_id);
void q_result_page_step_a(QResultScreen* self);
void q_result_page_step_b(QResultScreen* self);
void q_result_noop(void* unused);
BOOL q_result_phase_ck(QResultScreen* self, u8 mode);
/* 0x8039D110 / 0x803980F0 - the screen's phase entry and its per-frame phase advance, the two
 * helpers `menu/multi_result.cpp` drives its box band through (rule 2: this unit owns both
 * addresses; the declarations were added when that unit became their first consumer). */
void q_result_phase_enter(QResultScreen* self, u32 phase);
void q_result_phase_apply(QResultScreen* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_RESULT_H */
