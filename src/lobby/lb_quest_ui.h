/* `lobby/lb_quest_ui.cpp`'s records: the kitchen screen's work block, the trade screen's work block, the scene effect's
 * model work, and the declarations of the unit's entry points other units call. */
#ifndef MHTRI_LOBBY_LB_QUEST_UI_H
#define MHTRI_LOBBY_LB_QUEST_UI_H

#include "types.h"

struct MHchar;
struct _PLW;
struct _EFT;

/* The choice box of `lobby/fn_80212810.cpp` (`lb_choice_init`/`lb_choice_step`/`lb_choice_draw`), defined here
 * because this unit is its first consumer to need the layout. */
/* A choice box's template, the 0x14-byte records the screens keep in `.data`: the row count, the kind (0xFF lets
 * the box size itself), two layout values, a word, the sound a taken row plays (-1 for none) and the cursor's
 * wrap rule. size: 0x14 */
typedef struct LbChoiceDef {
    /* +0x00 */ s16 count;
    /* +0x02 */ u8 kind;
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ s16 value_0x04;
    /* +0x06 */ s16 value_0x06;
    /* +0x08 */ s32 value_0x08;
    /* +0x0C */ s32 se;
    /* +0x10 */ s32 wrap;
} LbChoiceDef;

/* A choice box (`lb_choice_init` clears its 0x20 bytes): the cursor, the disabled-row mask, the template's fields
 * and the caller's value. size: 0x20 */
typedef struct LbChoiceMenu {
    /* +0x00 */ s16 cursor;
    /* +0x02 */ u16 disabled;
    /* +0x04 */ s16 value;
    /* +0x06 */ s16 count;
    /* +0x08 */ u8 kind;
    /* +0x09 */ u8 pad_0x09;
    /* +0x0A */ s16 value_0x0A;
    /* +0x0C */ s16 value_0x0C;
    /* +0x0E */ u8 pad_0x0E[0x2];
    /* +0x10 */ s32 value_0x10;
    /* +0x14 */ s32 se;
    /* +0x18 */ s32 wrap;
    /* +0x1C */ u8 pad_0x1C[0x4];
} LbChoiceMenu;

/* One ingredient row of the kitchen screen: the ingredient id (`group * 3 + pick`, 0xFFFF for an empty pick), the
 * rare mark, the group the pair tables key on and whether the row can be picked.  `lb_kitchen_row_copy` moves the
 * first five bytes; the rows sit at a 6-byte stride. size: 0x6 */
typedef struct LbKitchenRow {
    /* +0x0 */ u16 id;
    /* +0x2 */ u8 rare;
    /* +0x3 */ u8 group;
    /* +0x4 */ u8 enabled;
    /* +0x5 */ u8 pad_0x5;
} LbKitchenRow;

/* The kitchen screen's work block at `lobby_w.menu_0xAC` (screen 0x11): `lb_kitchen_open` memsets the whole 0x2000
 * bytes.  The two picked ingredients, the twelve rolled ingredients (two pages of six), the visible page, the three
 * special-course picks and the rolled bonus courses.
 * size: 0x2000 */
typedef struct LbKitchenWork {
    /* +0x000 */ u8 state;                /* the screen step `lb_kitchen_step` switches on (0..7) */
    /* +0x001 */ u8 step;                 /* the input sub-step of the pick/special pages */
    /* +0x002 */ u8 tier;                 /* the meal tier (0..2): the cost row and the string base */
    /* +0x003 */ u8 pay_menu;             /* which payment menu the confirm box shows (0..2) */
    /* +0x004 */ u8 payment;              /* 0 free, 1 zenny, 2 resource points */
    /* +0x005 */ u8 online;               /* 0 offline, 1 the host's meal, 2 a guest's */
    /* +0x006 */ u16 seed;                /* the roll seed `lb_kitchen_courses_roll` steps */
    /* +0x008 */ u16 page_moved;          /* `menu_cursor_step_fixed_tail`'s moved flag */
    /* +0x00A */ u8 pad_0x00A[0x2];
    /* +0x00C */ u16 meal;                /* the pair's meal id */
    /* +0x00E */ u16 skill;               /* the pair's skill row */
    /* +0x010 */ u16 act;                 /* the NPC act the meal ends on */
    /* +0x012 */ u16 message;             /* the closing message string */
    /* +0x014 */ s16 special_count;       /* how many special picks the course takes (2 or 3) */
    /* +0x016 */ u16 special_id;          /* the special course's hold id (0x1B0 takes three picks) */
    /* +0x018 */ s16 list_cursor;         /* the special list's cursor */
    /* +0x01A */ s16 row_cursor;          /* the ingredient page's cursor */
    /* +0x01C */ s16 course_cursor;       /* the course cursor (0..3), the roll the course list shows */
    /* +0x01E */ s16 confirm_cursor;
    /* +0x020 */ s16 confirm_count;
    /* +0x022 */ s16 row_count;           /* rows on the current page (6) or in the special list */
    /* +0x024 */ s16 page;
    /* +0x026 */ s16 page_count;
    /* +0x028 */ LbChoiceMenu choice;     /* the opening choice box; its cursor picks meal (0) or special (1) */
    /* +0x048 */ bool confirm_enabled[3]; /* per confirm row: whether it can be taken */
    /* +0x04B */ s8 special[3];           /* the special picks, 0 when empty */
    /* +0x04E */ s16 courses[4];          /* the rolled bonus courses */
    /* +0x056 */ s16 extra_course;        /* the rolled fifth course */
    /* +0x058 */ u16 bonuses[4];          /* the bonuses the meal gives */
    /* +0x060 */ u16 holds[2];            /* the special courses the player holds */
    /* +0x064 */ u16 list[6];             /* the special list's entries */
    /* +0x070 */ LbKitchenRow picks[2];   /* the picked ingredients */
    /* +0x07C */ LbKitchenRow rows[12];   /* the rolled ingredients, two pages of six */
    /* +0x0C4 */ LbKitchenRow page_rows[6]; /* the current page's rows */
    /* +0x0E8 */ struct _PLW* npc;        /* the cook's player work */
    /* +0x0EC */ s32 arg_0x0EC;           /* the opener's second argument */
    /* +0x0F0 */ u8 pad_0x0F0[0x2000 - 0xF0];
} LbKitchenWork; /* size: 0x2000 */

