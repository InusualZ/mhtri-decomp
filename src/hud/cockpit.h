/* The cockpit HUD band `hud/cockpit.cpp` (`.text` 0x802D9EA4..0x802E0740): the entries other units call
 * (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_HUD_COCKPIT_H
#define MHTRI_HUD_COCKPIT_H

#include "types.h"

extern "C" {

/* 0x802DFC6C - a cockpit entry with no arguments that the note band's pane code calls (no body yet). */
void fn_802DFC6C(void);

}

/* Declarations moved here from `unsplit/lobby.h, menu.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

s8* str_tbl_33_get(u8 id);

/* 0x802DF6E4 - draws the money panel at layout `id` (the save's zenny, `%d` plus the currency sign).  GUESS
 * name; the spelling is the one `unsplit/lobby.h` and `lobby/lb_menu_page.cpp` already carry (one C signature). */
s32 menu_money_draw(s32 id);
/* 0x802D9EA4 - the cockpit's per-frame reaction hook the result screen runs while a page is open. */
void ai_npc_reaction_forward(void);

void draw_lsp_parts(void);                 /* 0x802DFEBC */

/* 0x802DFB70 - shows the new-mail notice: the cockpit icon and sound 0x27 (GUESS name). */
void cockpitShowNewMail(void);

/* 0x802E0468 - stores player slot `slot`'s transfer-mode icon (0 none, 1..3) in the cockpit state's per-slot byte
 * +0x7C.  NAME (a GUESS): its one caller is the network control's `updateTransferMode`, which sets it for the four
 * slots from the friend list and the layer's transfer state. */
void setCockpitTransferMode(u8 slot, s32 mode);

/* 0x802DE224 - holds the AI NPC page for this frame (its +0x25 byte).  GUESS name. */
void ainpc_page_hold_set(void);
/* 0x802DE360 / 0x802DE3F0 - draw the page's button marks at layout `panel` (moved by `offset` when given) unless the
 * page is hidden.  GUESS names. */
void ainpc_page_mark_a_draw(u16 panel, const struct _mh_ivec2_* offset);
void ainpc_page_mark_b_draw(u16 panel, const struct _mh_ivec2_* offset);

#ifdef __cplusplus
}

/* 0x802DAF48 - the page arrows over the id table `table` for page `page` of `count`, lit by the page-step `flags`
 * (`PutPageArrow__FPUsssUsPC10_mh_ivec2_Uc`, C++ scope - rule 9). */
struct _mh_ivec2_;
/* 0x802DE480 - whether the pointer (the Wii remote cursor) is in use (`chk_pointer__Fv`). */
s32 chk_pointer(void);
void PutPageArrow(u16* table, s16 page, s16 count, u16 flags, const struct _mh_ivec2_* pos, u8 mode);
#endif

#endif /* MHTRI_HUD_COCKPIT_H */
