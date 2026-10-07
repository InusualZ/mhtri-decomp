/*
 * lobby/lb_panel_msg_draw.h - leaf header (docs/plan.md 6.5 rule 2) for `lobby/fn_80212810.cpp`'s lobby panel
 *   helpers, in the owner's own signatures; the owner has no header of its own yet, and `unsplit/lobby.h` cannot sit
 *   beside a unit that keeps its own `lobby_w` view.
 */
#ifndef MHTRI_LOBBY_LB_PANEL_MSG_DRAW_H
#define MHTRI_LOBBY_LB_PANEL_MSG_DRAW_H

#include "types.h"

struct _mh_ivec2_;

struct LbChoiceMenu;
struct LbChoiceDef;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80214EF0 - draws lobby string 3/`id` in the message panel at layout `panel`.  GUESS name. */
s32 lb_panel_msg_draw(s32 panel, s16 id);
/* 0x80215170 - draws the yes/no row at layout `panel` with the answer `sel` (0 = yes) lit.  GUESS name. */
s32 lb_panel_yes_no_draw(s32 panel, s32 sel);
/* 0x802159F0 - the page arrows of layout `panel`: page `page` of `count`, lit by the page-step `flags`, dimmed
 * unless `active`.  GUESS name. */
s32 lb_page_arrow_draw(s32 panel, s16 page, s16 count, u16 flags, u32 active);
/* 0x80217934 - closes the lobby panel (a tail call into 0x802178B8 with 0).  GUESS name. */
s32 lb_panel_close(void);
/* 0x802146F0 - fills the choice box `menu` from its template `def`, with the disabled-row mask `disabled`.  GUESS. */
void lb_choice_init(struct LbChoiceMenu* menu, const struct LbChoiceDef* def, u16 disabled, s16 value);
/* 0x80214798 - steps the choice box: 1 when a row is taken, 2 when it is cancelled, 0 otherwise.  GUESS name. */
s32 lb_choice_step(struct LbChoiceMenu* menu);
/* 0x80214948 - draws the choice box at layout `panel`.  GUESS name. */
void lb_choice_draw(struct LbChoiceMenu* menu, u16 panel, const u16* frame_ids, const u16* row_ids, u16 msg_panel, u8 mode);
/* 0x802150DC - draws lobby string 3/`id` as line `line` of layout `panel` in colour `color`.  GUESS name. */
void lb_panel_line_draw(u16 panel, s16 id, s16 color, s32 line);
/* 0x8021584C - draws the sprites `ids` (0xFFFF-ended) with a pulsing additive blend.  GUESS name. */
void lb_sprite_pulse_draw(const u16* ids, const struct _mh_ivec2_* pos);
/* 0x80215C04 - draws the resource-point total at layout `panel`.  GUESS name. */
void lb_points_draw(u16 panel);
/* 0x80215C98 - draws one list row: its frame of `kind`, the cursor when `cursor`, and `text` in `color`.  GUESS. */
void lb_list_row_draw(s8* text, s32 cursor, struct _mh_ivec2_* pos, u32 color, u8 kind);
/* 0x802164F0 / 0x80216528 - draw one item cell (`item`, `count`) in the wide (7) / plain (0) frame.  GUESS names. */
void lb_item_cell_draw_wide(u16 item, s32 count, struct _mh_ivec2_* pos, s32 enabled, s32 cursor, s32 active, u16 width);
void lb_item_cell_draw(u16 item, s32 count, struct _mh_ivec2_* pos, s32 enabled, s32 cursor, s32 active, u16 width);
/* 0x80217118 - draws the item detail panel of `item` at layout `panel`.  GUESS name. */
void lb_item_detail_draw(u16 panel, u16 item, const u16* ids, s32 box_mark, s32 pouch_mark);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_PANEL_MSG_DRAW_H */
