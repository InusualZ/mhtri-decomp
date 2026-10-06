/* Leaf header (docs/plan.md 6.5 rule 2): the lobby list/panel helpers of `lobby/fn_80212810.cpp` the lobby screens
 * call (the list record is `lobby/lb_list.h`'s).  C linkage (the map rows are plain names). */
#ifndef MHTRI_LOBBY_LB_LIST_INIT_H
#define MHTRI_LOBBY_LB_LIST_INIT_H

#include "types.h"
#include "lobby/lb_list.h"   /* LbList, the list record */

struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80214654 - sets list `list` up (GUESS name). */
void lb_list_init(LbList* list, s16 count, u8 kind, s16 str_base, s16 help_str, u16 off_mask, s32 se_ok, u32 flags,
                  u16 blink_mask);
/* 0x802146F0 - sets list `list` up from template `tmpl` with the greyed and blinking rows given (GUESS name). */
void lb_choice_init(LbList* list, const LbListTmpl* tmpl, u16 off_mask, u16 blink_mask);
/* 0x80216A68 - draws the item-need list `items` (`count` rows) at `pos` (GUESS name). */
void lb_item_need_list_draw(const LbItemCount* items, s8 count, u8 flag, struct _mh_ivec2_* pos, u8 mode, u16 a, u16 b);
/* 0x80216640 - draws the lobby window of item `item` at `pos` (GUESS name). */
void lb_window_draw(u16 item, s32 b, struct _mh_ivec2_* pos, s32 d, s32 sel, s32 f, u16 g);
/* 0x80214798 - one input step of `list`: 1 when a row is decided, 2 on cancel, else 0 (GUESS name). */
s32 lb_choice_step(LbList* list);
/* 0x80214948 - draws `list` on panel `panel` with its sprite rows and the help line on `help_panel` (GUESS name). */
void lb_choice_draw(LbList* list, u16 panel, const u16* sprites, const u16* rows, u16 help_panel, u8 flag);
/* 0x80214EF0 - puts lobby string `str` of group 3 on help panel `panel` (GUESS name). */
s32 lb_panel_msg_draw(s32 panel, s16 str);
/* 0x8021505C / 0x802150DC - print lobby string `str` of group 3 on panel `panel` in colour `color`, the second on
 * row `row` (GUESS names). */
void lb_panel_str_print(u16 panel, s16 str, s16 color);
void lb_panel_line_draw(u16 panel, s16 str, s16 color, s32 row);
/* 0x80215170 - draws the yes/no choice on panel `panel` with `sel` picked (GUESS name). */
void lb_panel_yes_no_draw(u16 panel, s32 sel);
/* 0x802159F0 - puts the page arrows of page `page` of `pages` on panel `panel` (GUESS name). */
void lb_page_arrow_draw(u16 panel, s16 page, s16 pages, u16 x, s32 active);
/* 0x80215C98 - draws a list frame with text `text` at `pos` (GUESS name). */
void lb_list_row_draw(s8* text, s32 sel, struct _mh_ivec2_* pos, u32 color, u8 mode);
/* 0x80215DD4 - the same at panel `sub_panel` inside panel `panel` (GUESS name). */
void lb_frame_draw_at(u16 panel, u16 sub_panel, s8* text, s32 sel, u32 color, u8 mode);
/* 0x80217934 - refreshes the lobby NPCs after a menu closes (GUESS name). */
s32 lb_panel_close(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_LIST_INIT_H */
