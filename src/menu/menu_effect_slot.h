/* menu/menu_effect_slot.h - the declarations of `menu/menu_effect_slot.cpp`'s symbols (docs/plan.md 6.5 rule 2): the
 * menu page the top menu opens as kind 3 (four tabbed lists over one `MenuSlot`) and its list builders.
 */
#ifndef MHTRI_MENU_MENU_EFFECT_SLOT_H
#define MHTRI_MENU_MENU_EFFECT_SLOT_H

#include "types.h"

struct MenuSlot;
struct MenuScroll;
struct MenuEntry;

/* One (first row, row count) pair of `MenuSlot::kind_row_base`, the pair `note_slot_cursor_bounds` returns in the
 * high half of its result.  size: 0x2 */
typedef struct MenuRowPair {
    /* +0x00 */ u8 first;            /* the first row of the list (0xFF: the list is an option card) */
    /* +0x01 */ u8 count;            /* the number of rows it shows */
} MenuRowPair;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80348A48 - opens the page: tab cursor and sub-state cleared, the slide-in frame at 30. */
void menu_effect_slot_open(struct MenuSlot* slot);
/* 0x80348A78 - the page's frame handler: picks a tab, then steps the tab's list; returns 2 when it closes. */
s32 menu_effect_slot_step(struct MenuSlot* slot);
/* 0x80348CA4 / 0x80348CF4 - the item-row tab (the save's eight row records): open, step. */
void menu_row_list_open(struct MenuSlot* slot);
s32 menu_row_list_step(struct MenuSlot* slot);
/* 0x80348FD4 / 0x80349080 - the marked-monster tab: open, step. */
void menu_monster_list_open(struct MenuSlot* slot);
s32 menu_monster_list_step(struct MenuSlot* slot);
/* 0x80349184 / 0x80349228 - the known-combination tab: open, step. */
void menu_combo_list_open(struct MenuSlot* slot);
s32 menu_combo_list_step(struct MenuSlot* slot);
/* 0x8034932C / 0x80349398 - the three-level note tab and its option card: open, step. */
void menu_note_list_open(struct MenuSlot* slot);
s32 menu_note_list_step(struct MenuSlot* slot);
/* 0x80349914 - copies a row pair byte by byte. */
void note_slot_pair_copy(MenuRowPair* dst, const MenuRowPair* src);
/* 0x80349928 / 0x80349A94 - builds the item-row list's visible rows; scrolls it. */
void menu_row_list_build(struct MenuScroll* scroll, struct MenuEntry* rows);
void menu_row_list_scroll(struct MenuSlot* slot, u16 buttons);
/* 0x80349B04 / 0x80349C44 - builds the combination list's visible rows; scrolls it. */
void menu_combo_list_build(struct MenuScroll* scroll, struct MenuEntry* rows);
void menu_combo_list_scroll(struct MenuScroll* scroll, struct MenuEntry* rows, u16 buttons);
/* 0x80349C9C - builds the monster list's visible rows. */
void menu_monster_list_build(struct MenuScroll* scroll, struct MenuEntry* rows);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_EFFECT_SLOT_H */
