/* `LbList`, the 0x20-byte lobby menu list record the list helpers of `lobby/fn_80212810.cpp` keep (rule 1: one
 * definition, included by `lobby/lb_list_init.h` and the screens that embed it). */
#ifndef MHTRI_LOBBY_LB_LIST_H
#define MHTRI_LOBBY_LB_LIST_H

#include "types.h"

/* A lobby menu list: the cursor row, the rows shown greyed and blinking, the row count, its string group and first
 * string, the help line, and the decide sound and flags `lb_choice_step` uses.  size: 0x20 */
typedef struct LbList {
    /* +0x00 */ s16 cursor_0x00;
    /* +0x02 */ u16 off_mask_0x02;     /* a set bit greys the row out (deciding it is refused) */
    /* +0x04 */ u16 blink_mask_0x04;   /* a set bit blinks the row */
    /* +0x06 */ s16 count_0x06;
    /* +0x08 */ u8 kind_0x08;          /* the `LbStr` group of the rows; 0xFF builds the list from a table */
    /* +0x09 */ u8 pad_0x09;
    /* +0x0A */ s16 str_base_0x0A;     /* the rows' first string */
    /* +0x0C */ s16 help_str_0x0C;     /* the help line's first string (-1: none) */
    /* +0x0E */ u8 pad_0x0E[0x2];
    /* +0x10 */ s16* help_tbl_0x10;    /* per-row help string pairs */
    /* +0x14 */ s32 se_ok_0x14;        /* the decide sound (-1: none) */
    /* +0x18 */ u32 flags_0x18;        /* passed to the cursor step */
    /* +0x1C */ u8 pad_0x1C[0x4];
} LbList;

/* The template `lb_choice_init` copies a list from: the row count, string group, first string, help line, help
 * table, decide sound and flags.  size: 0x18 */
typedef struct LbListTmpl {
    /* +0x00 */ s16 count_0x00;
    /* +0x02 */ u8 kind_0x02;
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ s16 str_base_0x04;
    /* +0x06 */ s16 help_str_0x06;
    /* +0x08 */ s16* help_tbl_0x08;
    /* +0x0C */ s32 se_ok_0x0C;
    /* +0x10 */ u32 flags_0x10;
    /* +0x14 */ u8 pad_0x14[0x4];
} LbListTmpl;

/* An item and a count, the pair the lobby item lists walk.  size: 0x4 */
typedef struct LbItemCount {
    /* +0x00 */ u16 item_0x00;
    /* +0x02 */ s16 count_0x02;
} LbItemCount;

#endif /* MHTRI_LOBBY_LB_LIST_H */