/* One row of the trade list: the trade's index in the server's notice table, its state bits (1 sold out, 2 nothing
 * affordable, 4 the stock is short) and, per price, whether the player holds enough. size: 0x6 */
typedef struct LbTradeRow {
    /* +0x0 */ s16 index;
    /* +0x2 */ u8 state;
    /* +0x3 */ u8 affordable[3];
} LbTradeRow;

/* One price of a trade: the item and how many. size: 0x4 */
typedef struct LbTradeCost {
    /* +0x0 */ u16 item;
    /* +0x2 */ s16 count;
} LbTradeCost;

/* One 0x10-byte trade record of the server's notice table (`NetCtrlWk::getServerNotice`): the item on offer and its
 * three prices. size: 0x10 */
typedef struct LbTradeOffer {
    /* +0x0 */ LbTradeCost item;
    /* +0x4 */ LbTradeCost costs[3];
} LbTradeOffer;

/* The trade screen's opener record (`+0x5C`): only its id byte is read. size: 0x3 (approximate) */
typedef struct LbTradeSource {
    /* +0x0 */ u8 pad_0x0[0x2];
    /* +0x2 */ u8 id;
} LbTradeSource;

/* The trade screen's work block at `lobby_w.menu_0xAC` (screen 0x12): `lb_trade_open` memsets the whole 0x2000
 * bytes.
 * size: 0x2000 */
typedef struct LbTradeWork {
    /* +0x000 */ u8 state;                /* the screen step `lb_trade_step` switches on (0..6) */
    /* +0x001 */ u8 hold;                 /* the frames the exchange's repeat still waits */
    /* +0x002 */ u16 page_moved;
    /* +0x004 */ s16 cost_count;          /* the prices the picked trade has (1..3) */
    /* +0x006 */ s16 cost_cursor;
    /* +0x008 */ s16 row_count;
    /* +0x00A */ s16 row_cursor;
    /* +0x00C */ s16 page;
    /* +0x00E */ s16 page_count;
    /* +0x010 */ s16 repeat;              /* the exchange's repeat countdown */
    /* +0x012 */ u8 pad_0x012[0x2];
    /* +0x014 */ s32 yes_no;              /* the yes/no toggle */
    /* +0x018 */ LbChoiceMenu choice;     /* the opening choice box; its cursor picks the box (0) or the pouch (1) */
    /* +0x038 */ LbTradeRow rows[5];
    /* +0x056 */ u8 pad_0x056[0x2];
    /* +0x058 */ s32 arg_0x058;           /* the opener's first argument */
    /* +0x05C */ LbTradeSource* source;   /* the opener's second argument */
    /* +0x060 */ u8 pad_0x060[0x2000 - 0x60];
} LbTradeWork; /* size: 0x2000 */

/* The scene effect's work (`_EFT::work_0x38`, effect 0x36): five models, each with its slide frame and four effect
 * timers. size: 0x4C */
typedef struct LbSceneEftWork {
    /* +0x00 */ s32 count;
    /* +0x04 */ struct MHchar* models[5];
    /* +0x18 */ s16 frames[5];
    /* +0x22 */ s16 timers[5][4];
    /* +0x4A */ u8 pad_0x4A[0x2];
} LbSceneEftWork; /* size: 0x4C */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8038F4D8: opens the kitchen screen for the cook `npc`. */
void lb_kitchen_open(struct _PLW* npc, s32 arg);
/* 0x80390F20: the kitchen screen's frame step. */
void lb_kitchen_step(void);
/* 0x803928F0: opens the trade screen. */
void lb_trade_open(s32 arg, LbTradeSource* source);
/* 0x80392CC8: the trade screen's frame step. */
void lb_trade_step(void);
/* 0x80393994: spawns the scene effect. */
void lb_scene_eft_spawn(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_QUEST_UI_H */
